#pragma once
#include "decoder.h"
#include "sim/alu.h"
#include "sim/cdb.h"
#include "sim/lsq.h"
#include "sim/memory.h"
#include "sim/module_io.h"
#include "sim/predictor.h"
#include "sim/regfile.h"
#include "sim/rob.h"
#include "sim/rs.h"
#include "sim/types.h"
#include <cstdio>


namespace sim {
struct Simulator {
  enum Module : u8 {
    ROB_MODULE,
    RS_MODULE,
    LSQ_MODULE,
    FRONTEND_MODULE,
    MEMORY_MODULE
  };
  static constexpr u8 MODULE_COUNT = 5;
  static constexpr u8 MEMORY_LATENCY = 3;

  Mem mem;
  CDBArbiter cdb;
  Predictor prd;
  RegState rf;
  RSState rs;
  LSQState lsq;
  MemoryPipelineState memory_pipe;

  u32 pc, out;
  bool frz, term, fault;
  u32 fault_pc, fault_raw;
  u64 clk;

  void reset() {
    cdb = CDBArbiter{};
    rf = RegState{};
    rs = RSState{};
    lsq = LSQState{};
    memory_pipe = MemoryPipelineState{};
    prd.reset();

    pc = 0;
    frz = 0;
    clk = 0;
    out = 0;
    fault = 0;
    term = 0;
    fault_pc = 0;
    fault_raw = 0;
  }

  Simulator() { reset(); }
  void init(const char *path) {
    mem.load(path);
    reset();
  }
  void init_str(const char *image) {
    mem.init_from_str(image);
    reset();
  }
  void init_stream(FILE *fp) {
    mem.load_stream(fp);
    reset();
  }

  static u8 memory_size(MF op) {
    if (op == MB || op == MBU || op == SB)
      return 0;
    if (op == MH || op == MHU || op == SH)
      return 1;
    return 2; // give out a code
  }

  static bool sign_extending_load(MF op) { return op == MB || op == MH; }

  static u32 extend_forwarded_value(u32 val, MF op) {
    if (op == MB)
      return (val & 0x80u) ? val | 0xffffff00u : val & 0xffu;
    if (op == MBU)
      return val & 0xffu;
    if (op == MH)
      return (val & 0x8000u) ? val | 0xffff0000u : val & 0xffffu;
    if (op == MHU)
      return val & 0xffffu;
    return val;
  }
  void resolve_operand(u32 reg, u32 value, Tag &dependency) const {
    rf.resolve_operand(reg, rs, value, dependency);
  }

  static MemoryOutput evaluate_memory(const MemoryPipelineState &ppl,
                                      const MemoryRequest &request,
                                      const Mem &mem_in) {
    MemoryOutput output{};
    if (!ppl.busy || ppl.remaining != 1)
      return output;
    const MemoryRequest &req = ppl.request;
    output.valid = 1;
    output.store = req.store;
    output.lsq_slot = req.lsq_slot;
    output.address = req.address;
    output.tag = req.tag;
    output.operation = req.operation;
    if (req.store)
      output.value = req.forwarded_value;
    else if (req.forwarded)
      output.value = extend_forwarded_value(req.forwarded_value, req.operation);
    else
      output.value = mem_in.dread(req.address, memory_size(req.operation),
                                  sign_extending_load(req.operation));
    return output;
  }

  static IssueOutput
  evaluate_frontend(i32 current_pc, bool frozen, bool terminated, bool failed,
                    const Mem &mem_in, const Predictor &prd_in,
                    const RegState &rf_in, const ROBState &rob_in,
                    const RSState &rs_in, const LSQState &lsq_in,
                    const MemoryPipelineState &ppl_in) {
    IssueOutput output{};
    if (frozen || terminated || failed)
      return output;
    if (rob_in.full())
      return output;

    const u32 raw = mem_in.ifetch(current_pc);
    const Ins ins = dec::decode(raw);

    if (ins.op == INV) {
      output.invalid_instruction = 1;
      output.pc = current_pc;
      output.raw = raw;
      return output;
    }

    if ((ins.il || ins.is) && lsq_in.full())
      return output;
    if (ins.op == FN && (lsq_in.head != lsq_in.tail || ppl_in.busy))
      return output;

    u32 rs_slot = RSN;
    if (ins.op != FN) {
      rs_slot = rs_in.find_free();
      if (rs_slot == RSN)
        return output;
    }
    output.valid=1;
    output.pc=current_pc;
    output.raw=raw;
    output.ins=ins;
    output.rob_slot=rob_in.tail;
    output.rs_slot=rs_slot;
    output.lsq_slot=lsq_in.tail;
    output.memory_op=ins.il||ins.is;
    output.uses_rs=ins.op!=FN;
    output.tag=rob_in.next_tag();

    if(ins.r1) rf_in.resolve_operand(ins.rs1, rob_in, output.v1, output.q1);
    if(ins.r2) rf_in.resolve_operand(ins.rs2, rob_in, output.v2, output.q2);

    if(ins.op==IA||ins.op==IS||ins.op==IM)
    {
        output.v2=static_cast<u32>(ins.imm);
        output.q2=Tag{};
    }

    if(ins.ib){
        const bool taken=(prd_in.predict(current_pc)&1)!=0;
        output.predicted_taken=taken;
        output.next_pc= taken ? static_cast<u32>(current_pc+ins.imm) : current_pc+4;
    }else if(ins.ij&&!ins.ijr){
        output.next_pc=static_cast<u32>(current_pc+ins.imm);
    }else{
        output.next_pc=current_pc+4;
    }   

    return output;
  }
};
} // namespace sim