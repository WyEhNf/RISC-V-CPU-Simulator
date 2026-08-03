#pragma once
#include "sim/memory.h"
#include "sim/types.h"

namespace sim {
struct LSQEntry {
  bool busy, val_rdy, addr_rdy, il;
  u32 val, addr;
  Tag rt;
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
  void set_rdy(u32 idx,u32 val) {
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

}; // namespace sim