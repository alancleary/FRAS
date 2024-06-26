/* C++11 UniformRandomBitGenerator wrapper around xoroshift128+.
 * Original C code from: http://xoroshiro.di.unimi.it/xoroshiro128plus.c
 * C++11 code from: https://github.com/alexcoplan/xoroshiro128plus-cpp */
#ifndef INCLUDED_XOROSHIRO_XOROSHIRO128PLUS
#define INCLUDED_XOROSHIRO_XOROSHIRO128PLUS

#include <array>
#include <functional>
#include <random>

namespace xoroshiro {

struct xoroshiro128plus_engine
{

private:
  uint64_t state[2];

  static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
  }

public:
  using result_type = uint64_t;
  constexpr static result_type min() { return 0; }
  constexpr static result_type max() { return -1; }

  result_type operator()();
  void seed(std::function<uint32_t(void)>);
  void seed(const std::array<uint32_t, 4> &);
};

}

#endif
