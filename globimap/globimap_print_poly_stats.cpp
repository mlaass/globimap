#include "counting_globimap.hpp"
#include "globimap_test_config.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <string>

#include <highfive/H5File.hpp>
#include <tqdm.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include "archive.h"
#include "loc.hpp"
#include "rasterizer.hpp"
#include "shapefile.hpp"

#include <H5Cpp.h>

namespace fs = std::filesystem;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgt = boost::geometry::strategy::transform;
typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
typedef bg::model::box<point_t> box_t;
typedef bg::model::polygon<point_t> polygon_t;

typedef std::vector<polygon_t> poly_collection_t;

#ifndef TUM1_ICAML_ORG
const std::string base_path = "/mnt/G/datasets/atlas/";
const std::string vector_base_path = "/mnt/G/datasets/vector/";
const std::string experiments_path =
    "/home/moritz/workspace/bgdm/globimap/experiments/";
#else
const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";
const std::string vector_base_path = "/home/moritz/tf/vector/";
#endif

std::vector<std::string> datasets{"twitter_1mio_coords.h5",
                                  "twitter_10mio_coords.h5"};

// std::vector<std::string> datasets{"twitter_200mio_coords.h5",
//                                   "asia_200mio_coords.h5"};
std::vector<std::string> polygon_sets{"tl_2017_us_zcta510",
                                      "Global_LSIB_Polygons_Detailed"};
template <typename T> std::string render_hist(std::vector<T> hist) {
  std::stringstream ss;
  ss << "[";
  for (auto i = 0; i < hist.size(); ++i) {
    ss << hist[i] << ((i < (hist.size() - 1)) ? ", " : "");
  }
  ss << "]";
  return ss.str();
}

template <typename T>
std::string render_stat(const std::string &name, std::vector<T> stat) {
  double stat_min = FLT_MAX;
  double stat_max = 0;
  double stat_mean = 0;
  double stat_std = 0;

  for (double v : stat) {
    stat_min = std::min(stat_min, v);
    stat_max = std::max(stat_max, v);
    stat_mean += v;
  }
  stat_mean /= stat.size();

  for (double v : stat) {
    stat_std += pow((stat_mean - v), 2);
  }
  stat_std /= stat.size();
  stat_std = sqrt(stat_std);

  std::stringstream ss;
  ss << "\"" << name << "\": {\n";
  ss << "\"min\": " << stat_min << ",\n";
  ss << "\"max\": " << stat_max << ",\n";
  ss << "\"mean\": " << stat_mean << ",\n";
  ss << "\"std\": " << stat_std << ",\n";
  ss << "\"hist\": " << render_hist(globimap::make_histogram(stat, 1000))
     << "\n";
  ss << "}";
  return ss.str();
}

int main() {
  uint width = 2 * 8192, height = 2 * 8192;
  std::string exp_name = "polygons_stat";
  mkdir((experiments_path + exp_name).c_str(), 0777);
  for (auto shp : polygon_sets) {
    std::stringstream ss1;
    ss1 << shp << "-" << width << "x" << height;
    auto polyset_name = ss1.str();
    std::stringstream ss;
    ss << vector_base_path << polyset_name;
    auto poly_path = ss.str();

    int poly_count = 0;
    for (auto e : fs::directory_iterator(poly_path)) {
      poly_count += (e.is_regular_file() ? 1 : 0);
    }
    std::vector<uint64_t> polysizes;
    for (auto idx = 0; idx < poly_count; idx++) {
      std::vector<uint64_t> raster;
      std::stringstream ss;
      ss << poly_path << "/" << std::setw(8) << std::setfill('0') << idx;
      auto filename = ss.str();
      std::ifstream ifile(filename, std::ios::binary);
      if (!ifile.is_open()) {
        std::cout << "ERROR file doesn't exist: " << filename << std::endl;
      }
      Archive<std::ifstream> a(ifile);
      a >> raster;
      polysizes.push_back(raster.size());
      ifile.close();
    }
    std::stringstream fss;
    fss << experiments_path << exp_name << "/" << polyset_name << ".json";
    std::ofstream out(fss.str());
    out << "{" << render_stat("polysizes", polysizes) << ",\n";
    out << "\"size\": " << poly_count << "" << std::endl;
    out << "}" << std::endl;

    out.close();
    std::cout << "end: " << shp << std::endl;
  }
};
