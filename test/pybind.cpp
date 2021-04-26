#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;

int add(int i, int j) {
    return i + j;
}

int
get_val()
{
  int i = 45;
  return i;
}

/* PYBIND11_MODULE(pybind, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("add", &add, "A function which adds two numbers");
    m.def("get_val", &get_val, "return val");
}*/

int main ()
{
    py::object scipy = py::module_::import("scipy");
    // return scipy.attr("__version__");
    /*pybind11::initialize_interpreter();
    test();
    auto math = py::module::import("math");
    double root_two = math.attr("sqrt")(2.0).cast<double>();
    std::cout << "The square root of 2 is: " << root_two << "\n";
    py::finalize_interpreter();*/
}
