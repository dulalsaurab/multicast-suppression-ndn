#include <boost/python.hpp>

using namespace boost::python;

class A{ // simple example class
 public:
  A(int n) { value = n; }
  void set(int n) { value = n; }
  int get() { return value; }
 private:
  int value;
};

BOOST_PYTHON_MODULE(module_A){
// Create the Python type object for our extension class and
// define __init__ function.
class_<A>("A", init<int>())
  .def("get", &A::get, "docstring here") //Add a regular member function
  .add_property("value", &A::get, &A::set)
;
}
