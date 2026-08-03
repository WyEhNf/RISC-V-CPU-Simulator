#pragma once
#include "sim/alu.h"
namespace sim {
struct MemoryRequest {
  bool valid, forwarded, store;
  u32 lsq_slot, address, forwarded_value;
  Tag tag;
  MF operation;
};

struct MemoryPipelineState {
  bool busy;
  u8 remaining;
  MemoryRequest request;
};
struct CommitOutput {
  bool valid, terminate, write_reg, store, memory_op, branch, mispredict,
      redirect;
  bool predicted_taken;
  Tag tag;
  Ins ins;
  u32 value, address, pc, redirect_pc, termination_value;
};
struct ExecuteOutput {
  bool valid, memory_op;
  u32 rs_slot, store_value;
  Tag tag;
  Ins ins;
  AR result;
};

struct MemoryOutput {
  bool valid, store;
  u32 lsq_slot, value, address;
  Tag tag;
  MF operation;
};
struct IssueOutput {
  bool valid, invalid_instruction, memory_op, predicted_taken, users_rs;
  u32 pc, raw, rob_slot, rs_slot, lsq_slot, next_pc;
  Ins ins;
  Tag tag, q1, q2;
  u32 v1, v2;
};
struct CycleWires {
  CommitOutput commit;
  ExecuteOutput execute;
  MemoryOutput memory;
  MemoryRequest memory_request;
  IssueOutput issue;
  bool execute_accpted, cdb_valid, fault_request;
  Tag cdb_tag;
  u32 cdb_value;
};
struct CDBoutput {
  bool valid,excute_accepted;
  Tag tag;
  u32 value;
};
inline bool squash_cycle(const CycleWires &wires) {
  return wires.commit.terminate || wires.commit.mispredict;
}
inline bool accept_issue(const CycleWires &wires) {
  return wires.issue.valid && !squash_cycle(wires);
}
} // namespace sim