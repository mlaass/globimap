#include "globimap_test_config.hpp"
#include "globimap_v2.hpp"
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

std::string
test_polys_mask(globimap::Globimap<> &g, size_t poly_size,
                std::function<std::vector<uint64_t>(size_t)> poly_gen) {
  std::vector<uint32_t> errors_mask;
  std::vector<uint32_t> errors_hash;
  std::vector<uint32_t> diff_mask_hash;
  std::vector<uint32_t> sizes;
  std::vector<double> errors_mask_pc;
  std::vector<double> errors_hash_pc;
  double sum_pc = 0, sum = 0, sum_sz = 0;
  int n = 0;
  std::cout << "polygons: " << poly_size << " ..." << std::endl;
  auto P = tq::trange(poly_size);
  P.set_prefix("mask for polygons ");
  for (const auto &idx : P) {
    auto raster = poly_gen(idx);

    if (raster.size() > 0) {
      auto hsfn = g.to_hashfn(raster);

      auto mask_conf = globimap::FilterConfig{
          g.config.hash_k, {{1, g.config.layers[0].logsize}}};
      auto mask = globimap::Globimap(mask_conf, false);
      mask.put_all(raster);

      auto res_raster = g.get_sum_raster_collected(raster);
      auto res_mask = g.get_sum_masked(mask);
      auto res_hashfn = g.get_sum_hashfn(raster);
      uint64_t err_mask = std::abs((int64_t)res_mask - (int64_t)res_raster);
      uint64_t err_hash = std::abs((int64_t)res_hashfn - (int64_t)res_raster);
      uint64_t hash_mask_diff =
          std::abs((int64_t)res_mask - (int64_t)res_hashfn);

      errors_mask.push_back(err_mask);
      errors_hash.push_back(err_hash);
      double err_mask_pc = ((double)err_mask) / ((double)raster.size() / 2);
      double err_hash_pc = ((double)err_hash) / ((double)raster.size() / 2);
      errors_mask_pc.push_back(err_mask_pc);
      errors_hash_pc.push_back(err_hash_pc);
      sizes.push_back(raster.size() / 2);

      n++;
    }
  }

  std::stringstream ss;
  ss << "{\n";
  ss << "\"polygons\": " << n << ",\n";
  ss << render_stat("errors_mask_pc", errors_mask_pc) << ",\n";
  ss << render_stat("errors_hash_pc", errors_hash_pc) << ",\n";
  ss << render_stat("errors_mask", errors_mask) << ",\n";
  ss << render_stat("errors_hash", errors_hash) << ",\n";
  ss << render_stat("sizes", sizes) << "\n";

  ss << "\n}" << std::endl;
  // std::cout << ss.str();
  return ss.str();
}

static void encode_dataset(globimap::Globimap<> &g, const std::string &name,
                           const std::string &ds, uint width, uint height) {
  auto filename = base_path + ds;
  auto batch_size = 4096;
  std::cout << "Start test_h5 encode with {fn: \"" << filename
            << "\" } for: " << name << std::endl;
  using namespace HighFive;

  // we create a new hdf5 file
  auto file = File(filename, File::ReadWrite);
  std::vector<std::vector<double>> read_data;

  // we get the dataset
  DataSet dataset = file.getDataSet("coords");

  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  auto t1 = high_resolution_clock::now();

  std::vector<std::vector<double>> result;
  auto shape = dataset.getDimensions();
  int batches = std::floor(shape[0] / batch_size);
  std::cout << "Start test_h5 encode with {fn: \"" << filename
            << "\" } for: " << name << "\nbatches: " << batches
            << "\nbatchsize: " << batch_size << std::endl;
  for (auto i : tq::trange(batches)) {
    dataset.select({i * batch_size, 0}, {batch_size, 2}).read(result);

    for (auto p : result) {
      double x = (double)width * (((double)p[0] + 180.0) / 360.0);
      double y = (double)height * (((double)p[0] + 90.0) / 180.0);
      g.put({(uint64_t)x, (uint64_t)y});
    }
  }
  auto t2 = high_resolution_clock::now();
  duration<double, std::milli> insert_time = t2 - t1;
}

int main() {
  std::vector<std::vector<globimap::LayerConfig>> cfgs;
  get_configurations(cfgs, {16, 20, 24, 28}, {1, 8, 16, 32, 64});

  {
    uint k = 8;
    auto x = 0;
    uint width = 2 * 8192, height = 2 * 8192;
    std::string exp_name = "test_polygons_mask";
    save_configs(experiments_path + std::string("config_") + exp_name, cfgs);
    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      auto y = 0;
      for (auto shp : polygon_sets) {
        std::stringstream ss1;
        ss1 << shp << "-" << width << "x" << height;
        auto polyset_name = ss1.str();
        std::stringstream ss;
        ss << vector_base_path << polyset_name;
        auto poly_path = ss.str();

        int poly_count = 0;
        for (auto e : fs::directory_iterator(poly_path))
          poly_count += (e.is_regular_file() ? 1 : 0);

        for (auto ds : datasets) {
          std::stringstream fss;

          fss << experiments_path << exp_name << "/" << exp_name << ".w"
              << width << "h" << height << "." << fc.to_string() << ds << "."
              << polyset_name << ".json";
          if (file_exists(fss.str())) {
            std::cout << "file already exists: " << fss.str() << std::endl;
          } else {
            std::cout << "run: " << fss.str() << std::endl;
            std::ofstream out(fss.str());
            auto g = globimap::Globimap(fc, true);
            encode_dataset(g, fc.to_string(), ds, width, height);
            std::cout << "encoding done!" << std::endl;
            std::cout << "test: " << polyset_name << std::endl;
            out << test_polys_mask(g, poly_count, [&](int idx) {
              std::vector<uint64_t> raster;
              std::stringstream ss;
              ss << poly_path << "/" << std::setw(8) << std::setfill('0')
                 << idx;
              auto filename = ss.str();
              std::ifstream ifile(filename, std::ios::binary);
              if (!ifile.is_open()) {
                std::cout << "ERROR file doesn't exist: " << filename
                          << std::endl;
                return raster;
              }
              Archive<std::ifstream> a(ifile);
              a >> raster;
              ifile.close();
              return raster;
            });
            out.close();
          }
        }
        y++;
      }
      x++;
    }
  }
};
