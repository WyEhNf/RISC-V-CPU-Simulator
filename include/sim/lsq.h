#pragma once
#include "sim/memory.h"
#include "sim/module_io.h"
#include "sim/rob.h"
#include "sim/types.h"

namespace sim {
struct LSQEntry {
  bool busy, val_rdy, addr_rdy, request_pending, memory_complete, il;
  u32 val, addr;
  Tag rt;
  Ins ins;
};
struct LSQ {
  LSQEntry e[2][LSN];
  u32 head[2], tail[2];
  Mem *mem;

  void reset() { *this = LSQ{}; }
  void tick() {
    memcpy(e[0], e[1], sizeof(e[0]));
    head[0] = head[1];
    tail[0] = tail[1];
  }
  void snapshot() {
    memcpy(e[1], e[0], sizeof(e[0]));
    head[1] = head[0];
    tail[1] = tail[0];
  }
  u32 allocate(bool il, Tag rt) {
    u32 li = tail[1];
    tail[1] = (tail[1] + 1) % LSN;
    e[1][li].busy = 1;
    e[1][li].val_rdy = 0;
    e[1][li].addr_rdy = 0;
    e[1][li].il = il;
    e[1][li].rt = rt;
    e[1][li].val = 0;
    e[1][li].addr = 0;
    return li;
  }
  bool full() const {
    u32 nxt = (tail[1] + 1) % LSN;
    return nxt == head[1];
  }
  void set_addr(u32 idx, u32 addr) {
    e[1][idx].addr = addr;
    e[1][idx].addr_rdy = 1;
  }
  void set_store(u32 idx, u32 val) {
    e[1][idx].val = val;
    e[1][idx].val_rdy = 1;
  }
  void set_rdy(u32 idx, u32 val) {
    e[1][idx].val = val;
    e[1][idx].addr_rdy = 1;
  }
  LSQEntry &get(u32 idx) { return e[0][idx]; }
  u32 cur_h() const { return head[0]; }
  u32 cur_t() const { return tail[0]; }
  bool try_fwd(u32 i, u32 addr, u32 *v) {
    for (u32 j = head[0]; j != i; j = (j + 1) % LSN) {
      auto &tmp = e[0][j];
      if (tmp.busy && !tmp.il && tmp.addr == addr && tmp.val_rdy) {
        *v = tmp.val;
        return 1;
      }
    }
    return 0;
  }
  void advance_head() { head[1] = (head[0] + 1) % LSN; }
  void flush(u32 t) { tail[1] = t; }
  i32 find_by_tag(Tag t) const {
    for (u32 i = head[0]; i != tail[0]; i = (i + 1) % LSN) {
      if (e[0][i].busy && e[0][i].rt == t)
        return (i32)i;
    }
    return -1;
  }
};
struct LSQState {
  u32 h, tx;
  LSQEntry e[LSN];

  void reset() { *this = LSQState{}; }
  bool full() const { return (tx + 1) % LSN == h; }

  MemoryRequest evaluate(const ROBState &rob,
                         const MemoryPipelineState &memory) const {
    MemoryRequest output{};
    if (memory.busy)
      return output;

    if (h != tx) {
      const LSQEntry &head = e[h];
      if (head.busy && !head.il && head.val_rdy && !head.request_pending &&
          !head.memory_complete && rob.tag_live(head.rt) &&
          rob.h == head.rt.i && rob.e[rob.h].tag == head.rt) {
        output.valid = true;
        output.store = true;
        output.lsq_slot = h;
        output.tag = head.rt;
        output.address = head.addr;
        output.forwarded_value = head.val;
        output.operation = head.ins.mem;
        return output;
      }
    }

    for (u32 offset = 0; offset < LSN; ++offset) {
      const u32 i = (h + offset) % LSN;
      if (i == tx)
        break;
      const LSQEntry &entry = e[i];
      if (!entry.busy || !entry.il || entry.val_rdy || entry.request_pending ||
          !entry.addr_rdy || !rob.tag_live(entry.rt))
        continue;

      bool blocked = false, forwarded = false;
      u32 value = 0;
      for (u32 older_offset = 0; older_offset < LSN; ++older_offset) {
        const u32 j = (h + older_offset) % LSN;
        if (j == i)
          break;
        const LSQEntry &older = e[j];
        if (!older.busy || older.il)
          continue;
        if (!older.addr_rdy) {
          blocked = true;
          break;
        }
        if (older.addr == entry.addr) {
          if (!older.val_rdy) {
            blocked = true;
            break;
          }
          value = older.val;
          forwarded = true;
        }
      }
      if (blocked)
        continue;
      output.valid = true;
      output.lsq_slot = i;
      output.tag = entry.rt;
      output.address = entry.addr;
      output.operation = entry.ins.mem;
      output.forwarded = forwarded;
      output.forwarded_value = value;
      break;
    }
    return output;
  }

  void latch(const CycleWires &wires) {
    LSQState candidate = *this;
    if (wires.commit.mispredict || wires.commit.terminate) {
      candidate = LSQState{};
    } else {
      if (wires.commit.valid && wires.commit.memory_op &&
          candidate.h != candidate.tx) {
        candidate.e[candidate.h] = LSQEntry{};
        candidate.h = (candidate.h + 1) % LSN;
      }
      if (wires.cdb.execute_accepted && wires.execute.memory_op) {
        for (u32 offset = 0; offset < LSN; ++offset) {
          const u32 i = (candidate.h + offset) % LSN;
          if (i == candidate.tx)
            break;
          LSQEntry &entry = candidate.e[i];
          if (!entry.busy || entry.rt != wires.execute.tag)
            continue;
          entry.addr = wires.execute.result.v;
          entry.addr_rdy = true;
          if (!entry.il) {
            entry.val_rdy = true;
            entry.val = wires.execute.store_value;
          }
          break;
        }
      }
      if (wires.memory_request.valid && wires.memory_request.lsq_slot < LSN) {
        LSQEntry &entry = candidate.e[wires.memory_request.lsq_slot];
        if (entry.busy && entry.rt == wires.memory_request.tag)
          entry.request_pending = true;
      }
      if (wires.memory.valid && wires.memory.lsq_slot < LSN) {
        LSQEntry &entry = candidate.e[wires.memory.lsq_slot];
        if (entry.busy && entry.rt == wires.memory.tag) {
          entry.request_pending = false;
          entry.memory_complete = true;
          if (!wires.memory.store) {
            entry.val_rdy = true;
            entry.val = wires.memory.value;
          }
        }
      }
      if (accept_issue(wires) && wires.issue.memory_op) {
        LSQEntry &entry = candidate.e[wires.issue.lsq_slot];
        entry = LSQEntry{};
        entry.busy = true;
        entry.il = wires.issue.ins.il;
        entry.rt = wires.issue.tag;
        entry.ins = wires.issue.ins;
        candidate.tx = (wires.issue.lsq_slot + 1) % LSN;
      }
    }
    *this = candidate;
  }
};
} // namespace sim