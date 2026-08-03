#pragma once
#include "sim/types.h"
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
} // namespace sim