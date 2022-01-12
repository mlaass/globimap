#include "globimap_test_config.hpp"
#include "globimap_v2.hpp"
#include <chrono>
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

#include "rasterizer.hpp"
#include "shapefile.hpp"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgt = boost::geometry::strategy::transform;
typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
typedef bg::model::box<point_t> box_t;
typedef bg::model::polygon<point_t> polygon_t;

typedef std::vector<polygon_t> poly_collection_t;

const std::string base_path = "/mnt/G/datasets/atlas/";
const std::string vector_base_path = "/mnt/G/datasets/vector/";
const std::string experiments_path =
    "/home/moritz/workspace/bgdm/globimap/experiments/";
// const std::string base_path = "/home/moritz/tf/pointclouds_2d/data/";
// const std::string experiments_path = "/home/moritz/tf/globimap/experiments/";

std::vector<std::string> datasets{"twitter_1mio_coords.h5",
                                  "twitter_10mio_coords.h5"};

// std::vector<std::string> datasets{"twitter_200mio_coords.h5",
//                                   "asia_200mio_coords.h5"};
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
std::vector<uint64_t> rasterize_polygon(const polygon_t &poly) {
  std::vector<uint64_t> res;
  rasterize::Rasterizer<point_t> rasta;

  rasta.rasterize(poly, [&](double x, double y) {
    res.push_back((uint64_t)x);
    res.push_back((uint64_t)y);
  });
  return res;
}

std::vector<polygon_t> get_polygons_trans(const std::string &shapefile,
                                          double width, double height) {
  auto polys = get_polygons(shapefile);

  for (auto &poly : polys) {
    bgt::translate_transformer<double, 2, 2> translate(180, 90);
    bgt::scale_transformer<double, 2, 2> scale(1 / 360.0, 1 / 180.0);
    bgt::scale_transformer<double, 2, 2> scale2(width, height);

    bgt::matrix_transformer<double, 2, 2> translateScaleScale2(
        scale2.matrix() * scale.matrix() * translate.matrix());

    polygon_t p3 = poly;
    bg::transform(p3, poly, translateScaleScale2);
  }

  // TODO transform polys
  double min_x = std::numeric_limits<double>::max(),
         min_y = std::numeric_limits<double>::max(),
         max_x = std::numeric_limits<double>::min(),
         max_y = std::numeric_limits<double>::min();

  for (auto poly : polys) {
    box_t box;
    bg::envelope(poly, box);

    min_x = std::min(box.min_corner().get<0>(), min_x);
    min_y = std::min(box.min_corner().get<1>(), min_y);

    max_x = std::max(box.max_corner().get<0>(), max_x);
    max_y = std::max(box.max_corner().get<1>(), max_y);
  }
  std::cout << " min_x: " << min_x << " min_y: " << min_y << " max_x: " << max_x
            << " max_y: " << max_y << std::endl;
  return polys;
}

std::string test_polys(globimap::Globimap<> &g,
                       const std::vector<polygon_t> &polys) {
  std::vector<uint32_t> errors;
  std::vector<uint32_t> sizes;
  std::vector<double> errors_pc;
  double sum_pc = 0, sum = 0, sum_sz = 0;
  int n = 0;
  std::cout << "polygons: " << polys.size() << " ..." << std::endl;
  auto P = tq::tqdm(polys);
  P.set_prefix("raster check for polygons ");
  for (const auto &p : P) {
    // for (const auto &p : polys) {
    auto raster = rasterize_polygon(p);
    // std::cout << n << " -> " << raster.size() << " : " << std::endl;
    if (raster.size() > 0) {
      auto hsfn = g.to_hashfn(raster);
      auto res_raster = g.get_sum_raster_collected(raster);
      auto res_hashfn = g.get_sum_hashfn(raster);
      uint64_t err = std::abs((int64_t)res_hashfn - (int64_t)res_raster);
      // std::cout << res_raster << "  :" << res_hashfn << " : " << err << " : "
      //           << raster.size() / 2 << std::endl;
      errors.push_back(err);
      double err_pc = ((double)err) / ((double)raster.size() / 2);
      errors_pc.push_back(err_pc);
      sizes.push_back(raster.size() / 2);
      sum_pc += err_pc;
      sum += err;
      sum_sz += raster.size() / 2;
      n++;
    }
  }
  auto hist_pc = globimap::make_histogram<double>(errors_pc, 128);
  auto hist = globimap::make_histogram<uint32_t>(errors, 128);
  auto hist_sz = globimap::make_histogram<uint32_t>(sizes, 128);

  auto err_mean_pc = sum_pc / (double)n;
  auto err_mean = sum / (double)n;
  auto size_mean = sum_sz / (double)n;
  std::stringstream ss;
  ss << "{\n";
  ss << "\"polygons\": " << n << ",\n";
  ss << "\"sum\": " << sum << ",\n";
  ss << "\"sum_pc\": " << sum_pc << ",\n";
  ss << "\"sum_sz\": " << sum_sz << ",\n";
  ss << "\"err_mean\": " << err_mean << ",\n";
  ss << "\"err_mean_pc\": " << err_mean_pc << ",\n";
  ss << "\"histogram_pc\": [";
  for (auto i = 0; i < hist_pc.size(); ++i) {
    ss << hist_pc[i] << ((i < (hist_pc.size() - 1)) ? ", " : "");
  }
  ss << "],\n";
  ss << "\"histogram_sz\": [";
  for (auto i = 0; i < hist_sz.size(); ++i) {
    ss << hist_sz[i] << ((i < (hist_sz.size() - 1)) ? ", " : "");
  }
  ss << "],\n";
  ss << "\"histogram\": [";
  for (auto i = 0; i < hist.size(); ++i) {
    ss << hist[i] << ((i < (hist.size() - 1)) ? ", " : "");
  }
  ss << "]\n";
  ss << "\n}" << std::endl;
  std::cout << ss.str();
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
  get_configurations(cfgs);
  // uint width = 8192, height = 8192;
  // for (auto shp : polygon_sets) {
  //   shp = vector_base_path + shp;
  //   std::cout << shp << "... " << std::endl;
  //   auto p = get_polygons_trans(shp, width, height);
  //   std::cout << shp << ": " << p.size() << std::endl;
  // }

  // return 0;
  {
    uint k = 8;
    auto x = 0;
    uint width = 16 * 8192, height = 16 * 8192;
    std::string exp_name = "test_polygons";
    mkdir((experiments_path + exp_name).c_str(), 0777);
    for (auto c : {cfgs[32]}) {
      globimap::FilterConfig fc{k, c};
      std::cout << "fc: " << fc.to_string() << std::endl;
      auto y = 0;
      for (auto shp : polygon_sets) {
        shp = vector_base_path + shp;
        std::cout << shp << "... " << std::endl;
        auto polys = get_polygons_trans(shp, width, height);
        std::cout << shp << ": " << polys.size() << std::endl;

        for (auto ds : datasets) {
          std::stringstream fss;
          std::size_t found = shp.find_last_of("/\\");
          auto shapef = shp.substr(found + 1);
          fss << experiments_path << exp_name << "/" << exp_name << ".w"
              << width << "h" << height << "." << std::setw(4)
              << std::setfill('0') << x << "." << y << "-" << fc.to_string()
              << ds << "." << shapef << ".json";
          if (file_exists(fss.str())) {
            std::cout << "file already exists: " << fss.str() << std::endl;
          } else {
            std::cout << "run: " << fss.str() << std::endl;
            std::ofstream out(fss.str());
            auto g = globimap::Globimap(fc, true);
            encode_dataset(g, fc.to_string(), ds, width, height);
            std::cout << "encoding done!" << std::endl;
            std::cout << "test:" << shp << std::endl;
            out << test_polys(g, polys);
            out.close();
          }
        }
        y++;
      }
      x++;
    }
  }
};
