//
// Created by Mike Loomis on 6/22/2019.
//

#include <catch2/catch.hpp>

#include <msgpack/msgpack.hpp>

struct Example {
  int8_t x;
//  uint8_t y;
//  std::string z;
//  std::vector<std::string> array;

  template<class Packager>
  void pack(Packager &packager) {
    packager.process(x);
  }
};

TEST_CASE("Happy example interface") {
  auto example = Example{-32};
  std::vector<uint8_t> test_vector = msgpack::pack(example);

  auto unpacked_example = msgpack::unpack<Example>(test_vector.data());

  REQUIRE(unpacked_example.x == 1);
//  REQUIRE(unpacked_example.y == 2);
//  REQUIRE(unpacked_example.z == "Bob");
//  REQUIRE(unpacked_example.array == std::vector<std::string>{"Mike"});
}