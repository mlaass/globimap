#include "globimap_v2.hpp"
#include <chrono>
#include <iostream>
#include <math.h>

static inline void test1(globimap::Globimap<> &g) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  auto t1 = high_resolution_clock::now();
  auto limit = pow(2, 24);

  for (auto i = 0; i < limit; i++) {
    uint64_t x = rand();
    std::vector<uint64_t> v{x, (uint64_t)(((double)0xffffff00) * sin(x))};
    g.put(v);
  }

  auto t2 = high_resolution_clock::now();
  std::cout << "End!" << std::endl;
  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> ms_double = t2 - t1;

  std::cout << ms_double.count() / 1000.0 << "S\n";
};

int main() {

  std::cout << "Start!" << std::endl;

  auto g1 = globimap::Globimap(8, {1, 8, 16}, {16, 16, 16});
  auto g2 = globimap::Globimap(8, {8, 32}, {16, 16});
  auto g3 = globimap::Globimap(8, {1, 8, 32}, {16, 16, 16});
  test1(g1);
  test1(g2);
  test1(g3);

  return 0;
}