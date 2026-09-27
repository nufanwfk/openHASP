#include "uart_framing.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace hasp_uart;
std::vector<std::string> feed(Receiver& rx, const std::string& input) {
    std::vector<std::string> result;
    for(unsigned char c : input) if(rx.feed(c)) result.push_back(rx.line());
    return result;
}
std::string drain(Outbox& q, size_t chunk = 1) {
    std::string out;
    while(q.size()) { size_t n = q.size() < chunk ? q.size() : chunk; out.append(q.data(), n); q.consume(n); }
    return out;
}
int main() {
    Receiver rx;
    assert(feed(rx, "page 2\njsonl {\"page\":1}\r\n\n") ==
           (std::vector<std::string>{"page 2", "jsonl {\"page\":1}"}));
    assert(feed(rx, "page ").empty());
    assert(feed(rx, "1\n")[0] == "page 1");
    assert(feed(rx, std::string(1022, 'a') + "\r\n")[0].size() == 1022);
    assert(feed(rx, std::string(1023, 'a') + "\npage 1\n") == std::vector<std::string>{"page 1"});
    assert(feed(rx, std::string(3000, 'a') + "page 2\npage 1\n") == std::vector<std::string>{"page 1"});
    assert(feed(rx, std::string("bad\0page 2\n", 11)).empty());
    assert(feed(rx, "pa\rge 2\n").empty());
    assert(feed(rx, "page 1\n")[0] == "page 1");
    Outbox q;
    q.ready();
    assert(q.event("p1b48", "down"));
    assert(q.state("page", "2"));
    assert(drain(q, 3) == "\nready 1\nevent p1b48 down\npage 2\n");
    assert(q.state("x", std::string(510, 'v').c_str()));
    assert(drain(q).size() == 513);
    assert(!q.state("x", std::string(511, 'v').c_str()));
    assert(!q.state("a b", "v"));
    assert(!q.state("x", "v\npage 2"));
    assert(!q.state("x", "v\r"));
    assert(!q.state("", "v"));
    for(size_t i=0;i<Outbox::depth;++i) assert(q.state("page", "1"));
    assert(!q.event("p1b48", "release"));
    assert(drain(q) == "\nready 1\n");
    assert(q.state("page", "2"));
    q.consume(3); // interrupt an in-flight line at layout reload
    q.ready();
    assert(drain(q) == "\nready 1\n");
    assert(q.dropped == 6);
    std::cout << "PASS: framing, boundaries, partial writes, overflow recovery, ready ordering\n";
}
