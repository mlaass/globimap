#include "globimap/counting_globimap.hpp"
#include <chrono>
#include <iostream>
#include <math.h>

#include <fstream>
#include <highfive/H5File.hpp>
#include <iostream>
#include <string>
#include <tqdm.hpp>
#include <tqdm/tqdm.h>

#include "globimap_test_config.hpp"
#include <sys/stat.h>
#include <unistd.h>

const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

static std::string test_cos(globimap::CountingGloBiMap<> &g,
                            const std::string &name, uint width, uint height,
                            uint limit, bool errord) {
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
  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> insert_time = t2 - t1;
  std::stringstream ss;

  if (errord) {

    auto t3 = high_resolution_clock::now();
    g.detect_errors(0, 0, width, height);
    auto t4 = high_resolution_clock::now();

    /* Getting number of milliseconds as a double. */
    duration<double, std::milli> errord_time = t4 - t3;

    ss << "{\"summary\":" << g.summary() << ",\n";
    ss << "\"insert_time\": " << insert_time.count() / 1000.0 << ",\n";
    ss << "\"errord_time\": " << errord_time.count() / 1000.0 << "}"
       << std::endl;
  } else {

    ss << "{\"summary\":" << g.summary() << ",\n";
    ss << "\"insert_time\": " << insert_time.count() / 1000.0 << "\n}"
       << std::endl;
  }
  return ss.str();
};

int main() {
  std::vector<std::vector<globimap::LayerConfig>> cfgs;
  get_configurations(cfgs, {16, 20, 24, 28}, {1, 8, 16, 32, 64});

  // outc << configs_to_string(cfgs);
  // outc.close();
  {
    uint k = 8;
    auto x = 0;
    std::string exp_name = "test_cos_with_errord";
    save_configs(experiments_path + std::string("config_") + exp_name, cfgs);

    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      std::stringstream fss;
      fss << experiments_path << exp_name << "/" << exp_name
          << ".w8192h8192l65536." << std::setw(4) << std::setfill('0') << x
          << "-" << fc.to_string() << "json";
      if (file_exists(fss.str())) {
        std::cout << "file already exists: " << fss.str() << std::endl;
      } else {
        std::cout << "run: " << fss.str() << std::endl;
        std::ofstream out(fss.str());
        auto g = globimap::CountingGloBiMap(fc, true);
        out << test_cos(g, fc.to_string(), 8192, 8192, 65536, true);
        out.close();
        x++;
      }
    }
  }
  {
    uint k = 8;
    auto x = 0;
    std::string exp_name = "test_cos";
    save_configs(experiments_path + std::string("config_") + exp_name, cfgs);

    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      std::stringstream fss;
      fss << experiments_path << exp_name << "/" << exp_name
          << ".w8192h8192l65536." << std::setw(4) << std::setfill('0') << x
          << "-" << fc.to_string() << "json";
      if (file_exists(fss.str())) {
        std::cout << "file already exists: " << fss.str() << std::endl;
        break;
      }
      std::cout << "run: " << fss.str() << std::endl;
      std::ofstream out(fss.str());
      auto g = globimap::CountingGloBiMap(fc, false);
      out << test_cos(g, fc.to_string(), 8192, 8192, 65536, false);
      out.close();
      x++;
    }
  }
};
