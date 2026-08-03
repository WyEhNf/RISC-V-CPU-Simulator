#pragma once
#include "sim/cdb.h"
#include "sim/module_io.h"
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

// the following is the real implementation in tomasulo, which holds same but
// match the ends.
struct ROBState {
  u32 h, tx, generation[RBN];
  ROBEntry e[RBN];

  void reset() { *this = ROBState{}; }

  static bool in_ring(u32 index, u32 head, u32 tail) {
    return head <= tail ? index >= head && index < tail
                        : index >= head || index < tail;
  }

  bool tag_live(Tag tag) const {
    return tag.k == TROB && tag.i < RBN && in_ring(tag.i, h, tx) &&
           e[tag.i].tag == tag;
  }

  bool full() const { return (tx + 1) % RBN == h; }

  Tag next_tag() const {
    u32 next_generation = generation[tx] + 1;
    if (next_generation == 0)
      ++next_generation;
    return Tag{TROB, tx, next_generation};
  }

  CommitOutput evaluate(u32 committed_x10) const {
    CommitOutput output{};
    if (h == tx)
      return output;
    const ROBEntry &entry = e[h];
    if (!entry.rdy || !tag_live(entry.tag))
      return output;

    output.valid = true;
    output.tag = entry.tag;
    output.ins = entry.ins;
    output.value = entry.val;
    output.address = entry.addr;
    output.pc = entry.pc;
    output.terminate = entry.ins.raw == TERM;
    output.termination_value = committed_x10 & 0xFFu;
    output.write_reg = entry.ins.w && entry.ins.rd != 0;
    output.store = entry.ins.op == S;
    output.memory_op = entry.ins.il || entry.ins.is;
    output.branch = entry.ins.op == B;
    output.predicted_taken = entry.pt;
    if (output.branch) {
      const bool taken = entry.val != 0;
      output.mispredict = taken != entry.pt;
      output.redirect = output.mispredict;
      output.redirect_pc = taken ? entry.addr : entry.pc + 4;
    } else if (entry.ins.op == IC) {
      output.redirect = true;
      output.redirect_pc = entry.addr;
    }
    return output;
  }

  void latch(const CycleWires &wires) {
    ROBState candidate = *this;
    if (wires.commit.valid) {
      candidate.e[candidate.h] = ROBEntry{};
      candidate.h = (candidate.h + 1) % RBN;
    }
    if (wires.commit.mispredict) {
      for (u32 i = 0; i < RBN; ++i)
        candidate.e[i] = ROBEntry{};
      candidate.tx = candidate.h;
    } else if (!wires.commit.terminate) {
      if (wires.cdb.execute_accepted && wires.execute.memory_op &&
          wires.execute.tag.i < RBN &&
          candidate.e[wires.execute.tag.i].tag == wires.execute.tag) {
        ROBEntry &entry = candidate.e[wires.execute.tag.i];
        if (wires.execute.ins.op == S) {
          entry.val = wires.execute.store_value;
          entry.addr = wires.execute.result.v;
        }
      }
      if (wires.memory.valid && wires.memory.store &&
          wires.memory.tag.i < RBN &&
          candidate.e[wires.memory.tag.i].tag == wires.memory.tag)
        candidate.e[wires.memory.tag.i].rdy = true;
      if (wires.cdb.valid && wires.cdb.tag.i < RBN &&
          candidate.e[wires.cdb.tag.i].tag == wires.cdb.tag) {
        ROBEntry &entry = candidate.e[wires.cdb.tag.i];
        entry.rdy = true;
        entry.val = wires.cdb.value;
        if (wires.cdb.execute_accepted && !wires.execute.memory_op &&
            wires.execute.tag == wires.cdb.tag && wires.execute.result.bt)
          entry.addr = wires.execute.result.btgt;
      }
      if (accept_issue(wires)) {
        ROBEntry &entry = candidate.e[wires.issue.rob_slot];
        entry = ROBEntry{};
        entry.ins = wires.issue.ins;
        entry.tag = wires.issue.tag;
        entry.pc = wires.issue.pc;
        entry.pt = wires.issue.predicted_taken;
        entry.rdy = wires.issue.ins.op == FN;
        candidate.generation[wires.issue.rob_slot] = wires.issue.tag.generation;
        candidate.tx = (wires.issue.rob_slot + 1) % RBN;
      }
    }
    *this = candidate;
  }
};

} // namespace sim