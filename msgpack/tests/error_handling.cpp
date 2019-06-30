//
// Created by Mike Loomis on 6/29/2019.
//

#include <catch2/catch.hpp>

#include "msgpack/msgpack.hpp"

struct Example {
  std::map<std::string, bool> map;

  template<class T>
  void msgpack(T &pack) {
    pack(map);
  }
};

TEST_CASE("Unpacking a short dataset safely fails") {
  Example example{{{"compact", true}, {"schema", false}}};
  auto data = std::vector<uint8_t>{0x82, 0xa7, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x63, 0x74, 0xc3, 0xa6, 0x73, 0x63};
  std::error_code ec{};
  REQUIRE(!ec);
  REQUIRE(example.map != msgpack::unpack<Example>(data, ec).map);
  REQUIRE(ec);
  REQUIRE(ec == msgpack::UnpackerError::OutOfRange);
}