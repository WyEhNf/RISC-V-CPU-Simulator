#pragma once
#include "sim/module_io.h"
namespace sim
{
struct CBDArbiter
{
  CDBoutput evaluate(const ExecuteOutput &execute, const MemoryOutput &memory) const 
  {
    CDBoutput output{};
   if(memory.valid&&!memory.store){
    output.valid=1;
    output.tag=memory.tag;
    output.value=memory.value;
   }

   if(execute.valid){
    if(execute.memory_op){
        output.excute_accepted=1;
    }else if(!output.valid){
        output.excute_accepted=1;
        output.valid=1;
        output.tag=execute.tag;
        output.value=execute.result.v;
    }
   }
   return output;
  }
};
}