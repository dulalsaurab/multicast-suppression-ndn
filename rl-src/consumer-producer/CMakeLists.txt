cmake_minimum_required (VERSION 3.1)

set (CMAKE_CXX_STANDARD 14)

INCLUDE (FindPkgConfig)
pkg_check_modules (NDN_CXX libndn-cxx)

project (producer)
add_executable (producer producer.cpp)

include_directories(producer ${NDN_CXX_INCLUDE_DIRS})
link_directories(producer ${NDN_CXX_LIBRARY_DIRS} )
target_link_libraries(producer ${NDN_CXX_LIBRARIES})


add_executable (consumer consumer.cpp)

include_directories(consumer ${NDN_CXX_INCLUDE_DIRS})
link_directories(consumer ${NDN_CXX_LIBRARY_DIRS} )
target_link_libraries(consumer ${NDN_CXX_LIBRARIES})

