#include "rigid_registration_runner.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(trilli_wrapper, m) {
  m.doc() = "Python bindings for Trilli rigid registration";

  m.def("run_rigid_registration_trilli", &run_rigid_registration_trilli,
        py::arg("ref_img_path"), py::arg("float_img_path"),
        py::arg("output_folder"), py::arg("n_couples"), py::arg("rangeX") = 256,
        py::arg("rangeY") = 256, py::arg("rangeAngZ") = 1.0f,
        "Run rigid registration on 3D images");
}
