#pragma once
#include "sim/types.h"
#include "module_io.h"
#include "rob.h"
namespace sim {
struct RSEntry {
  bool busy;
  Ins ins;
  u32 v1, v2;
  Tag q1, q2;
  Tag rt;
  u32 pc;
};

struct RS {
  RSEntry e[2][RSN];
  void reset() { memset(this, 0, sizeof(*this)); }
  void tick() { memcpy(e[0], e[1], sizeof(e[0])); }
  
  u32 allocate(const Ins &ins, u32 v1, Tag q1, u32 v2, Tag q2, Tag rob_tag,
               u32 pc) {
    for (u32 i = 0; i < RSN; i++) {
      if (!e[0][i].busy && !e[1][i].busy) {
        e[1][i].busy = 1;
        e[1][i].ins = ins;
        e[1][i].v1 = v1;
        e[1][i].v2 = v2;
        e[1][i].q1 = q1;
        e[1][i].q2 = q2;
        e[1][i].rt = rob_tag;
        e[1][i].pc = pc;
        return i;
      }
    }
    return RSN;
  }
  bool ready(u32 idx) const {
    return e[0][idx].busy && !e[0][idx].q1.isvalid() && !e[0][idx].q2.isvalid();
  }

  i32 find_ready() const {
    for (u32 i = 0; i < RSN; i++) {
      if (ready(i))
        return i;
    }
    return -1;
  }

  void clear(u32 idx) { e[1][idx].busy = 0; }

  RSEntry &get(u32 idx) { return e[0][idx]; }

  void wakeup(Tag tag, u32 val) {
    for (u32 i = 0; i < RSN; ++i) {
      if (!e[0][i].busy)
        continue;
      bool flg1 = e[0][i].q1 == tag, flg2 = e[0][i].q2 == tag;
      if (flg1 || flg2) {
        e[1][i] = e[0][i];
        if (flg1) {
          e[1][i].q1 = Tag{};
          e[1][i].v1 = val;
        }
        if (flg2) {
          e[1][i].q2 = Tag{};
          e[1][i].v2 = val;
        }
      }
    }
  }

  void snapshot() { memcpy(e[1], e[0], sizeof(e[0])); }

  void flush() {
    for (u32 i = 0; i < RSN; i++) {
      e[1][i].busy = 0;
    }
  }
};


//the following is the real implementation in tomasulo, which holds same but match the ends.

struct RSState {
  RSEntry e[RSN];

  void reset() { *this = RSState{}; }

  u32 find_free() const {
    for (u32 i = 0; i < RSN; ++i)
      if (!e[i].busy)
        return i;
    return RSN;
  }

  ExecuteOutput evaluate(const ROBState &rob) const {
    ExecuteOutput output{};
    for (u32 i = 0; i < RSN; ++i) {
      const RSEntry &entry = e[i];
      if (!entry.busy || entry.q1.isvalid() || entry.q2.isvalid() ||
          !rob.tag_live(entry.rt))
        continue;
      output.valid = true;
      output.rs_slot = i;
      output.tag = entry.rt;
      output.ins = entry.ins;
      output.store_value = entry.v2;
      output.result = alu(entry.ins, entry.v1, entry.v2, entry.pc);
      output.memory_op = output.result.mem;
      break;
    }
    return output;
  }

  static void wakeup_entry(RSEntry &entry, Tag tag, u32 value) {
    if (!entry.busy || !tag.isvalid())
      return;
    if (entry.q1 == tag) {
      entry.q1 = Tag{};
      entry.v1 = value;
    }
    if (entry.q2 == tag) {
      entry.q2 = Tag{};
      entry.v2 = value;
    }
  }

  void latch(const CycleWires &wires) {
    RSState candidate = *this;
    if (squash_cycle(wires)) {
      candidate = RSState{};
    } else {
      if (wires.cdb.execute_accepted)
        candidate.e[wires.execute.rs_slot] = RSEntry{};
      if (accept_issue(wires) && wires.issue.uses_rs) {
        RSEntry &entry = candidate.e[wires.issue.rs_slot];
        entry = RSEntry{};
        entry.busy = true;
        entry.ins = wires.issue.ins;
        entry.v1 = wires.issue.v1;
        entry.v2 = wires.issue.v2;
        entry.q1 = wires.issue.q1;
        entry.q2 = wires.issue.q2;
        entry.rt = wires.issue.tag;
        entry.pc = wires.issue.pc;
      }
      for (u32 i = 0; i < RSN; ++i) {
        if (wires.cdb.valid)
          wakeup_entry(candidate.e[i], wires.cdb.tag, wires.cdb.value);
        if (wires.commit.write_reg)
          wakeup_entry(candidate.e[i], wires.commit.tag, wires.commit.value);
      }
    }
    *this = candidate;
  }


};

} // namespace sim