#pragma once
#include "sim/types.h"
namespace sim {
struct RegFile {
  u32 rg[RN];
  Tag rt[RN];
  void reset() {
    memset(rg, 0, sizeof rg);
    rg[0] = 0, rt[0] = Tag{};
  }
  void write_reg(u8 rd, u32 val) {
    if (rd)
      rg[rd] = val;
  }
  void clear_rat(u8 rd, Tag t) {
    if (rd && rt[rd] == t)
      rt[rd] = Tag{};
  }
  void set_rat(u8 rd, Tag t) {
    if (rd)
      rt[rd] = t;
  }
  u32 arch(u8 rd) const { return rd ? rg[rd] : 0; }
  Tag rat_cur(u8 rd) const { return rd ? rt[rd] : Tag{}; }
};

using RegState = RegFile;

} // namespace sim