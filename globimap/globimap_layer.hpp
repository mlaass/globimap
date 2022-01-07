#ifndef GLOBIMAP_LAYER_HPP
#define GLOBIMAP_LAYER_HPP
#include <cstdlib>
namespace globimap {

template <typename BITS1 = bool, typename BITS8 = uint8_t,
          typename BITS16 = uint16_t, typename BITS32 = uint32_t,
          typename BITS64 = uint64_t>
struct LayerA {

  uint bits;
  uint64_t size;
  uint64_t mask;
  uint8_t *data;
  LayerA() : data(nullptr) {}
  virtual ~LayerA() {
    if (data != nullptr)
      std::free(data);
  }

  void resize(uint64_t new_size) {
    size = new_size;
    size_t byte_size = size * (bits / 8);
    if (bits == 1) {
      byte_size = size;
    }
    data = std::malloc(byte_size);
  }
};
template <typename BITS1 = bool, typename BITS8 = uint8_t,
          typename BITS16 = uint16_t, typename BITS32 = uint32_t,
          typename BITS64 = uint64_t>
struct LayerV {

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

} // namespace globimap
#endif