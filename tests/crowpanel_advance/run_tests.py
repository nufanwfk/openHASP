#!/usr/bin/env python3
"""Host checks of the actual board helper; no hardware or full-firmware claim."""
import os, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)
 (p/'hasplib.h').write_text('''#pragma once
#include <stdint.h>
#include <vector>
#define F(x) x
#define INPUT 0
#define OUTPUT 1
#define LOW 0
extern std::vector<int> pinCalls;
inline void delay(int) {}
inline void pinMode(int p,int m) { pinCalls.push_back(p*10+m); }
inline void digitalWrite(int p,int v) { pinCalls.push_back(p*10+v); }
''')
 (p/'hasp_debug.h').write_text('#define LOG_ERROR(...) ((void)0)\n')
 (p/'Wire.h').write_text('''#pragma once
#include <cassert>
#include <vector>
struct FakeWire {
 bool controller=true, touch=true; int address=0, payload=-1, starts=0;
 std::vector<int> commands;
 void begin(int sda,int scl,int speed) { assert(sda==15&&scl==16&&speed==400000); ++starts; }
 void setTimeOut(int t) { assert(t==50); }
 void beginTransmission(int a) {address=a;payload=-1;}
 void write(int v) {payload=v;commands.push_back(v);}
 int endTransmission() {return address==0x30 ? (controller?0:2) : (touch?0:2);}
};
extern FakeWire Wire;
''')
 (p/'test.cpp').write_text('''#include "hasplib.h"
#include "Wire.h"
#include "hal/boards/crowpanel_advance.h"
#include <string>
FakeWire Wire; std::vector<int> pinCalls;
int main(int argc,char**argv) {
 std::string mode=argv[1]; Wire.controller=mode!="absent"; Wire.touch=mode=="normal";
 bool ok=crowpanelAdvanceBegin(); assert(ok==(mode=="normal"));
 assert(crowpanelAdvanceBegin()==ok); assert(Wire.starts==1);
 if(mode=="absent") {assert(pinCalls.empty());assert(Wire.commands.empty());crowpanelAdvanceBacklight(255,true);assert(Wire.commands.empty());return 0;}
 if(mode=="missing_touch") {int count=0;for(int c:Wire.commands)if(c==250)++count;assert(count==5);assert(pinCalls.size()==15);}
 else assert(pinCalls.empty());
 for(int level=0;level<256;++level) {crowpanelAdvanceBacklight(level,true);assert(Wire.commands.back()<=245);}
 assert(Wire.commands.back()==0);crowpanelAdvanceBacklight(0,true);assert(Wire.commands.back()==245);
 crowpanelAdvanceBacklight(255,false);assert(Wire.commands.back()==245);
 assert(crowpanelAdvancePinInUse(1));assert(crowpanelAdvancePinInUse(15));assert(crowpanelAdvancePinInUse(48));
 assert(!crowpanelAdvancePinInUse(19));assert(!crowpanelAdvancePinInUse(20));
}
''')
 args=[os.getenv('CXX','c++'),'-std=c++11','-Wall','-Wextra','-Wno-unused-parameter','-DESP32=1','-DHASP_CROWPANEL_ADVANCE_STC=1','-I'+str(p),'-I'+str(root/'src'),str(p/'test.cpp'),str(root/'src/hal/boards/crowpanel_advance.cpp'),'-o',str(p/'test')]
 subprocess.run(args,check=True)
 for case in ['normal','absent','missing_touch']: subprocess.run([str(p/'test'),case],check=True)
print('PASS: brightness bounds, missing controller, bounded touch recovery, repeated begin and reserved pins')
