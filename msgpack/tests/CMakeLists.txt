cmake_minimum_required(VERSION 3.9)

include(FetchContent)
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.1.0)
FetchContent_MakeAvailable(Catch2)

add_executable(Msgpack_tests
               type_packing_tests.cpp
               examples.cpp
               error_handling.cpp
               object_packing_tests.cpp
               )

if (MSVC)
    target_compile_options(Msgpack_tests PRIVATE /WX /Zc:__cplusplus)
else (MSVC)
    target_compile_options(Msgpack_tests PRIVATE -Wall -Wextra -pedantic -Werror)
endif (MSVC)

target_link_libraries(Msgpack_tests
                      Catch2::Catch2WithMain
                      Msgpack::Msgpack)

set_target_properties(Msgpack_tests PROPERTIES CXX_STANDARD 17)
target_compile_features(Msgpack_tests PUBLIC cxx_std_17)
