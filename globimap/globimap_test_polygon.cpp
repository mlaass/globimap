#include "globimap_test_config.hpp"
#include "globimap_v2.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>

#include <highfive/H5File.hpp>
#include <tqdm.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include "rasterizer.hpp"
#include "shapefile.hpp"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
typedef bg::model::polygon<point_t> polygon_t;

typedef std::vector<polygon_t> poly_collection_t;

const std::string base_path = "/mnt/G/datasets/atlas/";
const std::string vector_base_path = "/mnt/G/datasets/vector/";
const std::string experiments_path = "../experiments/";
// const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
// const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

std::vector<std::string> datasets{"twitter_200mio_coords.h5",
                                  "asia_200mio_coords.h5"};
std::vector<std::string> polygon_sets{
    "tl_2017_us_zcta510/tl_2017_us_zcta510",
    "Global_LSIB_Polygons_Detailed/Global_LSIB_Polygons_Detailed"};

std::vector<polygon_t> get_polygons(const std::string &filename) {
  std::vector<polygon_t> res;
  importSHP(filename, [&](SHPObject *shp, int start, int end) {
    if (shp->nSHPType == SHPT_POLYGON) {
      polygon_t poly;
      for (auto i = start; i < end; ++i) {
        bg::append(poly.outer(), point_t(shp->padfX[i], shp->padfY[i]));
      }
      res.push_back(poly);
    }
  });
  return res;
}
std::vector<std::pair<int, int>> rasterize_polygon(const polygon_t &poly) {
  std::vector<std::pair<int, int>> res;
  rasterize::Rasterizer<point_t> rasta;
  rasta.rasterize(poly, [&](double x, double y) { res.push_back({x, y}); });

  return res;
}

static std::string test_polygon(globimap::Globimap<> &g,
                                const std::string &name, const std::string &ds,
                                const std::string &shapefile, uint width,
                                uint height, bool errord) {
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

  auto polys = get_polygons(shapefile);

  // TODO transform polys
  // TODO rasterize polys
  // TODO convert raster to hashfn collection
  // TODO query hashfn and compare results with results from collection

  std::stringstream ss;
  ss << "{\"summary\":" << g.summary() << ",\n";
  ss << "\"insert_time\": " << insert_time.count() / 1000.0 << "}\n";

  return ss.str();
}

int main() {
  std::vector<std::vector<globimap::LayerConfig>> cfgs;
  get_configurations(cfgs);

  {
    uint k = 8;
    auto x = 0;
    uint width = 8192, height = 8192;
    std::string exp_name = "test_polygons";
    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : cfgs) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      auto y = 0;
      for (auto ds : datasets) {
        for (auto shp : polygon_sets) {
          std::stringstream fss;
          fss << experiments_path << exp_name << "/" << exp_name << ".w"
              << width << "h" << height << "." << std::setw(4)
              << std::setfill('0') << x << "." << y << "-" << fc.to_string()
              << ds << "." << shp << ".json";
          if (file_exists(fss.str())) {
            std::cout << "file already exists: " << fss.str() << std::endl;
          } else {
            std::cout << "run: " << fss.str() << std::endl;
            std::ofstream out(fss.str());
            auto g = globimap::Globimap(fc, true);
            out << test_polygon(g, fc.to_string(), ds, shp, width, height,
                                true);
            out.close();
          }
        }
        y++;
      }
      x++;
    }
  }
};
