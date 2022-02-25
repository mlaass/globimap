#ifndef GLOBIMAP_HPP_INC
#define GLOBIMAP_HPP_INC
#include "hashfn.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

#define THRESHOLD_1BIT 1
#define THRESHOLD_8BIT 0xff
#define THRESHOLD_16BIT 0xffff
#define THRESHOLD_32BIT 0xffffffff
#define THRESHOLD_64BIT 0xffffffffffffffff

// #define MAKE_PARALLEL 1

#ifdef MAKE_PARALLEL

#define PARA_FOR #pragma omp parallel for
#define PARA #pragma omp parallel
#define PARA_CRIT #pragma omp CRIT
#define PARA_ATOMIC #pragma omp atomic

#else

#define PARA_FOR ;
#define PARA ;
#define PARA_CRIT ;
#define PARA_ATOMIC ;

#endif

namespace globimap {
template <typename T>
static std::vector<double> make_histogram(std::vector<T> &values,
                                          const uint size = 1024) {
  std::sort(values.begin(), values.end());
  uint bucket_size = values.size() / size;
  uint rest = values.size() % size;
  uint step = size / rest;
  std::vector<double> res({0});
  res.resize(size);

  auto off = 0;
  for (auto i = 0; i < size; ++i) {
    double bucket = 0;
    uint b = bucket_size;
    for (auto x = 0; x < b; x++) {
      bucket += values[off + x];
    }
    off += b;
    if (bucket != 0)
      bucket /= (double)b;
    res[i] = bucket;
  }
  return res;
}

static const uint64_t H1 = 8589845122, H2 = 8465418721;

template <typename BITS1 = bool, typename BITS8 = uint8_t,
          typename BITS16 = uint16_t, typename BITS32 = uint32_t,
          typename BITS64 = uint64_t>
struct Layer {

  uint bits;
  uint64_t size;
  uint64_t mask;
  std::vector<BITS1> f1;
  std::vector<BITS8> f8;
  std::vector<BITS16> f16;
  std::vector<BITS32> f32;
  std::vector<BITS64> f64;

  void resize(uint64_t new_size) {
    size = new_size;
    switch (bits) {
    case 1:
      f1.resize(size);
      break;
    case 8:
      f8.resize(size);
      break;
    case 16:
      f16.resize(size);
      break;
    case 32:
      f32.resize(size);
      break;
    case 64:
      f64.resize(size);
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
  }
  template <typename T> T get(size_t i) const {
    switch (bits) {
    case 1:
      return static_cast<T>(f1[i]);
      break;
    case 8:
      return static_cast<T>(f8[i]);
      break;
    case 16:
      return static_cast<T>(f16[i]);
      break;
    case 32:
      return static_cast<T>(f32[i]);
      break;
    case 64:
      return static_cast<T>(f64[i]);
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
    return 1;
  }

  template <typename T> T put(size_t i, T value) {
    switch (bits) {
    case 1:
      f1[i] = static_cast<BITS1>(value);
      break;
    case 8:
      f8[i] = static_cast<BITS8>(value);
      break;
    case 16:
      f16[i] = static_cast<BITS16>(value);
      break;
    case 32:
      f32[i] = static_cast<BITS32>(value);
      break;
    case 64:
      f64[i] = static_cast<BITS64>(value);
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
  }
  void increment(size_t i) {
    switch (bits) {
    case 1:
      f1[i] = true;
      break;
    case 8:
      f8[i]++;
      break;
    case 16:
      f16[i]++;
      break;
    case 32:
      f32[i]++;
      break;
    case 64:
      f64[i]++;
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
  }
  bool threshold(size_t i) {
    switch (bits) {
    case 1:
      return f1[i] == THRESHOLD_1BIT;
      break;
    case 8:
      return f8[i] == THRESHOLD_8BIT;
      break;
    case 16:
      return f16[i] == THRESHOLD_16BIT;
      break;
    case 32:
      return f32[i] == THRESHOLD_32BIT;
      break;
    case 64:
      return f64[i] == THRESHOLD_64BIT;
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
      return false;
    }
  }
  uint64_t byte_size() {
    switch (bits) {
    case 1:
      return f1.size() / 8;
      break;
    case 8:
      return f8.size();
      break;
    case 16:
      return f16.size() * 2;
      break;
    case 32:
      return f32.size() * 4;
      break;
    case 64:
      return f64.size() * 8;
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
      return 0;
    }
  }
  std::vector<uint8_t> as_bytes() {
    switch (bits) {
    case 1:
      return as_bytes_1bit();
      break;
    case 8:
      return f8;
      break;
    case 16:
      return as_bytes_from<BITS16>(f16, 2);
      break;
    case 32:
      return as_bytes_from<BITS32>(f32, 4);
      break;
    case 64:
      return as_bytes_from<BITS64>(f64, 8);
      break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
  }
  std::vector<uint8_t> as_bytes_1bit() {
    std::vector<uint8_t> buf;
    uint8_t ch = 0;
    for (auto i = 0; i < f1.size(); i++) {
      auto bit = i % 8;
      ch |= (f1[i] << bit);
      if (bit == 7) {
        // full byte has been built.
        buf.push_back(ch);
        ch = 0;
      }
    }
    return buf;
  }
  template <typename T>
  std::vector<uint8_t> as_bytes_from(std::vector<T> f, size_t bytes) {
    std::vector<uint8_t> buf(f.size * bytes);
    std::vector<uint8_t> ch(bytes);
    for (auto i = 0; i < f.size(); i++) {
      memcpy(&ch[0], &f[i], bytes);
      std::copy(ch.begin(), ch.end(), std::back_inserter(buf));
    }
    return buf;
  }
  struct Stats {
    uint64_t zeros, min, max, sum;
  };

  Stats stats() {
    Stats s = {0};
    s.min = ULLONG_MAX;

    switch (bits) {
    case 1: {
#pragma omp parallel for
      for (size_t i = 0; i < f1.size(); i++) {
        PARA_ATOMIC
        s.sum += f1[i];
        s.min = std::min(s.min, (uint64_t)f1[i]);
        s.max = std::max(s.max, (uint64_t)f1[i]);
        if (f1[i] == 0)
          s.zeros++;
      }
    } break;
    case 8: {
#pragma omp parallel for
      for (size_t i = 0; i < f8.size(); i++) {
        PARA_ATOMIC
        s.sum += f8[i];
        s.min = std::min(s.min, (uint64_t)f8[i]);
        s.max = std::max(s.max, (uint64_t)f8[i]);
        if (f8[i] == 0)
          s.zeros++;
      }
    } break;
    case 16: {
#pragma omp parallel for
      for (size_t i = 0; i < f16.size(); i++) {
        PARA_ATOMIC
        s.sum += f16[i];
        s.min = std::min(s.min, (uint64_t)f16[i]);
        s.max = std::max(s.max, (uint64_t)f16[i]);
        if (f16[i] == 0)
          s.zeros++;
      }
    } break;
    case 32: {
#pragma omp parallel for
      for (size_t i = 0; i < f32.size(); i++) {
        PARA_ATOMIC
        s.sum += f32[i];
        s.min = std::min(s.min, (uint64_t)f32[i]);
        s.max = std::max(s.max, (uint64_t)f32[i]);
        if (f32[i] == 0)
          s.zeros++;
      }
    } break;
    case 64: {
#pragma omp parallel for
      for (size_t i = 0; i < f64.size(); i++) {
        PARA_ATOMIC
        s.sum += f64[i];
        s.min = std::min(s.min, (uint64_t)f64[i]);
        s.max = std::max(s.max, (uint64_t)f64[i]);
        if (f64[i] == 0)
          s.zeros++;
      }
    } break;
    default:
      assert(false); // bits needs to be 1,8,16,32 or 64
    }
    return s;
  }

  std::string summary() {

    auto s = stats();

    std::stringstream ss;
    ss << "{" << std::endl;
    ss << "\"bits:\": " << bits << "," << std::endl;
    ss << "\"size:\": " << size << "," << std::endl;
    ss << "\"byte_size:\": " << byte_size() << "," << std::endl;
    ss << "\"foz:\": " << (double)(s.zeros) / (double)size << "," << std::endl;
    ss << "\"zeros:\": " << s.zeros << "," << std::endl;
    ss << "\"sum:\": " << s.sum << "," << std::endl;
    ss << "\"min:\": " << s.min << "," << std::endl;
    ss << "\"max:\": " << s.max << "," << std::endl;
    ss << "\"mean:\": " << (double)s.sum / (double)size << std::endl;
    // ss << "\"eci\": " << errors.size() << std::endl;
    ss << "}";
    return ss.str();
  }
};

struct LayerConfig {
  uint bits;
  uint logsize;
};

struct FilterConfig {
  uint hash_k;
  std::vector<LayerConfig> layers;
  std::string to_string() {
    std::stringstream ss;
    ss << "k_" << hash_k;
    ss << "bits_";
    for (auto l1 : layers) {
      ss << l1.bits << ".";
    }
    ss << "logsize_";
    for (auto l2 : layers) {
      ss << l2.logsize << ".";
    }
    return ss.str();
  }
};
/***
 *
 ***/
template <typename BITS1 = bool, typename BITS8 = uint8_t,
          typename BITS16 = uint16_t, typename BITS32 = uint32_t,
          typename BITS64 = uint64_t>
struct Globimap {
  typedef std::pair<uint32_t, uint32_t> coord_t;

  typedef std::map<coord_t, uint32_t> coord_map_t;

  std::vector<Layer<BITS1, BITS8, BITS16, BITS32, BITS64>> layers;
  uint64_t hashcount;

  bool collect_input;
  coord_map_t errors;
  coord_map_t counter;
  double error_rate;
  FilterConfig config;

  Globimap(const FilterConfig &conf, bool collect = false)
      : collect_input(collect) {
    hashcount = conf.hash_k;
    config = conf;

    for (auto i = 0; i < conf.layers.size(); ++i) {
      assert(conf.layers[i].bits == 1 || conf.layers[i].bits == 8 ||
             conf.layers[i].bits == 16 || conf.layers[i].bits == 32 ||
             conf.layers[i].bits ==
                 64); // conf.layers[i].bits needs to be one of 1,8,16,32,64
      Layer l;
      l.bits = conf.layers[i].bits;
      l.mask = (static_cast<uint64_t>(1) << conf.layers[i].logsize) - 1;
      l.resize(l.mask + 1);

      layers.push_back(l);
    }
  }

  void put_all(const std::vector<uint64_t> &points) {
    for (auto p = 0; p < points.size(); p += 2) {
      putp(&points[p]);
    }
  }
  void put(const std::vector<uint64_t> &point) { putp(&point[0]); }
  void putp(const uint64_t *point) {
    uint64_t h1 = H1, h2 = H2;
    if (collect_input) {
      coord_t p = {point[0], point[1]};
      if (counter.count(p) == 0) {
        counter[p] = 1;
      } else {
        counter[p] += 1;
      }
    }
    hash(&point[0], 2, &h1, &h2);
    auto all_full = true;
    for (uint64_t i = 0; i < static_cast<uint64_t>(hashcount); i++) {
      for (auto &l : layers) {
        uint64_t k = (h1 + (i + 1) * h2) & l.mask;
        if (!l.threshold(k)) {
          PARA_CRIT
          l.increment(k);
          all_full = false;
          break;
        }
      }
    }
    // assert(!all_full); // insufficent size in filter configuration
  }

  bool get_bool(const std::vector<uint64_t> &point) {
    uint64_t h1 = H1, h2 = H2;
    hash(&point[0], 2, &h1, &h2);
    auto res = true;
    for (uint64_t i = 0; i < static_cast<uint64_t>(hashcount); i++) {
      uint64_t k = (h1 + (i + 1) * h2) & layers[0].mask;
      uint64_t v = layers[0].template get<uint64_t>(k);
      res = res && (v != 0);
    }
    return res;
  }

  template <typename RT> RT get_mean(const std::vector<uint64_t> &point) {
    uint64_t sum = 0;
    uint64_t h1 = H1, h2 = H2;

    hash(&point[0], 2, &h1, &h2);
    for (size_t i = 0; i < static_cast<size_t>(hashcount); i++) {
      for (auto &l : layers) {
        uint64_t k = (h1 + (i + 1) * h2) & l.mask;
        auto v = l.get(k);
        if (v == 0) {
          return 0;
        }
        sum += v;
        if (!l.threshold()) {
          break;
        }
      }
    }
    return (RT)sum / (RT)hashcount;
  }

  uint64_t get_min(const std::vector<uint64_t> &point) {

    uint64_t h1 = H1, h2 = H2;

    hash(&point[0], 2, &h1, &h2);
    uint64_t min_v = UINT64_MAX;
    for (auto i = 0; i < hashcount; i++) {
      uint64_t sum = 0;
      for (auto &l : layers) {
        uint64_t k = (h1 + (i + 1) * h2) & l.mask;
        auto v = l.template get<uint64_t>(k);
        if (v == 0) {
          return 0;
        }
        sum += v;
        if (!l.threshold(k)) {
          break;
        }
      }
      min_v = std::min(min_v, sum);
    }
    return min_v;
  }
  uint64_t get_min_hs(uint64_t h1, uint64_t h2) {

    uint64_t min_v = UINT64_MAX;
    for (auto i = 0; i < hashcount; i++) {
      uint64_t sum = 0;
      for (auto &l : layers) {
        uint64_t k = (h1 + (i + 1) * h2) & l.mask;
        auto v = l.template get<uint64_t>(k);
        if (v == 0) {
          return 0;
        }
        sum += v;
        if (!l.threshold(k)) {
          break;
        }
      }
      min_v = std::min(min_v, sum);
    }
    return min_v;
  }

  std::vector<uint64_t> to_hashfn(const std::vector<uint64_t> &point) {
    std::vector<uint64_t> res;
    res.resize(point.size());
#pragma omp parallel for
    for (auto i = 0; i < point.size() - 1; i += 2) {
      uint64_t h1 = H1, h2 = H2;
      hash(&point[i], 2, &h1, &h2);
      res[i] = h1;
      res[i + 1] = h2;
    }
    return res;
  }

  uint64_t get_sum_hashfn(const std::vector<uint64_t> &hashfn) {
    uint64_t sum = 0;
#pragma omp parallel for
    for (auto i = 0; i < hashfn.size() - 1; i += 2) {
#pragma omp atomic
      sum += get_min_hs(hashfn[i], hashfn[i + 1]);
    }
    return sum;
  }
  uint64_t get_sum_raster_collected(const std::vector<uint64_t> &raster) {
    uint64_t sum = 0;
#pragma omp parallel for
    for (auto i = 0; i < raster.size() - 1; i += 2) {
      coord_t p = {raster[i], raster[i + 1]};
      uint64_t v = 0;
      if (counter.count(p) != 0)
        v = counter[p];

#pragma omp atomic
      sum += v;
    }
    return sum;
  }

  uint64_t get_sum_masked(const Globimap &mask) {
    uint64_t sum = 0;
#pragma omp parallel for
    for (auto i = 0; i < layers[0].size; i++) {
      if (mask.layers[0].template get<bool>(i & mask.layers[0].mask)) {
        for (auto &l : layers) {
          auto k = i & l.mask;
          auto v = l.template get<uint64_t>(k);
#pragma omp atomic
          sum += v;
          if (!l.threshold(k)) {
            break;
          }
        }
      }
    }
    return sum / mask.hashcount;
  }

  uint64_t byte_size() {
    uint64_t s = 0;
    for (auto &l : layers) {
      s += l.byte_size();
    }
    return s;
  }

  void detect_errors(uint64_t x, uint64_t y, uint64_t width, uint64_t height) {
    if (counter.size() == 0) {
      return;
    }
    for (auto u = 0; u < width; u++) {
      for (auto v = 0; v < height; v++) {
        coord_t p = {x + u, y + v};
        if (counter.count(p) == 0) {

          if (get_bool({x + u, y + v})) {
            errors[p] = 1;
          }
        } else {
          auto m = (uint64_t)get_min({x + u, y + v});
          uint64_t d = std::abs((int64_t)m - (int64_t)counter[p]);

          if (d != 0) {
            errors[p] = d;
          }
        }
      }
    }
    counter.clear();
    error_rate = (double)errors.size() / (double)(width * height);
  }

  std::vector<uint64_t> error_magnitudes() {
    std::vector<uint64_t> err_mag;
    err_mag.reserve(errors.size());
    for (const auto &ep : errors) {
      err_mag.push_back(ep.second);
    }
    return err_mag;
  }

  std::string error_summary() {
    std::stringstream ss;
    ss << "{\n";
    ss << "\"unique_input\": " << counter.size() << ",\n";
    ss << "\"errors\": " << errors.size() << ",\n";
    ss << "\"error_rate\": " << error_rate << ",\n";

    auto emag = error_magnitudes();
    auto hist = make_histogram(emag);
    uint64_t mmin = UINT64_MAX;
    uint64_t mmax = 0;
    uint64_t sum = 0;
    for (auto e : emag) {
      mmin = std::min(e, mmin);
      mmax = std::max(e, mmax);
      sum += e;
    }

    double mmean = 0;
    if (emag.size() > 0)
      mmean = (double)sum / (double)emag.size();

    ss << "\"magnitude_min\": " << mmin << ",\n";
    ss << "\"magnitude_max\": " << mmax << ",\n";
    ss << "\"magnitude_sum\": " << sum << ",\n";
    ss << "\"magnitude_mean\": " << mmean << ",\n";
    ss << "\"histogram\": [";
    for (auto i = 0; i < hist.size(); ++i) {
      ss << hist[i] << ((i < (hist.size() - 1)) ? ", " : "");
    }
    ss << "]\n";

    // ss << "\"magnitudes\": [";
    // for (auto i = 0; i < emag.size(); ++i) {
    //   ss << emag[i] << ((i < (emag.size() - 1)) ? ", " : "");
    // }
    // ss << "]\n";

    ss << "\n}" << std::endl;
    return ss.str();
  }
  std::string summary_config() { return "{}"; }
  std::string summary() {
    std::stringstream ss;
    ss << "{\n";
    ss << "\"byte_size\": " << byte_size() << ",\n";
    ss << "\"kb_size\": " << (double)byte_size() / (double)1024 << ",\n";
    ss << "\"mb_size\": " << (double)byte_size() / (double)(1024 * 1024)
       << ",\n";
    ss << "\"collect_input\": " << (collect_input ? "true" : "false") << ",\n";
    if (collect_input) {
      ss << "\"error_summary\": " << error_summary() << ",\n";
    }
    ss << "\"layers\": [\n";

    for (auto i = 0; i < layers.size(); i++) {
      ss << layers[i].summary() << ((i == layers.size() - 1) ? "\n" : ",\n");
    }
    ss << "]\n}" << std::endl;

    return ss.str();
  }

  bool compaction() {
    // TODO look at max values of higher layer and see if they can be
    // collapsed by increasing bit depth of lower layers
    for (auto i = layers.end(); i != layers.begin(); i--) {
      if (i->stats().max == 0) {
        layers.erase(i);
        i = layers.end();
      }
    }
  }
};

} // namespace globimap

#endif
