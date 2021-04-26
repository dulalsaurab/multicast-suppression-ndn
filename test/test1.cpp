#include <iostream>
#include "Python.h"

#include <stdio.h>

#define MOD_NAME "my_mod"

int
main()
{
  // setenv("PYTHONPATH",".",1);
  const char *scriptDirectoryName = "/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/test";
  PyObject *pName=NULL, *pModule=NULL, *pDict, *pFunc, *pValue, *presult;

  // Initialize the Python Interpreter
   Py_Initialize();
   PyObject *sysPath = PySys_GetObject("path");
   PyObject *path = PyUnicode_FromString(scriptDirectoryName);
   int result = PyList_Insert(sysPath, 0, path);
   PyObject *fname = PyUnicode_FromString("my_mod");
   pModule = PyImport_Import(fname);
   if (pModule != NULL)
   {
     std::cout << "Python module found\n";

     // Load all module level attributes as a dictionary
     PyObject *pDict = PyModule_GetDict(pModule);

     // Remember that you are loading the module as a dictionary, the lookup you were
     // doing on pDict would fail as you were trying to find something as an attribute
     // which existed as a key in the dictionary
     PyObject *pFunc = PyDict_GetItem(pDict, PyUnicode_FromString("my_func"));

     if (pFunc != NULL)
     {
       PyObject_CallObject(pFunc, NULL);
     }
     else
     {
       std::cout << "Couldn't find func\n";
     }
   }
   else
     {
          PyErr_Print();
          std::exit(1);
       std::cout << "Python Module not found\n";
     }
   return 0;
}
