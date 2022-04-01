#include "counting_globimap.hpp"
#include "globimap_test_config.hpp"
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

#include "loc.hpp"
#include "rasterizer.hpp"
#include "shapefile.hpp"

#include "archive.h"
#include <H5Cpp.h>

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

void make_rasterized_poly_ds_(const std::vector<polygon_t> &polys,
                              const std::string &filename) {
  H5::H5File file(filename, H5F_ACC_TRUNC);
  const hsize_t n_dims = 1;
  const hsize_t n_rows = polys.size();
  H5::DataSpace dataspace(n_dims, &n_rows);

  // target dtype for the file
  H5::IntType item_type(H5::PredType::INTEL_U64);
  H5::VarLenType file_type(item_type);

  // dtype of the generated data
  H5::IntType mem_item_type(H5::PredType::INTEL_U64);
  H5::VarLenType mem_type(item_type);

  H5::DataSet dataset = file.createDataSet("raster", file_type, dataspace);

  std::vector<std::vector<uint64_t>> data;
  data.reserve(n_rows);

  // this structure stores length of each varlen row and a pointer to
  // the actual data
  std::vector<hvl_t> varlen_spec(n_rows);

  std::cout << "polygons: " << polys.size() << " ..." << std::endl;
  auto P = tq::tqdm(polys);
  P.set_prefix("raster check for polygons ");
  hsize_t idx = 0;
  for (const auto &p : P) {
    auto raster = rasterize_polygon(p);
    auto size = raster.size();
    data.push_back(raster);

    varlen_spec.at(idx).len = size;
    varlen_spec.at(idx).p = (void *)&data.at(idx).front();
    idx++;
  }
  std::cout << "write dataset " << filename << std::endl;
  dataset.write(&varlen_spec.front(), mem_type);
}
void make_rasterized_poly_ds(const std::vector<polygon_t> &polys,
                             const std::string &pathname) {

  const hsize_t n_dims = 1;
  const hsize_t n_rows = polys.size();

  std::vector<std::vector<uint64_t>> data;
  data.reserve(n_rows);

  std::cout << "polygons: " << polys.size() << " ..." << std::endl;
  auto P = tq::tqdm(polys);
  P.set_prefix("rasterize polygons ");

  mkdir((pathname).c_str(), 0777);
  uint idx = 0;
  for (const auto &p : P) {
    auto raster = rasterize_polygon(p);
    std::stringstream ss;
    ss << pathname << "/" << std::setw(8) << std::setfill('0') << idx;
    auto filename = ss.str();
    // std::cout << "\n\nwrite dataset " << filename << std::endl;
    std::ofstream ofile(filename, std::ios::binary);
    Archive<std::ofstream> a(ofile);
    a << raster;
    // std::cout << "close " << filename << std::endl;
    ofile.close();
    idx++;
  }
  std::cout << "end " << pathname << std::endl;
}
void make_rasterized_poly_ds__(const std::vector<polygon_t> &polys,
                               const std::string &filename) {

  const hsize_t n_dims = 1;
  const hsize_t n_rows = polys.size();

  std::vector<std::vector<uint64_t>> data;
  data.reserve(n_rows);

  std::cout << "polygons: " << polys.size() << " ..." << std::endl;
  auto P = tq::tqdm(polys);
  P.set_prefix("raster check for polygons ");
  uint idx = 0;
  for (const auto &p : P) {
    auto raster = rasterize_polygon(p);
    data.push_back(raster);
    idx++;
  }

  std::ofstream ofile(filename, std::ios::binary);
  Archive<std::ofstream> a(ofile);
  a << data;
  std::cout << "close " << filename << std::endl;
  ofile.close();
}

int main() {
  const uint max_i = 6;
  for (auto i = 0; i < max_i; i += 1) {
    uint mul = (uint)powl(2, i);
    uint width = mul * 2048, height = mul * 2048;
    std::cout << i << "/" << max_i << " -> " << mul << " -> " << width << "x"
              << height << std::endl;
    for (auto shp : polygon_sets) {
      std::string shp_short = shp.substr(shp.find("/"));
      shp = vector_base_path + shp;
      std::stringstream ss;
      ss << vector_base_path << shp_short << "-" << width << "x" << height;

      if (file_exists(ss.str())) {
        std::cout << "path already exists: " << ss.str() << std::endl;
      } else {

        std::cout << shp << "... " << std::endl;
        auto polys = get_polygons_trans(shp, width, height);
        std::cout << shp_short << ": " << polys.size() << std::endl;
        // << ".bin";
        make_rasterized_poly_ds(polys, ss.str());
        std::cout << "\n\ndone writing " << ss.str() << std::endl;
      }
    }
  }
};
