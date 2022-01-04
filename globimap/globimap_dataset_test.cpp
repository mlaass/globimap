#include "globimap_v2.hpp"
#include <chrono>
#include <iostream>
#include <math.h>

#include <fstream>
#include <highfive/H5File.hpp>
#include <iostream>
#include <string>
#include <tqdm.hpp>
#include <tqdm/tqdm.h>

const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

std::vector<std::string> datasets{"asia-latest.h5",
                                  "asia_1bil_coords.h5",
                                  "asia_500mio_coords.h5",
                                  "europe-latest.h5",
                                  "osm_points.h5",
                                  "staypoints.h5",
                                  "twitter.h5",
                                  "twitter_100mio_coords.h5",
                                  "twitter_10mio_coords.h5",
                                  "twitter_1mio_coords.h5",
                                  "twitter_50mio_coords.h5",
                                  "twitter_coords.h5"};

void get_configurations(std::vector<std::vector<globimap::LayerConfig>> &cfgs,
                        const std::vector<uint> &sizes = {16, 24, 32}) {

  std::vector<uint> bits{1, 8, 16, 32, 64};
  std::vector<uint> sz = sizes;

  std::vector<std::vector<uint>> sz_perms;
  std::cout << "make permutations ..." << std::endl;
  do {
    sz_perms.push_back(sz);
  } while (std::next_permutation(sz.begin(), sz.end()));

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
}
std::string
configs_to_string(std::vector<std::vector<globimap::LayerConfig>> cfgs) {
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

static void test_h5_encode(globimap::Globimap<> &g, const std::string &name,
                           uint ds_index) {
  auto filename = base_path + datasets[ds_index];
  auto batch_size = 4096;
  std::cout << "Start test_h5 encode with {fn: \"" << filename
            << "\" } for: " << name << std::endl;
  using namespace HighFive;

  // we create a new hdf5 file
  auto file = File(filename, File::ReadWrite);
  std::vector<std::vector<double>> read_data;

  // we get the dataset
  DataSet dataset = file.getDataSet("coords");

  std::vector<std::vector<double>> result;
  auto shape = dataset.getDimensions();
  int batches = std::floor(shape[0] / batch_size);
  std::cout << "Start test_h5 encode with {fn: \"" << filename
            << "\" } for: " << name << "\nbatches: " << batches
            << "\nbatchsize: " << batch_size << std::endl;
  for (auto i : tq::trange(batches)) {
    dataset.select({i * batch_size, 0}, {batch_size, 2}).read(result);

    for (auto p : result) {
      g.put({(uint64_t)p[0], (uint64_t)p[1]});
    }
  }
  std::cout << "End!" << std::endl;
  std::cout << g.summary() << std::endl;
}

static std::string test_cos(globimap::Globimap<> &g, const std::string &name,
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

  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> insert_time = t2 - t1;
  duration<double, std::milli> errord_time = t3 - t2;
  std::stringstream ss;

  ss << "{\"summary\":" << g.summary() << ",\n";
  ss << "\"insert_time\": " << insert_time.count() / 1000.0 << ",\n";
  ss << "\"errord_time\": " << errord_time.count() / 1000.0 << "}" << std::endl;
  return ss.str();
};

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
    std::stringstream fss;
    fss << experiments_path << "test_cos.w8192h8192l8192." << std::setw(4)
        << std::setfill('0') << x << "-" << fc.to_string() << "json";
    std::cout << "run: " << fss.str() << std::endl;
    std::ofstream out(fss.str());
    auto g = globimap::Globimap(fc, true);
    out << test_cos(g, fc.to_string(), 8192, 8192, 8192);
    out.close();
    x++;
  }
};

int main1() {

  std::cout << "1 BIT: " << THRESHOLD_1BIT << ", 8 BIT: " << THRESHOLD_8BIT
            << ", 16 BIT: " << THRESHOLD_16BIT
            << ", 32 BIT: " << THRESHOLD_32BIT
            << ", 64 BIT: " << THRESHOLD_64BIT << std::endl;
  std::cout << "Start! Tests" << std::endl;

  // auto g1 = globimap::Globimap(8, {1, 8, 16}, {16, 16, 16});
  // auto g2 = globimap::Globimap(8, {8, 32}, {16, 16});
  // auto g3 = globimap::Globimap(8, {1, 8, 32}, {16, 16, 16}, true);

  // std::cout << "**********************************************" << std::endl;
  // test_cos(g1, "G1", pow(2, 12), pow(2, 12), pow(2, 24));
  // std::cout << "**********************************************" << std::endl;
  // test_cos(g2, "G2", pow(2, 12), pow(2, 12), pow(2, 24));
  // std::cout << "**********************************************" << std::endl;
  // test_cos(g3, "G3", pow(2, 12), pow(2, 12), pow(2, 24));
  // std::cout << "**********************************************" << std::endl;
}
