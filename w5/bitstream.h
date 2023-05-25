#pragma once

#include <cstring>
#include <cstdint>

class Bitstream {
  public:
    Bitstream(uint8_t* data) : ptr(data) {}

    template<typename T>
    void write(const T& val) {
      memcpy(ptr + offset, reinterpret_cast<const uint8_t*>(&val), sizeof(T));
      offset += sizeof(T);
    }

    template<typename T>
    void read(T& val) {
      memcpy(reinterpret_cast<uint8_t*>(&val), ptr + offset, sizeof(T));
      offset += sizeof(T);
    }

  private:
    uint8_t* ptr;
    uint32_t offset = 0;
};