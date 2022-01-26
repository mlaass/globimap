
#ifndef GLOBIMAP_TEST_CONFIG
#define GLOBIMAP_TEST_CONFIG
#include "globimap_v2.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

inline bool file_exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

typedef std::vector<std::vector<globimap::LayerConfig>> config_t;
std::string to_string(std::vector<globimap::LayerConfig> conf) {
  std::stringstream ss;
  for (const globimap::LayerConfig &x : conf) {
    ss << x.bits << "." << x.logsize << ";";
  }
  return ss.str();
}

void get_configurations(config_t &cfgs,
                        const std::vector<uint> &sizes = {16, 20, 24, 28}) {

  std::vector<uint> bits{1, 8, 16, 32, 64};
  std::vector<uint> sz = sizes;

  std::vector<std::vector<uint>> sz_perms;
  std::cout << "make permutations ..." << std::endl;
  do {
    sz_perms.push_back(sz);
  } while (std::next_permutation(sz.begin(), sz.end()));

  std::cout << "make configurations ..." << sz_perms.size() << std::endl;

  auto si = 0;
  std::map<std::string, int> exist;

  for (auto &s : sz_perms) {
    std::cout << si << " : " << s[0] << ", " << std::endl;
    for (auto a = 0; a < 2; a++) {
      std::vector<globimap::LayerConfig> x = {};
      // std::cout << a << ", ";
      x.push_back({bits[a], s[0]});
      cfgs.push_back(x);
      for (auto b = a + 1; b < 5; b++) {
        // std::cout << b << ", ";
        x.push_back({bits[b], s[1]});
        cfgs.push_back(x);
        for (auto c = b + 1; c < 5; c++) {
          // std::cout << c << ", ";
          x.push_back({bits[c], s[2]});
          auto s = to_string(x);
          if (exist.count(s) == 0) {
            cfgs.push_back(x);
            exist[s] = 1;
            std::cout << s << std::endl;
          }
          x.pop_back();
        }
        x.pop_back();
      }
    }

    std::cout << "\n" << std::endl;
    si++;
  }

  std::cout << "created configurations: " << cfgs.size() << std::endl;
}

std::string configs_to_string(const config_t &cfgs) {
  std::cout << "write configurations ..." << std::endl;
  std::stringstream ss;

  ss << "{ \"config\":[" << std ::endl;
  int i = 0;
  for (auto &cfg : cfgs) {
    // ss << "\"cfg" << i << "\": ";
    ss << "[\n";
    auto l = 0;
    for (auto c : cfg) {
      ss << "{\"bits\": " << c.bits << ", \"logsize\": " << c.logsize << "}"
         << ((l == cfg.size() - 1) ? "" : ",\n");
      l++;
    }
    ss << "]" << ((i == cfgs.size() - 1) ? "" : ",\n");
    i++;
  }
  ss << "\n]}" << std ::endl;

  return ss.str();
}

void save_configs(const std::string &fn, const config_t &cfgs) {
  auto outc = std::ofstream(fn);
  outc << configs_to_string(cfgs);
  outc.close();
}
#endif