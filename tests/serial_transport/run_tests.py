#!/usr/bin/env python3
"""Test framing and actual extension using fake hardware and real ArduinoJson 6."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--arduinojson', type=Path, required=True, help='ArduinoJson 6 library root (already an openHASP dependency)')
p.add_argument('--h5', type=Path, help='Optional nanoels/h5 directory for cross-repository compatibility tests')
args = p.parse_args()
includes = ['-I' + str(root / 'src/custom'), '-I' + str(root / 'tests/serial_transport/stubs'), '-I' + str(root / 'src'), '-I' + str(args.arduinojson / 'src')]
if args.h5:
    includes += ['-DH5_COMPATIBILITY=1', '-I' + str(args.h5)]
with tempfile.TemporaryDirectory() as tmp:
    for name, extra in [('framing', []), ('integration', ['-DHASP_USE_MQTT=0']), ('integration', ['-DHASP_USE_MQTT=1']), ('portable', ['-DTEST_UART_EXTERNAL_HAL=1']), ('portable', ['-DTEST_UART_UNSUPPORTED=1'])]:
        output = str(Path(tmp) / (name + str(len(extra))))
        subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) + ['-std=c++11', '-Wall', '-Wextra', '-Werror'] + shlex.split(os.environ.get('CXXFLAGS', '')) + includes + extra + [str(root / ('tests/serial_transport/' + name + '_test.cpp')), '-o', output], check=True)
        subprocess.run([output], check=True)
