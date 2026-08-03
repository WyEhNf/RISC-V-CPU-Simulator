#pragma once
#include "sim/types.h"
namespace sim {

struct ROBEntry {
  Ins ins;
  bool rdy;
  u32 val, addr, pc;
  Tag tag;
  bool pt;
};
struct ROB {
  ROBEntry e[2][RBN];
  u32 head[2], tail[2];
  u32 generation[RBN];
  bool term;
  u32 outv;

  void reset() { memset(this, 0, sizeof(*this)); }
  u32 allocate() {
    u32 ri = tail[1];
    if (++generation[ri] == 0)
      ++generation[ri];
    e[1][ri] = ROBEntry{};
    e[1][ri].tag = Tag{TROB, ri, generation[ri]};
    tail[1] = (tail[1] + 1) % RBN;
    return ri;
  }
  bool full() const {
    u32 nt = (tail[1] + 1) % RBN;
    return nt == head[1];
  }
  void mark_ready(u32 idx, u32 v, u32 a = 0) {
    e[1][idx].rdy = true;
    e[1][idx].val = v;
    e[1][idx].addr = a;
  }
  bool head_ready() const { return head[0] != tail[0] && e[0][head[0]].rdy; }
  ROBEntry head_entry() const { return e[0][head[0]]; }
  void advance_head() { head[1] = (head[0] + 1) % RBN; }
  void flush_younger(u32 from_idx) { tail[1] = from_idx; }
  void set_term(u32 v) {
    term = 1;
    outv = v;
  }
  void tick() {
    memcpy(e[0], e[1], sizeof(e[0]));
    head[0] = head[1];
    tail[0] = tail[1];
  }
};
} // namespace sim