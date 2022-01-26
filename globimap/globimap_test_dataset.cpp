#include "globimap_v2.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>

#include <highfive/H5File.hpp>
#include <tqdm.hpp>
#include <tqdm/tqdm.h>

#include "globimap_test_config.hpp"

const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

std::vector<std::string> datasets{
    "twitter_1mio_coords.h5",   "twitter_10mio_coords.h5",
    "twitter_100mio_coords.h5", "twitter_50mio_coords.h5",
    "twitter_200mio_coords.h5", "asia_500mio_coords.h5",
    "asia_1bil_coords.h5"};

static std::string test_encode(globimap::Globimap<> &g, const std::string &name,
                               const std::string &ds, uint width, uint height,
                               bool errord) {
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
}

int main() {
  config_t cfgs;
  get_configurations(cfgs);

  {
    uint k = 8;
    auto x = 0;
    uint width = 8192, height = 8192;
    std::string exp_name = "test_datasets_with_errord";
    save_configs(experiments_path + std::string("config_") + exp_name, cfgs);
    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      auto y = 0;
      for (auto d : datasets) {
        std::stringstream fss;
        fss << experiments_path << exp_name << "/" << exp_name << ".w" << width
            << "h" << height << "." << std::setw(4) << std::setfill('0') << x
            << "." << y << "-" << fc.to_string() << d << ".json";
        if (file_exists(fss.str())) {
          std::cout << "file already exists: " << fss.str() << std::endl;
        } else {
          std::cout << "run: " << fss.str() << std::endl;
          std::ofstream out(fss.str());
          auto g = globimap::Globimap(fc, true);
          out << test_encode(g, fc.to_string(), d, width, height, true);
          out.close();
        }
        y++;
      }
      x++;
    }
  }
  {
    uint k = 8;
    auto x = 0;
    uint width = 8192, height = 8192;
    std::string exp_name = "test_datasets";
    save_configs(experiments_path + std::string("config_") + exp_name, cfgs);
    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      auto y = 0;
      for (auto d : datasets) {
        std::stringstream fss;
        fss << experiments_path << exp_name << "/" << exp_name << ".w" << width
            << "h" << height << "." << std::setw(4) << std::setfill('0') << x
            << "." << y << "-" << fc.to_string() << d << ".json";
        if (file_exists(fss.str())) {
          std::cout << "file already exists: " << fss.str() << std::endl;
        } else {
          std::cout << "run: " << fss.str() << std::endl;
          std::ofstream out(fss.str());
          auto g = globimap::Globimap(fc, false);
          out << test_encode(g, fc.to_string(), d, width, height, false);
          out.close();
        }
        y++;
      }
      x++;
    }
  }
};
