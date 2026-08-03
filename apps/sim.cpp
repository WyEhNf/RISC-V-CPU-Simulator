#include "sim/simulator.h"
#include <cstdio>
#include <cstring>
int main(int argc, char **argv) {
  sim::Simulator sim;
  const char *path = nullptr;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--input") && i + 1 < argc) {
      path = argv[++i];
    }
  }
  if (path)
    sim.init(path);
  else
    sim.init_stream(stdin);
  for (; sim.clk < 2e8;) {
    sim.cycle();
    if (sim.done()) {
      if (sim.failed())
        fprintf(stderr, "Simulator fault at PC=0x%08X, raw=0x%08X\n",
                sim.fault_pc, sim.fault_raw);
      else
        fprintf(stderr, "Simulator terminated with output 0x%08X\n",
                sim.get_output());
      return sim.failed() ? 1 : 0;
    }
  }
  fprintf(stderr, "Simulator exceeded max cycles\n");
  return 1;
}