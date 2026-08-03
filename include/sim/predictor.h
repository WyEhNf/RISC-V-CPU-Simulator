#pragma once
#include "types.h"
#include <cstdio>
namespace sim {
struct Predictor {
  i32 t2[BHTN][4];
  u8 h[BHTN];
  i32 l[BHTN];
  i32 g;
  u32 tot, hit;
  void reset() {
    for (u32 i = 0; i < BHTN; i++) {
      h[i] = 0;
      for (int j = 0; j < 4; ++j)
        t2[i][j] = 2;
      l[i] = 2;
    }
    g = 2;
    tot = hit = 0;
  }
  u8 predict(u32 pc) const {
    u16 ix = (u16)(pc & 0xFFF);
    u8 tp = t2[ix][h[ix]] < 3 ? 1 : 0;
    return (u8)((l[ix] < 3 ? 1 : 0) << 2 | (g < 3 ? 1 : 0) << 1 | tp);
  }
  void update(u32 pc, bool taken, u8 pred) {
    tot++;
    if (taken == (pred & 1))
      ++hit;
    u16 ix = (u16)(pc & 0xFFF);
    auto sat = [](i32 &x, bool y) {
      if (y) {
        if (x > 1)
          --x;
      } else if (x < 4)
        ++x;
    };
    sat(l[ix], taken);
    sat(g, taken);
    sat(t2[ix][h[ix]], taken);
    h[ix] = (u8)((h[ix] << 1 | taken) & 3);
  }
  void report() const {
    fprintf(stderr, "Predict %u/%u (%.2f%%)\n", hit, tot,
            tot ? 100.0 * hit / tot : 0.0);
  }
};

} // namespace sim