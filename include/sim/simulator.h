#pragma once
#include "sim/memory.h"
#include "sim/regfile.h"
#include "sim/rob.h"
#include "sim/rs.h"
#include "sim/lsq.h"
#include "sim/predictor.h"
#include "sim/module_io.h"
#include "sim/alu.h"
#include "sim/cdb.h"
#include "sim/types.h"
#include  <cstdio>

namespace sim
{
struct Simulator
{
    enum Module: u8{
        ROB_MODULE,RS_MODULE,LSQ_MODULE,FRONTEND_MODULE,MEMORY_MODULE
    };
    static constexpr u8 MODULE_COUNT=5;
    static constexpr u8 MEMORY_LATENCY=3;

    Mem mem;
    CDBArbiter cdb;
    Predictor prd;
    RegState rf;
    RSState rs;
    LSQState lsq;
    MemoryPipelineState memory_pipe;

    u32 pc, out;
    bool frz,term,fault;
    u32 fault_pc,fault_raw;
    u64 clk;

    void reset()
    {
        cdb=CDBArbiter{};
        rf=RegState{};
        rs=RSState{};
        lsq=LSQState{};
        memory_pipe=MemoryPipelineState{};
        prd.reset();

        pc=0; frz=0; clk=0; out=0;
        fault=0; term=0; fault_pc=0; fault_raw=0;
    }

    Simulator() {reset();}
    void init(const char *path){
        mem.load(path);
        reset();
    }
    void init_str(const char *image){
        mem.init_from_str(image);
        reset();
    }
    void init_stream(FILE *fp){
        mem.load_stream(fp);
        reset();
    }
};

} // namespace sim