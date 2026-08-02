#pragma once
#include <cstdint>
#include <cstring>

namespace sim {
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;

// RN: Register Number
// RBN: Reload Buffer Number
// RSN: Reservaton Station Number
// LSN : Load-Store Queue Number
// BHTN: Branch History Table Number
// TERM: Stopper.
constexpr u32 MemorySize = 1 << 18, RN = 32, RBN = 32, RSN = 32, LSN = 32,
              BHTN = 1 << 12, TERM = 0x0ff00513u;
enum Op : u8 { R, IA, IS, IM, IC, S, B, U, J, FN, INV };
enum AF : u8 { ADD, SUB, AND, OR, XOR, SLL, SRL,SRA,SLT, SLTU, NOP };
enum BF : u8 { EQ, NE, LT, LTU, GE, GEU, NOBR };
enum MF : u8 { MW, MH, MHU, MV, MBU,SW, SH, SB, NOM };
enum TK : u8 { TN, TROB, TRS, TLSQ };

struct Tag {
  TK k = TN;
  u32 i = 0;
  u32 generation = 0;
  constexpr bool isvalid() const { return k != TN; }
  friend constexpr bool operator==(const Tag &a, const Tag &b) {
    return a.k == b.k && a.i == b.i && a.generation == b.generation;
  }
  friend constexpr bool operator!=(const Tag &a, const Tag &b) {
    return !(a == b);
  }
};

struct Ins {
  u32 raw = 0;
  Op op;
  AF alu;
  BF branch;
  MF mem;
  u8 rd, rs1, rs2;
  i32 imm;
  bool w, r1, r2, ib, ij, ijr, il, is;
};

} // namespace sim