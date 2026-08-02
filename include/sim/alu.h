#pragma once
#include "sim/types.h"
namespace sim {
struct AR {
  u32 v;
  bool bt, mem;
  u32 btgt;
};
inline AR alu(const Ins &d, u32 v1, u32 v2, u32 pc) {
  AR res{};
  res.v = 0;
  switch (d.alu) {
  case ADD:
    res.v = v1 + v2;
    break;
  case SUB:
    res.v = v1 - v2;
    break;
  case AND:
    res.v = v1 & v2;
    break;
  case OR:
    res.v = v1 | v2;
    break;
  case XOR:
    res.v = v1 ^ v2;
    break;
  case SLL:
    res.v = v1 << (v2 & 31u);
    break;
  case SRL:
    res.v = v1 >> (v2 & 31u);
    break;
  case SRA:
    res.v = (u32)((i32)v1 >> (v2 & 31u));
    break;
  case SLT:
    res.v = (i32)v1 < (i32)v2 ? 1u : 0u;
    break;
  case SLTU:
    res.v = v1 < v2 ? 1u : 0u;
    break;
  default:
    break;
  }
  if (d.op == IM || d.op == S) {
    res.mem = 1;
    res.v = v1 + (u32)d.imm;
  }
  if (d.op == IC) {
    res.v = (u32)((i32)pc + 4);
    res.bt = 1;
    res.btgt = (u32)((i32)v1 + d.imm) & (-1u);
  }
  if (d.op == B) {
    bool c = 0;
    switch (d.branch) {
    case EQ:
      c = v1 == v2;
      break;
    case NE:
      c = v1 != v2;
      break;
    case LT:
      c = (i32)v1 < (i32)v2;
      break;
    case LTU:
      c = v1 < v2;
      break;
    case GE:
      c = (i32)v1 >= (i32)v2;
      break;
    case GEU:
      c = v1 >= v2;
      break;
    default:
      break;
    }
    res.bt = c;
    res.v = c ? 1u : 0u;
    res.btgt = (u32)((i32)pc + d.imm);
  }
  if (d.op == J) {
    res.bt = 1;
    res.btgt = (u32)((i32)pc + d.imm);
    res.v = pc + 4;
  }
  if (d.op == U) {
    if (d.alu == ADD)
      res.v = (u32)((i32)pc + (i32)d.imm);
    else
      res.v = (u32)((i32)d.imm);
  }

  return res;
}
} // namespace sim