//
// Created by Mike Loomis on 6/29/2019.
//

#include <catch2/catch.hpp>

#include "msgpack/msgpack.hpp"

struct ExampleError {
  std::map<std::string, bool> map;

  template<class T>
  void pack(T &pack) {
    pack(map);
  }
};

TEST_CASE("Unpacking a short dataset safely fails") {
  ExampleError example{{{"compact", true}, {"schema", false}}};
  auto data = std::vector<uint8_t>{0x82, 0xa7};
  std::error_code ec{};
  REQUIRE(!ec);
  REQUIRE(example.map != msgpack::unpack<ExampleError>(data, ec).map);
  REQUIRE(ec);
  REQUIRE(ec == msgpack::UnpackerError::OutOfRange);
}

TEST_CASE("Unpacking a mismatched dataset safely fails") {
  // TODO: Add bad input fails for all unpack types
}