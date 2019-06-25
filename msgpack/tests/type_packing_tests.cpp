//
// Created by Mike Loomis on 6/22/2019.
//

#include <catch2/catch.hpp>

#include <msgpack/msgpack.hpp>

struct Example {
  std::map<int, int> x;
//  uint8_t y;
//  std::string z;
//  std::vector<std::string> array;

  template<class Packager>
  void pack(Packager &packager) {
    packager.process(x);
  }
};

TEST_CASE("Nil type packing") {
  auto null_obj = std::nullptr_t{};
  auto packer = msgpack::Packer{};
  packer.process(null_obj);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xc0});
}

TEST_CASE("Boolean type packing") {
  auto bool_obj = false;
  auto packer = msgpack::Packer{};
  packer.process(bool_obj);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xc2});
  bool_obj = true;
  packer.clear();
  packer.process(bool_obj);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xc3});
}

TEST_CASE("Integer type packing") {
  uint8_t x1 = 0x1;
  uint16_t x2 = 0x1111;
  uint32_t x3 = 0x11111111;
  uint64_t x4 = 0x1111111111111111;
  uint8_t x5 = std::numeric_limits<uint8_t>::max();

  int8_t y1 = -1;
  int16_t y2 = std::numeric_limits<int8_t>::min() - 1;
  int32_t y3 = std::numeric_limits<int16_t>::min() - 1;
  int64_t y4 = std::numeric_limits<int64_t>::min();
  int64_t y5 = std::numeric_limits<int64_t>::min() + 1;
  int64_t y6 = 0;

  auto packer = msgpack::Packer{};
  packer.process(x1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x1});
  packer.clear();
  packer.process(x2);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xcd, 0x11, 0x11});
  packer.clear();
  packer.process(x3);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xce, 0x11, 0x11, 0x11, 0x11});
  packer.clear();
  packer.process(x4);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xcf, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11});
  packer.clear();
  packer.process(x5);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xcc, 0xff});

  packer.clear();
  packer.process(y1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xff});
  packer.clear();
  packer.process(y2);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xd1, 0xff, 0x7f});
  packer.clear();
  packer.process(y3);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xd2, 0xff, 0xff, 0x7f, 0xff});
  packer.clear();
  packer.process(y4);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xd3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  packer.clear();
  packer.process(y5);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xd3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01});
  packer.clear();
  packer.process(y6);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x00});
}

TEST_CASE("Float type packing") {
  float x1 = 0.0f;
  float x2 = -0.0f;
  float x3 = 12345.67f;
  float x4 = -12345.6789f;

  double y1 = 0.0;
  double y2 = -0.0;
  double y3 = 12345.67;
  double y4 = -12345.67;

  auto packer = msgpack::Packer{};
  packer.process(x1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x00});
  packer.clear();
  packer.process(x2);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x00});
  packer.clear();
  packer.process(x3);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xca, 0x46, 0x40, 0xe6, 0xae});
  packer.clear();
  packer.process(x4);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xca, 0xc6, 0x40, 0xe6, 0xb7});

  packer.clear();
  packer.process(y1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x00});
  packer.clear();
  packer.process(y2);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0x00});
  packer.clear();
  packer.process(y3);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xcb, 0x40, 0xc8, 0x1c, 0xd5, 0xc2, 0x8f, 0x5c, 0x29});
  packer.clear();
  packer.process(y4);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xcb, 0xc0, 0xc8, 0x1c, 0xd5, 0xc2, 0x8f, 0x5c, 0x29});
}

TEST_CASE("String type packing") {
  auto str1 = std::string("test");
  auto packer = msgpack::Packer{};
  packer.process(str1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0b10100000 | 4, 't', 'e', 's', 't'});
}

TEST_CASE("Byte array type packing") {
  auto vec1 = std::vector<uint8_t>{1, 2, 3, 4};
  auto packer = msgpack::Packer{};
  packer.process(vec1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0xc4, 4, 1, 2, 3, 4});
}

TEST_CASE("Array type packing") {
  auto list1 = std::list<std::string>{"one", "two", "three"};
  auto packer = msgpack::Packer{};
  packer.process(list1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0b10010000 | 3, 0b10100000 | 3, 'o', 'n', 'e',
                                                  0b10100000 | 3, 't', 'w', 'o',
                                                  0b10100000 | 5, 't', 'h', 'r', 'e', 'e'});
}

TEST_CASE("Map type packing") {
  auto map1 = std::map<uint8_t, std::string>{std::make_pair(0, "zero"), std::make_pair(1, "one")};
  auto packer = msgpack::Packer{};
  packer.process(map1);
  REQUIRE(packer.vector() == std::vector<uint8_t>{0b10000000 | 2, 0, 0b10100000 | 4, 'z', 'e', 'r', 'o',
                                                  1, 0b10100000 | 3, 'o', 'n', 'e'});
}