#pragma once
#include "sim/types.h"
#include <cstdio>
#include <cstring>

namespace sim {
struct Mem {
  u8 m[MemorySize];

  static int hex_value(char ch) {
    if (ch >= '0' && ch <= '9')
      return ch - '0';
    if (ch >= 'a' && ch <= 'f')
      return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
      return ch - 'A' + 10;
    return -1;
  }

  static bool in_bounds(u32 address, u32 bytes) {
    return bytes <= MemorySize && address <= MemorySize - bytes;
  }

  void init_from_str(const char *image) {
    memset(m, 0, sizeof(m));
    u32 address = 0;
    const char *cursor = image;
    for (; cursor != '\0';) {
      if (*cursor == '@') {
        cursor++;
        u32 parsed = 0;
        bool flg = 0;
        for (; hex_value(*cursor) >= 0;) {
          parsed = (parsed << 4) | (static_cast<u32>(hex_value(*cursor)));
          flg = 1;
          cursor++;
        }
        if (flg)
          address = parsed;
        continue;
      }
      const int high = hex_value(cursor[0]);
      const int low = cursor[0] == '\0' ? -1 : hex_value(cursor[1]);
      if (high >= 0 && low >= 0) {
        if (in_bounds(address, 1)) {
          if (address < MemorySize)
            m[address] = static_cast<u8>((high << 4) | low);
          address++;
        }
        cursor += 2;
      } else {
        cursor++;
      }
    }
  }
  void load(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == nullptr) {
      memset(m, 0, sizeof(m));
      fprintf(stderr, "Failed to open file: %s\n", path);
      return;
    }
    load_stream(fp);
    fclose(fp);
  }
  void load_stream(FILE *fp) {
    memset(m, 0, sizeof(m));
    u32 address = 0;
    int cur = 0;
    for (;;) {
      cur = fgetc(fp);
      if (cur == EOF)
        break;
      if (cur == '@') {
        u32 parsed = 0;
        bool flg = 0;
        for (;;) {
          cur = fgetc(fp);
          if (cur == EOF || hex_value(static_cast<char>(cur)) < 0)
            break;
          parsed =
              parsed << 4 | static_cast<u32>(hex_value(static_cast<u8>(cur)));
          flg = 1;
        }
        if (flg)
          address = parsed;
        continue;
      }
      const int high = hex_value(static_cast<char>(cur));
      if (high < 0)
        continue;
      const int lookhead = fgetc(fp);
      if (lookhead == EOF)
        break;
      const int low = hex_value(static_cast<char>(lookhead));
      if (low < 0) {
        if (lookhead == '@')
          ungetc(lookhead, fp);
        continue;
      }
      if (address < MemorySize)
        m[address] = static_cast<u8>((high << 4) | low);
      address++;
    }
  }
  u32 dread(u32 address, u8 size, bool sign_extend) const {
    const u32 bytes = size == 2 ? 4u : size == 1 ? 2u : 1u;
    if (!in_bounds(address, bytes))
      return 0;
    u32 value = static_cast<u32>(m[address]);
    if (bytes >= 2) {
      value |= static_cast<u32>(m[address + 1]) << 8;
    }
    if (bytes == 4) {
      value |= static_cast<u32>(m[address + 2]) << 16;
      value |= static_cast<u32>(m[address + 3]) << 24;
    }
    if (sign_extend) {
      if (bytes == 1 && value & 0x80u)
        value |= 0xffffff00u;
      if (bytes == 2 && value & 0x8000u)
        value |= 0xffff0000u;
    }
    return value;
  }
  void dwrite(u32 address, u32 value, u8 size) {
    const u32 bytes = size == 2 ? 4u : size == 1 ? 2u : 1u;
    if (!in_bounds(address, bytes))
      return;
    m[address] = static_cast<u8>(value & 0xffu);
    if (bytes >= 2) {
      m[address + 1] = static_cast<u8>((value >> 8) & 0xffu);
    }
    if (bytes == 4) {
      m[address + 2] = static_cast<u8>((value >> 16) & 0xffu);
      m[address + 3] = static_cast<u8>((value >> 24) & 0xffu);
    }
  }
};
} // namespace sim