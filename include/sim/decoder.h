#pragma once
#include "sim/types.h"
namespace sim::dec {

inline u32 ex(u32 mask, int l, int r) {
  return r == 32 ? mask >> l : (mask & ((1u << r) - 1u)) >> l;
}
inline i32 sx(u32 v, int b) {
  const u32 sign_bit = 1u << b;
  return static_cast<i32>((v ^ sign_bit) - sign_bit);
}

inline Ins decode(u32 raw) {
  Ins d{};
  d.raw = raw;
  u32 op = ex(raw, 0, 7);
  switch (op) {
  // R-type
  case 0b0110011:
    d.op = Op::R;
    d.rd = (u8)ex(raw, 7, 12);
    d.rs1 = (u8)ex(raw, 15, 20);
    d.rs2 = (u8)ex(raw, 20, 25);
    d.w = d.r1 = d.r2 = 1;
    switch (ex(raw, 12, 15) << 7 | ex(raw, 25, 32)) {
    case 0b00000000000:
      d.alu = AF::ADD;
      break;
    case 0b00000100000:
      d.alu = AF::SUB;
      break;
    case 0b11100000000:
      d.alu = AF::AND;
      break;
    case 0b11000000000:
      d.alu = AF::OR;
      break;
    case 0b10000000000:
      d.alu = AF::XOR;
      break;
    case 0b00100000000:
      d.alu = AF::SLL;
      break;
    case 0b10100000000:
      d.alu = AF::SRL;
      break;
    case 0b10101000000:
      d.alu = AF::SRA;
      break;
    case 0b01000000000:
      d.alu = AF::SLT;
      break;
    case 0b01100000000:
      d.alu = AF::SLTU;
      break;
    default:
      d.op  = Op::INV;
      break;
    }
    break;
  case 0b0010011:
    d.rd = (u8)ex(raw, 7, 12);
    d.rs1 = (u8)ex(raw, 15, 20);
    d.w = d.r1 = 1;
    {
      u32 f3 = ex(raw, 12, 15);
      if (f3 == 0b001 || f3 == 0b101) {
        d.op = IS;
        d.imm = (i32)ex(raw, 20, 25);
        u32 sh = ex(raw, 25, 32);
        if (f3 == 0b001 && sh == 0)
          d.alu = SLL;
        else if (f3 == 0b101 && sh == 0)
          d.alu = SRL;
        else if (f3 == 0b101 && sh == 0b0100000)
          d.alu = SRA;
        else
          d.op = INV;
      } else {
        d.op = IA;
        d.imm = sx(ex(raw, 20, 32), 11);
        switch (f3) {
        case 0b000:
          d.alu = ADD;
          break;
        case 0b111:
          d.alu = AND;
          break;
        case 0b110:
          d.alu = OR;
          break;
        case 0b100:
          d.alu = XOR;
          break;
        case 0b010:
          d.alu = SLT;
          break;
        case 0b011:
          d.alu = SLTU;
          break;
        default:
          d.op = INV;
          break;
        }
      }
    }
    break;

    // Load-Type
  case 0b0000011:
    d.op = IM;
    d.rd = (u8)ex(raw, 7, 12);
    d.rs1 = (u8)ex(raw, 15, 20);
    d.imm = sx(ex(raw, 20, 32), 11);
    d.w = d.r1 = d.il = 1;
    d.alu = ADD;
    switch (ex(raw, 12, 15)) {
    case 0b000:
      d.mem = MB;
      break;
    case 0b100:
      d.mem = MBU;
      break;
    case 0b001:
      d.mem = MH;
      break;
    case 0b101:
      d.mem = MHU;
      break;
    case 0b010:
      d.mem = MW;
      break;
    default:
      d.op = INV;
      break;
    }
    break;

  // FENCE-type
  case 0b0001111:
    d.op = FN;
    d.alu = NOP;
    if (ex(raw, 12, 15) != 0 || ex(raw, 7, 12) != 0 || ex(raw, 15, 20) != 0)
      d.op = INV;
    break;

  // JALR
  case 0b1100111:
    d.op = IC;
    d.rd = (u8)ex(raw, 7, 12);
    d.rs1 = (u8)ex(raw, 15, 20);
    d.imm = sx(ex(raw, 20, 32), 11);
    d.w = d.r1 = d.ij = d.ijr = 1;
    d.alu = ADD;
    break;

  // Store-Type
  case 0b0100011:
    d.op = S;
    d.rs1 = (u8)ex(raw, 15, 20);
    d.rs2 = (u8)ex(raw, 20, 25);
    d.imm = sx(ex(raw, 25, 32) << 5 | ex(raw, 7, 12), 11);
    d.r1 = d.r2 = d.is = 1;
    d.alu = ADD;
    switch (ex(raw, 12, 15)) {
    case 0b000:
      d.mem = SB;
      break;
    case 0b001:
      d.mem = SH;
      break;
    case 0b010:
      d.mem = SW;
      break;
    default:
      d.op = INV;
      break;
    }
    break;

  // B-type
  case 0b1100011:
    d.op = B;
    d.rs1 = (u8)ex(raw, 15, 20);
    d.rs2 = (u8)ex(raw, 20, 25);
    d.r1 = d.r2 = d.ib = 1;
    {
      u32 v = ex(raw, 31, 32) << 12;
      v |= ex(raw, 7, 8) << 11;
      v |= ex(raw, 25, 31) << 5;
      v |= ex(raw, 8, 12) << 1;
      d.imm = sx(v, 12);
    }
    switch (ex(raw, 12, 15)) {
    case 0b000:
      d.branch = EQ;
      break;
    case 0b001:
      d.branch = NE;
      break;
    case 0b100:
      d.branch = LT;
      break;
    case 0b101:
      d.branch = GE;
      break;
    case 0b110:
      d.branch = LTU;
      break;
    case 0b111:
      d.branch = GEU;
      break;
    default:
      d.op = INV;
      break;
    }
    break;

  // AUIPC-type
  case 0b0010111:
    d.op = U;
    d.rd = (u8)ex(raw, 7, 12);
    d.imm = (i32)(ex(raw, 12, 32) << 12);
    d.w = 1;
    d.alu = ADD;
    break;

  // LUI-type
  case 0b0110111:
    d.op = U;
    d.rd = (u8)ex(raw, 7, 12);
    d.imm = (i32)(ex(raw, 12, 32) << 12);
    d.w = 1;
    d.alu = NOP;
    break;

  // JAL-type
  case 0b1101111:
    d.op = J;
    d.rd = (u8)ex(raw, 7, 12);
    {
      u32 v = ex(raw, 31, 32) << 20;
      v |= ex(raw, 12, 20) << 12;
      v |= ex(raw, 20, 21) << 11;
      v |= ex(raw, 21, 31) << 1;
      d.imm = sx(v, 20);
    }
    d.w = d.ij = 1;
    d.alu = ADD;
    break;

  // INV
  default:
    d.op = INV;
    break;
  }
  return d;
}
} // namespace sim::dec