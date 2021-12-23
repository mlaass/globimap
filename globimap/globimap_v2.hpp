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
  template <typename T> T get(size_t i) {
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
      PARA_FOR
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
      PARA_FOR
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
      PARA_FOR
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
      PARA_FOR
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
      PARA_FOR
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

struct pair_hash {
  inline std::size_t operator()(const std::pair<uint64_t, uint64_t> &v) const {
    return v.first * H1 + v.second * H2;
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

template <typename BITS1 = bool, typename BITS8 = uint8_t,
          typename BITS16 = uint16_t, typename BITS32 = uint32_t,
          typename BITS64 = uint64_t>
struct Globimap {
  typedef std::unordered_set<std::pair<uint64_t, uint64_t>, pair_hash>
      coord_set_t;

  std::vector<Layer<BITS1, BITS8, BITS16, BITS32, BITS64>> layers;
  coord_set_t unique_input, errors;
  uint64_t hashcount;
  double error_rate;
  bool collect_input;

  Globimap(const FilterConfig &conf, bool collect = false)
      : collect_input(collect) {
    hashcount = conf.hash_k;

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

  void put(std::vector<uint64_t> a) {
    uint64_t h1 = H1, h2 = H2;
    if (collect_input)
      unique_input.insert({a[0], a[1]});
    hash(&a[0], 2, &h1, &h2);
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
    assert(!all_full); // insufficent size in filter configuration
  }

  void detect_errors(uint64_t x, uint64_t y, uint64_t width, uint64_t height) {
    if (unique_input.size() == 0) {
      return;
    }

    for (auto u = 0; u < width; u++) {
      for (auto v = 0; v < height; v++) {
        if (!unique_input.count({x + u, y + v}) > 0) {
          if (get_bool({x + u, y + v})) {
            errors.insert({x + u, y + v});
          }
        }
      }
    }
    error_rate = (double)errors.size() / (double)(width * height);
  }

  bool get_bool(const std::vector<uint64_t> &a) {
    uint64_t h1 = H1, h2 = H2;
    hash(&a[0], 2, &h1, &h2);
    auto res = true;
    for (uint64_t i = 0; i < static_cast<uint64_t>(hashcount); i++) {
      uint64_t k = (h1 + (i + 1) * h2) & layers[0].mask;
      uint64_t v = layers[0].template get<uint64_t>(k);
      res = res && (v != 0);
    }
    return res;
  }

  template <typename RT> RT get_mean(const std::vector<uint64_t> &a) {
    uint64_t sum = 0;
    uint64_t h1 = H1, h2 = H2;

    hash(&a[0], 2, &h1, &h2);
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

  template <typename RT> RT get_min(const std::vector<uint64_t> &a) {

    uint64_t min_v = UINT64_MAX;
    uint64_t h1 = H1, h2 = H2;

    hash(&a[0], 2, &h1, &h2);
    for (auto i = 0; i < hashcount; i++) {
      uint64_t sum = 0;
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
      min_v = std::min(min_v, sum);
    }
    return (RT)min_v;
  }

  uint64_t byte_size() {
    uint64_t s = 0;
    for (auto &l : layers) {
      s += l.byte_size();
    }
    return s;
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
      ss << "\"unique_input\": " << unique_input.size() << ",\n";
      ss << "\"errors\": " << errors.size() << ",\n";
      ss << "\"error_rate\": " << error_rate << ",\n";
    }
    ss << "\"layers\": [\n";

    for (auto i = 0; i < layers.size(); i++) {
      ss << layers[i].summary() << ((i == layers.size() - 1) ? "\n" : ",\n");
    }
    ss << "]\n}" << std::endl;

    return ss.str();
  }
};

} // namespace globimap

#endif
