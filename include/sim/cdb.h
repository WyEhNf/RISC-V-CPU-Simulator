#pragma once
#include "sim/module_io.h"
namespace sim
{
struct CDBArbiter
{
  CDBOutput evaluate(const ExecuteOutput &execute, const MemoryOutput &memory) const 
  {
    CDBOutput output{};
   if(memory.valid&&!memory.store){
    output.valid=1;
    output.tag=memory.tag;
    output.value=memory.value;
   }

   if(execute.valid){
    if(execute.memory_op){
        output.execute_accepted=1;
    }else if(!output.valid){
        output.execute_accepted=1;
        output.valid=1;
        output.tag=execute.tag;
        output.value=execute.result.v;
    }
   }
   return output;
  }
};
}