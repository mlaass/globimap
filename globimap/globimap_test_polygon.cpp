#include "globimap_v2.hpp"
#include <chrono>
#include <iostream>
#include <math.h>

#include <fstream>
#include <highfive/H5File.hpp>
#include <iostream>
#include <string>
#include <tqdm.hpp>

const std::string base_path = "/mnt/G/datasets/atlas/";
const std::string experiments_path = "../experiments/";
// const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
// const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

std::vector<std::string> datasets{"twitter_200mio_coords.h5",
                                  "asia_200mio_coords.h5"};

int main() {
  std::vector<std::vector<globimap::LayerConfig>> cfgs;
  get_configurations(cfgs);

  // outc << configs_to_string(cfgs);
  // outc.close();

  uint k = 8;
  auto x = 0;
  for (auto c : cfgs) {
    globimap::FilterConfig fc{k, c};
    std::cout << "fc: " << fc.to_string() << std::endl;

    x++;
  }
};
