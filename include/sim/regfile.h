#pragma once
#include "sim/types.h"
#include "sim/module_io.h"
#include "sim/rs.h"
#include "sim/rob.h"
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


  template <class ROBLike>
  void resolve_operand(u8 r, const ROBLike &rob, u32 &value,
                       Tag &dependency) const {
    dependency = Tag{};
    if (r == 0) {
      value = 0;
      return;
    }
    const Tag producer = rt[r];
    if (!producer.isvalid() || !rob.tag_live(producer)) {
      value = rg[r];
      return;
    }
    const auto &entry = rob.e[producer.i];
    if (entry.rdy)
      value = entry.val;
    else
      dependency = producer;
  }

  void latch(const CycleWires &wires) {
    RegFile candidate = *this;
    if (wires.commit.write_reg) {
      candidate.write_reg(wires.commit.ins.rd, wires.commit.value);
      candidate.clear_rat(wires.commit.ins.rd, wires.commit.tag);
    }
    if (wires.commit.mispredict) {
      for (u32 i = 0; i < RN; ++i)
        candidate.rt[i] = Tag{};
    } else if (accept_issue(wires) && wires.issue.ins.w) {
      candidate.set_rat(wires.issue.ins.rd, wires.issue.tag);
    }
    candidate.rg[0] = 0;
    candidate.rt[0] = Tag{};
    *this = candidate;
  }

};

using RegState = RegFile;

} // namespace sim