#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <functional>
#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// This holds the actual implementation. Copy this header to your projects (and
// a hasher, for example murmur.hpp)

#include "globimap.hpp"

// Wrap 2D C++ array (given as pointer) to a numpy object.
template <typename T>
static py::array_t<T> wrap2D(T *data, size_t w, size_t h) {

  auto shape = {h, w};
  auto strides = std::vector<size_t>({8 * w, 8});
  auto caps = py::capsule(
      data, [](void *v) { /*delete reinterpret_cast<double *>(v);*/ });

  return py::array_t<T, py::array::forcecast | py::array::c_style>(
      shape, strides, data, caps);
}
template <typename T> static py::array_t<T> wrap1D(T *data, size_t s) {

  auto shape = {s};
  auto strides = std::vector<size_t>({sizeof(T)});

  return py::array_t<T, py::array::forcecast | py::array::c_style>(
      shape, strides, data);
}

// This template is a handy tool to call a function f(i,j,value) for each entry
// of a 2D matrix self. template<typename func> static void
void map_matrix(const py::array_t<double> &self,
                std::function<void(int, int, double)> f) {
  if (self.ndim() != 2)
    throw(std::runtime_error("2D array expected"));
  auto s1 = self.strides(0);
  auto s2 = self.strides(1);
  const char *data = reinterpret_cast<const char *>(self.data());

  for (int i1 = 0; i1 < self.shape(0); i1++) {
    for (int i2 = 0; i2 < self.shape(1); i2++) {
      size_t offset = i1 * s1 + i2 * s2;
      // std::cout <<"("<< offset<<", "<<i1 <<", "<<i2 <<"), ";
      const double *d = reinterpret_cast<const double *>(data + offset);
      f(i1, i2, *d);
    }
  }
  std::cout << std::endl;
}
template <typename T>
void map_pointcloud(const py::array_t<T> &self,
                    std::function<void(int, int)> f) {
  if (self.ndim() != 2)
    throw(std::runtime_error("2D array expected"));
  auto s1 = self.strides(0);
  auto s2 = self.strides(1);
  const char *data = reinterpret_cast<const char *>(self.data());

  for (int i1 = 0; i1 < self.shape(0); i1++) {
    size_t offset0 = i1 * s1;
    size_t offset1 = i1 * s1 + s2;
    // std::cout <<"("<< offset<<", "<<i1 <<", "<<i2 <<"), ";
    const double *d0 = reinterpret_cast<const double *>(data + offset0);
    const double *d1 = reinterpret_cast<const double *>(data + offset1);
    f((int)*d0, (int)*d1);
  }
  std::cout << std::endl;
}

// This wil be our implementation in C++ of a Python class globimap.
typedef GloBiMap<uint8_t> globimap_t;
typedef GloBiMap<uint8_t> globimap_t;

// The module begins
PYBIND11_MODULE(globimap, m) {
  // It exports a class (named globimap) with chained functions, see README.md
  py::class_<globimap_t>(m, "globimap")
      .def(py::init<>())
      .def("rasterize",
           +[](globimap_t &self, size_t x, size_t y, size_t s0, size_t s1) {
             auto &data = self.rasterize(x, y, s0, s1);
             return wrap2D<double>((double *)&data[0], s0, s1);
           })
      .def("correct",
           +[](globimap_t &self, size_t x, size_t y, size_t s0,
               size_t s1) -> py::array {
             auto &data = self.apply_correction(x, y, s0, s1);
             return wrap2D<double>((double *)&data[0], s0, s1);
           })
      .def("put",
           +[](globimap_t &self, uint32_t x, uint32_t y) {
             self.put({x, y});
           })
      .def("put",
           +[](globimap_t &self, uint32_t x, uint32_t y, uint32_t z) {
             std::vector<uint32_t> a = {x, y, z, 0};
             self.putp((uint64_t *)&a[0]);
           })
      .def("get",
           +[](globimap_t &self, uint32_t x, uint32_t y) -> bool {
             return self.get({x, y});
           })
      .def("get",
           +[](globimap_t &self, uint32_t x, uint32_t y, uint32_t z) -> bool {
             std::vector<uint32_t> a = {x, y, z, 0};
             return self.getp((uint64_t *)&a[0]);
           })
      .def("configure",
           +[](globimap_t &self, size_t k, size_t m) { self.configure(k, m); })
      .def("clear", +[](globimap_t &self) { self.clear(); })
      .def("summary",
           +[](globimap_t &self) -> std::string { return self.summary(); })
      .def("map",
           +[](globimap_t &self, py::array mat, int o0, int o1) {
             map_matrix(mat, [&](int i0, int i1, double v) {
               if (v != 0 && v != 1) {
                 std::cout << v << std::endl;
                 throw(std::runtime_error("data is not binary."));
               }

               if (v == 1)
                 self.put({static_cast<uint32_t>(o0 + i0),
                           static_cast<uint32_t>(o1 + i1)});
             });
           })
      .def("enforce",
           +[](globimap_t &self, py::array mat, int o0, int o1) {
             map_matrix(mat, [&](int i0, int i1, double v) {
               if (v == 0 && self.get({static_cast<uint32_t>(o0 + i0),
                                       static_cast<uint32_t>(o1 + i1)})) {
                 // this is a false positive
                 self.add_error({static_cast<uint32_t>(o0 + i0),
                                 static_cast<uint32_t>(o1 + i1)});
               }
             });
           })
      .def("get_buffer",
           +[](globimap_t &self) -> py::array_t<uint8_t> {
             std::string buf;
             self.tobuffer(buf);
             return py::array_t<uint8_t>(
                 buf.length(), reinterpret_cast<const uint8_t *>(buf.data()));
           })
      .def("from_buffer",
           +[](globimap_t &self, py::array_t<uint8_t> buf) -> void {
             self.from_buffer(buf.data(), buf.size(), buf.size() * 8);
           })
      .def("get_filter", +[](globimap_t &self) { return self.filter; })
      .def("stats", +[](globimap_t &self) { return self.stats(); })
      // .def("get_filter_np",
      //      +[](globimap_t &self) -> py::array_t<bool> {
      //        return wrap1D<bool>(&self.filter[0], self.filter.size());
      //      })
      .def("get_filterf", +[](globimap_t &self) -> py::array_t<float> {
        py::array_t<float, py::array::c_style> a({self.filter.size()});
        auto r = a.mutable_unchecked();

        for (py::ssize_t i = 0; i < r.shape(0); i++) {
          r(i) = self.filter[i] ? 1.0f : 0.0f;
        }
        return a;
      });
}
