#include "globimap_v2.hpp"
#include <chrono>
#include <iostream>
#include <math.h>

static void test_cos(globimap::Globimap<> &g, const std::string &name,
                     uint width, uint height, uint limit) {
  std::cout << "Start cos with {w: " << width << ", h: " << height
            << ", limit: " << limit << " } for: " << name << std::endl;
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  auto t1 = high_resolution_clock::now();

  for (auto i = 0; i < limit; i++) {
    uint64_t x = rand() % width;
    double y = (double)x / (double)width;
    y = (double)height * (1 + 0.5 * cos(y * M_2_PI));
    std::vector<uint64_t> v{x, (uint64_t)y};
    g.put(v);
  }

  auto t2 = high_resolution_clock::now();
  g.detect_errors(0, 0, width, height);
  auto t3 = high_resolution_clock::now();

  std::cout << "End!" << std::endl;
  std::cout << g.summary() << std::endl;

  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> insert_time = t2 - t1;
  duration<double, std::milli> errord_time = t3 - t2;

  std::cout << "insert_time: " << insert_time.count() / 1000.0 << "S\n";
  std::cout << "errord_time: " << errord_time.count() / 1000.0 << "S"
            << std::endl;
};

int main() {

  std::cout << "Start! Tests" << std::endl;

  auto g1 = globimap::Globimap(8, {1, 8, 16}, {16, 16, 16});
  auto g2 = globimap::Globimap(8, {8, 32}, {16, 16});
  auto g3 = globimap::Globimap(8, {1, 8, 32}, {16, 16, 16}, true);

  std::cout << "**********************************************" << std::endl;
  test_cos(g1, "G1", pow(2, 12), pow(2, 12), pow(2, 24));
  std::cout << "**********************************************" << std::endl;
  test_cos(g2, "G2", pow(2, 12), pow(2, 12), pow(2, 24));
  std::cout << "**********************************************" << std::endl;
  test_cos(g3, "G3", pow(2, 12), pow(2, 12), pow(2, 24));
  std::cout << "**********************************************" << std::endl;

  std::vector<uint> bits{1, 8, 16, 32, 64};
  std::vector<uint> sz{16, 24, 32};
  std::vector<std::vector<uint>> sz_perms;
  std::cout << "make permutations ..." << std::endl;
  do {
    sz_perms.push_back(sz);
  } while (std::next_permutation(sz.begin(), sz.end()));

  std::vector<std::vector<globimap::LayerConfig>> cfgs;

  std::cout << "make configurations ..." << sz_perms.size() << std::endl;

  auto si = 0;
  for (auto &s : sz_perms) {
    std::cout << si << " : " << s[0] << ", ";
    for (auto a = 0; a < 2; a++) {
      std::vector<globimap::LayerConfig> x = {};
      std::cout << a << ", ";
      x.push_back({bits[a], s[0]});
      cfgs.push_back(x);
      for (auto b = a + 1; b < 5; b++) {
        std::cout << b << ", ";
        x.push_back({bits[b], s[1]});
        cfgs.push_back(x);
        for (auto c = b + 1; c < 5; c++) {
          std::cout << c << ", ";
          x.push_back({bits[c], s[2]});
          cfgs.push_back(x);
          x.pop_back();
        }
        x.pop_back();
      }
    }

    std::cout << "\n" << std::endl;
    si++;
  }
  std::cout << "print configurations ..." << std::endl;

  int i = 0;
  for (auto &cfg : cfgs) {
    std::cout << "\"cfg" << i << "\": [\n";
    auto l = 0;
    for (auto c : cfg) {
      std::cout << "{\"bits\": " << c.bits << ", \"logsize\": " << c.logsize
                << "}," << std::endl;
      l++;
    }
    std::cout << "],\n" << std ::endl;

    i++;
  }

  return 0;
}