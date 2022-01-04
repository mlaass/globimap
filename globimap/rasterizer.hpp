#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include <algorithm>
#include <boost/geometry/geometries/geometries.hpp>
#include <limits>

namespace detail {

namespace bg = boost::geometry;

/*  rasterize a polygon implementation  */

template <typename point_t, typename float_t = double, typename int_t = int64_t>
struct Edge {
  typedef bg::model::segment<point_t> segment_t;
  float_t x, xNorm, yNorm;
  float_t minX, maxX;
  float_t slope;
  int_t yMax, yMin; // min-max of edge
  float_t dX, dY;
  segment_t line;
  Edge(point_t &a, point_t &b) : line{a, b} {
    auto &pMin = (bg::get<1>(a) < bg::get<1>(b) ? a : b);
    auto &pMax = (bg::get<1>(a) < bg::get<1>(b) ? b : a);

    yMax = bg::get<1>(pMax);
    yMin = bg::get<1>(pMin);

    x = bg::get<0>(pMin);                          // start x
    minX = std::min(bg::get<0>(a), bg::get<0>(b)); // start x
    maxX = std::max(bg::get<0>(a), bg::get<0>(b)); // start x

    dX = (bg::get<0>(pMax) - bg::get<0>(pMin));
    dY = (bg::get<1>(pMax) - bg::get<1>(pMin));
    slope = dY != 0 ? slope = dX / dY : 0;

    auto yOff = yMin - std::floor(yMin);
    auto xOff = x - std::floor(x);

    yNorm = std::round(yMin) + 0.5;
    xNorm = x + (yNorm - yMin) * slope;
  }

  friend std::ostream &operator<<(std::ostream &stream, const Edge &edge) {
    return std::cout << "Edge bucket: " << edge.yMin << "=>" << edge.yMax
                     << "dx=" << edge.dX << "; dy=" << edge.dY
                     << "; x=" << edge.x << ";" << std::endl;
  }
};

template <typename point_t, typename float_t = double> struct Rasterizer {
  typedef bg::model::polygon<point_t> polygon_t;
  typedef Edge<point_t, float_t> edge_t;
  struct {
    bool operator()(const edge_t &a, const edge_t &b) const {
      return a.yMin < b.yMin;
    }
  } yMinCompare;

  enum { STEP_INTERSECT, STEP_RASTERIZE };
  std::list<edge_t> ET; // edge table
  std::list<edge_t> AL; // active list

  int scanline;

  std::pair<bool, int> custom_scanline;

  void set_scanline(int _scanline) {
    custom_scanline = std::make_pair(true, _scanline);
  }

  void clear_scanline(int _scanline) {
    custom_scanline = std::make_pair(false, 0);
  }

  void init(polygon_t &p) {
    if (p.outer().size() < 3) // don't render invalid polygons
      return;
    // build edge table containing all edges n=>0, 0=>1, ...n-1=>n
    edge_t e = {p.outer().back(), p.outer()[0]};
    if (e.dY != 0)
      ET.push_back(e);
    for (size_t i = 0; i < p.outer().size() - 1; i++) {
      e = {p.outer()[i], p.outer()[i + 1]};
      if (e.dY != 0)
        ET.push_back(e);
    }
    // sort according to yMin
    ET.sort(yMinCompare);

    //	std::cout << bg::wkt(p) << std::endl;
    if (ET.empty())
      return;

    if (!custom_scanline.first) {
      scanline = ET.front().yMin;
    } else {
      auto _scanline = custom_scanline.second;
      if (ET.front().yMin != _scanline)
        //	    std::cout << "Starting at different scanlines: " <<
        // ET.front().yMin << " vs." << scanline << std::endl;
        scanline = ET.front().yMin;
      //	    std::cout << "silent step should now do " << scanline << "
      // until " << _scanline << std::endl;
      while (scanline < _scanline) {
        //	      std::cout << "silent step:" << scanline << " towards " <<
        //_scanline << std::endl;
        step_intersect([](int x, int y) {});
      }
    }
  }

  bool done() { return (ET.empty() && AL.empty()); }
  template <typename func> void step_intersect(func putpixel) {
#ifdef DEBUG_LOG
    std::cout << "scanline(" << scanline << "):" << std::endl;
#endif
    // all edges that start here are moved from ET to AL
    for (auto it = ET.begin(); it != ET.end();) {
      if (std::floor(it->yMin - 1) <= scanline) {
        AL.push_back(*it);
#ifdef DEBUG_LOG
        std::cout << "erase (" << it->yMin << ") ";
#endif
        it = ET.erase(it);
      } else
        it++;
    }
#ifdef DEBUG_LOG
    std::cout << "active(" << AL.size() << ") ";
#endif
    // all edges that end here, are removed from AL
    for (auto it = AL.begin(); it != AL.end();) {
      if (std::ceil(it->yMax + 1) < scanline) {
        it = AL.erase(it);
      } else
        it++;
    }

    // theoretic: for multipolygons, this could become empty. However, for
    // polygons it should always have at least two active edges
    if (AL.empty()) {
      scanline++;
      return;
    }

    // sort according to X coordinate
    AL.sort([](const edge_t &a, const edge_t &b) {
      // it is an iterator pointing to Edge
      // sort by x and slope
      return (a.x < b.x || (a.x == b.x && a.slope < b.slope));
    });
#ifdef DEBUG_LOG
    std::cout << "-> (" << AL.size() << "):" << std::endl;
    for (auto &a : AL) {
      std::cout << "{(" << a.line.first.get<0>() << "," << a.line.first.get<1>()
                << "),"
                << "(" << a.line.second.get<0>() << ","
                << a.line.second.get<1>() << ")},";
    }
#endif

    // prepare scanline filling
    auto minX = std::floor(AL.front().minX - 1);
    auto maxX = std::ceil(AL.back().maxX);
    auto line = typename edge_t::segment_t(point_t(minX, scanline + 0.5),
                                           point_t(maxX, scanline + 0.5));

    std::vector<point> output;
    for (auto &e : AL) {
      bg::intersection(line, e.line, output);
#ifdef DEBUG_LOG
      std::cout << "[";
      for (auto i = output.begin(); i != output.end(); ++i)
        std::cout << "(" << bg::get<0>(*i) << ',' << bg::get<1>(*i) << "),";
      std::cout << "], ";
#endif
    }
#ifdef DEBUG_LOG
    std::cout << " | sum: " << output.size() << std::endl;
#endif
    // assert(output.size() % 2 == 0);
    std::sort(output.begin(), output.end(),
              ([](const point &a, const point &b) {
                // sort by x
                return bg::get<0>(a) < bg::get<0>(b);
              }));
#ifdef BORDERS_ONLY
    // Just the borders
    auto index = 0;
    for (auto &i : output) {
      //	float_t pixelX = bg::get<0>(i) - std::floor(bg::get<0>(i));

      int x = index % 2 == 0 ? std::round(bg::get<0>(i))
                             : std::floor(bg::get<0>(i) - 0.5);

      putpixel(x, scanline);
      index++;
    }
#else
    if (output.size() > 0) {

      for (auto i = 1; i < output.size(); i += 2) {
        auto xstart = std::round(bg::get<0>(output[i - 1]));
        auto xend = std::floor(bg::get<0>(output[i]) - 0.5);
#ifdef DEBUG_LOG
        std::cout << "xstart: " << xstart << " xend: " << xend << std::endl;
#endif
        for (auto x = xstart; x <= xend; x++)
          putpixel(x, scanline);
      }
    }
#endif

    scanline++;
    return;
  }
  template <typename func> void step_rasterize(func putpixel) {
//#define DEBUG_LOG
#ifdef DEBUG_LOG
    std::cout << "scanline(" << scanline << "):" << std::endl;
#endif
    // all edges that start here are moved from ET to AL
    for (auto it = ET.begin(); it != ET.end();) {
      if (std::floor(it->yNorm) <= scanline) {
        AL.push_back(*it);
#ifdef DEBUG_LOG
        std::cout << "erase (" << it->yMin << ") ";
#endif
        it = ET.erase(it);
      } else
        it++;
    }
#ifdef DEBUG_LOG
    std::cout << "active(" << AL.size() << ") ";
#endif
    // all edges that end here, are removed from AL
    for (auto it = AL.begin(); it != AL.end();) {
      if (std::ceil(it->yMax - 1) < scanline) {
        it = AL.erase(it);
      } else
        it++;
    }

    // theoretic: for multipolygons, this could become empty. However, for
    // polygons it should always have at least two active edges
    if (AL.empty()) {
      scanline++;
      return;
    }

    // sort according to X coordinate
    AL.sort([](float_t a, float_t b) {
      // it is an iterator pointing to Edge
      // sort by x and slope
      return ((a.xNorm < b.xNorm) || (a.xNorm == b.xNorm && a.slope < b.slope));
    });
#ifdef DEBUG_LOG
    std::cout << "-> (" << AL.size() << "):" << std::endl;
    for (auto &a : AL) {
      std::cout << "{(" << a.line.first.get<0>() << "," << a.line.first.get<1>()
                << "),"
                << "(" << a.line.second.get<0>() << ","
                << a.line.second.get<1>() << ")},";
    }
#endif

    // prepare scanline filling
    auto minX = std::floor(AL.front().minX - 2);
    auto maxX = std::ceil(AL.back().maxX);
    auto line = edge_t::segment_t(point_t(minX, scanline + 0.5),
                                  point_t(maxX, scanline + 0.5));
    auto it = AL.begin();
    bool inside = false;
#ifdef DEBUG_LOG
    std::cout << "[" << std::endl;
#endif
    for (float_t x = minX; x <= maxX; x++) {
      auto go = true;
      while (go) {
        go = false;
        if (!inside) {
          if (x + 0.5 >= it->xNorm && it != AL.end()) {
            inside = !inside;
            it++;
            go = true;
          }
        } else {
          if ((x + 0.499999) > it->xNorm) {
#ifdef DEBUG_LOG
            std::cout.precision(std::numeric_limits<float_t>::max_digits10);
            std::cout << std::endl
                      << "(" << x + 0.5 << " > " << it->xNorm << ")"
                      << std::endl;
            std::cout.precision(2);
#endif
            inside = !inside;
            it++;
            go = true;
          }
        }
        if (it == AL.end())
          break;
      }
#ifdef DEBUG_LOG
      std::cout << "n:" << it->xNorm << "~" << it->x << " x:" << x
                << " in:" << inside << ", ";
#endif

      if (inside)
        putpixel((int)x, scanline);
    }
#ifdef DEBUG_LOG
    std::cout << "]" << std::endl;
#endif

    // increment all the X based on slope
    for (auto &e : AL) {
      if (e.dX != 0)
        e.xNorm += e.slope;
    }

    scanline++;
    return;
  }

  template <typename func>
  void rasterize(polygon_t &p, func putpixel, int step_id = STEP_RASTERIZE,
                 int steps = -1) {
    init(p);

    switch (step_id) {
    case STEP_INTERSECT:
      while (!done()) {
        step_intersect(putpixel);
      }
      break;
    case STEP_RASTERIZE:
      while (!done()) {
        step_rasterize(putpixel);
      }
      break;
    }
    // Process ET=>AL=>(void)
  }
};

} // namespace detail
using detail::Rasterizer;

#endif
