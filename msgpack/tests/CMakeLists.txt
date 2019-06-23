cmake_minimum_required(VERSION 3.9)

find_package(Catch2 REQUIRED)

add_executable(Msgpack_tests
               main.cpp
               interface_dev_tests.cpp)

target_link_libraries(Msgpack_tests
                      Catch2::Catch2
                      Msgpack::Msgpack)

target_compile_features(Msgpack_tests PUBLIC cxx_std_17)
