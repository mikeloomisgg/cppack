//
// Created by Mike Loomis on 6/28/2019.
//

#include <catch2/catch.hpp>

#include "msgpack/msgpack.hpp"

struct Example {
  bool compact{};
  bool schema{};

  template<class T>
  void pack(T &pack) {
    pack(compact, schema);
  }
};

TEST_CASE("Website example") {
  Example example{true, false};
  auto data = msgpack::pack(example);

  REQUIRE(data.size() == 2);
  REQUIRE(data == std::vector<uint8_t>{0xc3, 0xc2});

  REQUIRE(example.compact == msgpack::unpack<Example>(data).compact);
  REQUIRE(example.schema == msgpack::unpack<Example>(data).schema);
}

TEST_CASE("Name/value pair style packing") {
  Example example{true, false};
  auto data = msgpack::nvp_pack(example);

  REQUIRE(data.size() == 7);
  REQUIRE(data == std::vector<uint8_t>{0x82, 0xa1, 0x30, 0xc3, 0xa1, 0x31, 0xc2});

  REQUIRE(example.compact == msgpack::nvp_unpack<Example>(data).compact);
  REQUIRE(example.schema == msgpack::nvp_unpack<Example>(data).schema);
}

TEST_CASE("Unpack with error handling") {
  auto data = std::vector<uint8_t>{0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74, 0xc3, 0xa6, 0x73, 0x63};
  std::error_code ec{};
  auto unpacked_object = msgpack::unpack<Example>(data, ec);
  if (ec && ec == msgpack::UnpackerError::BadInput)
    REQUIRE(true);
  else
    REQUIRE(false);
}