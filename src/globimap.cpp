#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <iostream>
#include <functional>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

// This holds the actual implementation. Copy this header to your projects (and
// a hasher, for example murmur.hpp)

#include "globimap.hpp"

// Wrap 2D C++ array (given as pointer) to a numpy object.
static py::array_t<double> wrap2D(double *data, size_t h, size_t w) {

  auto shape = {h, w};
  auto strides = std::vector<size_t>({w * 8, 8});
  auto caps =
      py::capsule(data, [](void *v) { /*delete reinterpret_cast<double *>(v);*/ });

  return py::array_t<double, py::array::forcecast | py::array::c_style>(
      shape, strides, data, caps);
}

// This template is a handy tool to call a function f(i,j,value) for each entry
// of a 2D matrix self. template<typename func> static void
void map_matrix(const py::array_t<double> &self, std::function<void(int,int,double)> f) {
  if (self.ndim() != 2)
    throw(std::runtime_error("2D array expected"));
  auto s1 = self.strides(0);
  auto s2 = self.strides(1);
  const char * data = reinterpret_cast<const char *>(self.data());

  for (int i1 = 0; i1 < self.shape(0); i1++) {
    for (int i2 = 0; i2 < self.shape(1); i2++) {
      size_t offset = i1 * s1 + i2 * s2;
      //std::cout <<"("<< offset<<", "<<i1 <<", "<<i2 <<"), ";
      const double *d = reinterpret_cast<const double *>(data + offset);
      f(i1, i2, *d);
    }
  }
  std::cout << std::endl;
}

// This wil be our implementation in C++ of a Python class globimap.
typedef GloBiMap<bool> globimap_t;

// The module begins
PYBIND11_MODULE(globimap, m) {
  // It exports a class (named globimap) with chained functions, see README.md
  py::class_<globimap_t>(m, "globimap")
      .def(py::init<>())
      .def("rasterize",
           +[](globimap_t &self, size_t x, size_t y, size_t s0, size_t s1) {
             auto &data = self.rasterize(x, y, s0, s1);
             return wrap2D(&data[0], s0, s1);
           })
      .def("correct",
           +[](globimap_t &self, size_t x, size_t y, size_t s0,
               size_t s1) -> py::array {
             auto &data = self.apply_correction(x, y, s0, s1);
             return wrap2D(&data[0], s0, s1);
           })
      .def("put",
           +[](globimap_t &self, uint32_t x, uint32_t y) {
             self.put({x, y});
           })
      .def("get",
           +[](globimap_t &self, uint32_t x, uint32_t y) -> bool {
             return self.get({x, y});
           })
      .def("configure",
           +[](globimap_t &self, size_t k, size_t m) { self.configure(k, m); })
      .def("clear", +[](globimap_t &self) { self.clear(); })
      .def("summary",
           +[](globimap_t &self) -> std::string { return self.summary(); })
      .def("map", +[](globimap_t &self, py::array mat, int o0, int o1) {
          map_matrix(mat, [&](int i0, int i1, double v) {
            if (v != 0 && v != 1){
              std::cout << v<< std::endl;
              throw(std::runtime_error("data is not binary."));
            }
              
            if (v == 1)
              self.put({static_cast<uint32_t>(o0 + i0),
                        static_cast<uint32_t>(o1 + i1)});
          });
        })
      .def("enforce", +[](globimap_t &self, py::array mat, int o0, int o1) {
        map_matrix(mat, [&](int i0, int i1, double v) {
          if (v == 0 && self.get({static_cast<uint32_t>(o0 + i0),
                                  static_cast<uint32_t>(o1 + i1)})) {
            // this is a false positive
            self.add_error({static_cast<uint32_t>(o0 + i0),
                            static_cast<uint32_t>(o1 + i1)});
          }
        });
      });
}
