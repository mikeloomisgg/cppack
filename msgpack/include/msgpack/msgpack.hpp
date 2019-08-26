//
// Created by Mike Loomis on 6/22/2019.
//

#ifndef CPPACK_PACKER_HPP
#define CPPACK_PACKER_HPP

#include <vector>
#include <set>
#include <list>
#include <map>
#include <array>
#include <chrono>
#include <cmath>
#include <bitset>
#include <unordered_map>

namespace msgpack {
enum class UnpackerError {
  OutOfRange = 1,
  BadInput = 2
};

struct UnpackerErrCategory : public std::error_category {
 public:
  const char *name() const noexcept override {
    return "unpacker";
  };

  std::string message(int ev) const override {
    switch (static_cast<msgpack::UnpackerError>(ev)) {
      case msgpack::UnpackerError::OutOfRange:
        return "tried to dereference out of range during deserialization";
      case msgpack::UnpackerError::BadInput:
        return "data decoded does not match type of object";
      default:
        return "(unrecognized error)";
    }
  };
};

const UnpackerErrCategory theUnpackerErrCategory{};

inline
std::error_code make_error_code(msgpack::UnpackerError e) {
  return {static_cast<int>(e), theUnpackerErrCategory};
}
}

namespace std {
template<>
struct is_error_code_enum<msgpack::UnpackerError> : public true_type {};
}

namespace msgpack {

enum FormatConstants : uint8_t {
  // positive fixint = 0x00 - 0x7f
  // fixmap = 0x80 - 0x8f
  // fixarray = 0x90 - 0x9a
  // fixstr = 0xa0 - 0xbf
  // negative fixint = 0xe0 - 0xff

  nil = 0xc0,
  false_bool = 0xc2,
  true_bool = 0xc3,
  bin8 = 0xc4,
  bin16 = 0xc5,
  bin32 = 0xc6,
  ext8 = 0xc7,
  ext16 = 0xc8,
  ext32 = 0xc9,
  float32 = 0xca,
  float64 = 0xcb,
  uint8 = 0xcc,
  uint16 = 0xcd,
  uint32 = 0xce,
  uint64 = 0xcf,
  int8 = 0xd0,
  int16 = 0xd1,
  int32 = 0xd2,
  int64 = 0xd3,
  fixext1 = 0xd4,
  fixext2 = 0xd5,
  fixext4 = 0xd6,
  fixext8 = 0xd7,
  fixext16 = 0xd8,
  str8 = 0xd9,
  str16 = 0xda,
  str32 = 0xdb,
  array16 = 0xdc,
  array32 = 0xdd,
  map16 = 0xde,
  map32 = 0xdf
};

template<class T>
struct is_container {
  static const bool value = false;
};

template<class T, class Alloc>
struct is_container<std::vector<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::list<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::map<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::unordered_map<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::set<T, Alloc> > {
  static const bool value = true;
};

template<class T>
struct is_stdarray {
  static const bool value = false;
};

template<class T, std::size_t N>
struct is_stdarray<std::array<T, N>> {
  static const bool value = true;
};

template<class T>
struct is_map {
  static const bool value = false;
};

template<class T, class Alloc>
struct is_map<std::map<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_map<std::unordered_map<T, Alloc> > {
  static const bool value = true;
};

struct skip {};

template<bool nvp_packing = false>
class Packer {
 public:
  template<class ... Types>
  void operator()(const Types &... args) {
    if constexpr(nvp_packing) {
      std::map<std::string, skip> placeholder{};
      const std::size_t n = sizeof...(Types);
      for (auto i = 0U; i < n; i++) {
        placeholder[std::to_string(i)];
      }
      pack_type(placeholder);
      (pack_nvp(std::forward<const Types &>(args)), ...);
    } else {
      (pack_type(std::forward<const Types &>(args)), ...);
    }
  }

  template<class ... Types>
  void process(const Types &... args) {
    if constexpr(nvp_packing) {
      std::map<std::string, skip> placeholder{};
      const std::size_t n = sizeof...(Types);
      for (auto i = 0U; i < n; i++) {
        placeholder[std::to_string(i)];
      }
      pack_type(placeholder);
      (pack_nvp(std::forward<const Types &>(args)), ...);
    } else {
      (pack_type(std::forward<const Types &>(args)), ...);
    }
  }

  const std::vector<uint8_t> &vector() const {
    return serialized_object;
  }

  void clear() {
    serialized_object.clear();
  }

 private:
  std::vector<uint8_t> serialized_object{};
  std::size_t current_index{};

  template<typename T>
  void pack_nvp(const T &value) {
    pack_type(std::to_string(current_index));
    pack_type(value);
    current_index++;
  }

  template<class T>
  void pack_type(const T &value) {
    if constexpr(is_map<T>::value) {
      pack_map(value);
    } else if constexpr (is_container<T>::value || is_stdarray<T>::value) {
      pack_array(value);
    } else {
      auto recursive_packer = Packer<nvp_packing>{};
      const_cast<T &>(value).pack(recursive_packer);
      auto vec = recursive_packer.vector();
      serialized_object.insert(serialized_object.end(), vec.begin(), vec.end());
    }
  }

  template<class T>
  void pack_type(const std::chrono::time_point<T> &value) {
    pack_type(value.time_since_epoch().count());
  }

  template<class T>
  void pack_array(const T &array) {
    if (array.size() < 16) {
      auto size_mask = uint8_t(0b10010000);
      serialized_object.emplace_back(uint8_t(array.size() | size_mask));
    } else if (array.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(array16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(array.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (array.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(array32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(array.size() >> (8U * (i - 1)) & 0xff));
      }
    } else {
      return; // Give up if string is too long
    }
    for (const auto &elem : array) {
      pack_type(elem);
    }
  }

  template<class T>
  void pack_map(const T &map) {
    if (map.size() < 16) {
      auto size_mask = uint8_t(0b10000000);
      serialized_object.emplace_back(uint8_t(map.size() | size_mask));
    } else if (map.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(map16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (map.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(map32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    }
    for (const auto &elem : map) {
      pack_type(std::get<0>(elem));
      pack_type(std::get<1>(elem));
    }
  }

  void pack_map(std::map<std::string, skip> map) {
    if (map.size() < 16) {
      auto size_mask = uint8_t(0b10000000);
      serialized_object.emplace_back(uint8_t(map.size() | size_mask));
    } else if (map.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(map16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (map.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(map32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  std::bitset<64> twos_complement(int64_t value) {
    if (value < 0) {
      auto abs_v = llabs(value);
      return ~abs_v + 1;
    } else {
      return {(uint64_t) value};
    }
  }

  std::bitset<32> twos_complement(int32_t value) {
    if (value < 0) {
      auto abs_v = abs(value);
      return ~abs_v + 1;
    } else {
      return {(uint32_t) value};
    }
  }

  std::bitset<16> twos_complement(int16_t value) {
    if (value < 0) {
      auto abs_v = abs(value);
      return ~abs_v + 1;
    } else {
      return {(uint16_t) value};
    }
  }

  std::bitset<8> twos_complement(int8_t value) {
    if (value < 0) {
      auto abs_v = abs(value);
      return ~abs_v + 1;
    } else {
      return {(uint8_t) value};
    }
  }

  void pack_type(const skip &value) {
  }

  void pack_type(const int8_t &value) {
    if (value > 31 || value < -32) {
      serialized_object.emplace_back(int8);
    }
    serialized_object.emplace_back(uint8_t(twos_complement(value).to_ulong()));
  }

  void pack_type(const int16_t &value) {
    if (abs(value) < abs(std::numeric_limits<int8_t>::min())) {
      pack_type(int8_t(value));
    } else {
      serialized_object.emplace_back(int16);
      auto serialize_value = uint16_t(twos_complement(value).to_ulong());
      for (auto i = sizeof(value); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  void pack_type(const int32_t &value) {
    if (abs(value) < abs(std::numeric_limits<int16_t>::min())) {
      pack_type(int16_t(value));
    } else {
      serialized_object.emplace_back(int32);
      auto serialize_value = uint32_t(twos_complement(value).to_ulong());
      for (auto i = sizeof(value); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  void pack_type(const int64_t &value) {
    if (llabs(value) < llabs(std::numeric_limits<int32_t>::min()) && value != std::numeric_limits<int64_t>::min()) {
      pack_type(int32_t(value));
    } else {
      serialized_object.emplace_back(int64);
      auto serialize_value = uint64_t(twos_complement(value).to_ullong());
      for (auto i = sizeof(value); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  void pack_type(const uint8_t &value) {
    if (value <= 0x7f) {
      serialized_object.emplace_back(value);
    } else {
      serialized_object.emplace_back(uint8);
      serialized_object.emplace_back(value);
    }
  }

  void pack_type(const uint16_t &value) {
    if (value > std::numeric_limits<uint8_t>::max()) {
      serialized_object.emplace_back(uint16);
      for (auto i = sizeof(value); i > 0U; --i) {
        serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
      }
    } else {
      pack_type(uint8_t(value));
    }
  }

  void pack_type(const uint32_t &value) {
    if (value > std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(uint32);
      for (auto i = sizeof(value); i > 0U; --i) {
        serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
      }
    } else {
      pack_type(uint16_t(value));
    }
  }

  void pack_type(const uint64_t &value) {
    if (value > std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(uint64);
      for (auto i = sizeof(value); i > 0U; --i) {
        serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
      }
    } else {
      pack_type(uint32_t(value));
    }
  }

  void pack_type(const std::nullptr_t &/*value*/) {
    serialized_object.emplace_back(nil);
  }

  void pack_type(const bool &value) {
    if (value) {
      serialized_object.emplace_back(true_bool);
    } else {
      serialized_object.emplace_back(false_bool);
    }
  }

  void pack_type(const float &value) {
    double integral_part;
    auto fractional_remainder = float(modf(value, &integral_part));

    if (fractional_remainder == 0) { // Just pack as int
      pack_type(int64_t(integral_part));
    } else {
      // TODO: Handle decimal floats, some systems will use base 10 floats. Until then don't compile
      static_assert(std::numeric_limits<float>::radix == 2);
      auto exponent = ilogb(value);
      float full_mantissa = value / float(scalbn(1.0, exponent));
      auto sign_mask = std::bitset<32>(uint32_t(std::signbit(full_mantissa)) << 31);
      auto excess_127_exponent_mask = std::bitset<32>(uint32_t(exponent + 127) << 23);
      auto normalized_mantissa_mask = std::bitset<32>();
      float implied_mantissa = fabs(full_mantissa) - 1.0f;
      for (auto i = 23U; i > 0; --i) {
        integral_part = 0;
        implied_mantissa *= 2;
        implied_mantissa = float(modf(implied_mantissa, &integral_part));
        if (uint8_t(integral_part) == 1) {
          normalized_mantissa_mask |= std::bitset<32>(uint32_t(1 << (i - 1)));
        }
      }

      uint32_t ieee754_float32 = (sign_mask | excess_127_exponent_mask | normalized_mantissa_mask).to_ulong();
      serialized_object.emplace_back(float32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(ieee754_float32 >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  // TODO: Tested serialized doubles from javascript weren't unpacking correctly
  void pack_type(const double &value) {
    double integral_part;
    double fractional_remainder = modf(value, &integral_part);

    if (fractional_remainder == 0) { // Just pack as int
      pack_type(int64_t(integral_part));
    } else {
      static_assert(std::numeric_limits<float>::radix == 2);
      auto exponent = ilogb(value);
      double full_mantissa = value / scalbn(1.0, exponent);
      auto sign_mask = std::bitset<64>(uint64_t(std::signbit(full_mantissa)) << 63);
      auto excess_127_exponent_mask = std::bitset<64>(uint64_t(exponent + 1023) << 52);
      auto normalized_mantissa_mask = std::bitset<64>();
      double implied_mantissa = fabs(full_mantissa) - 1.0f;

      for (auto i = 52U; i > 0; --i) {
        integral_part = 0;
        implied_mantissa *= 2;
        implied_mantissa = modf(implied_mantissa, &integral_part);
        if (uint8_t(integral_part) == 1) {
          normalized_mantissa_mask |= std::bitset<64>(uint64_t(1) << (i - 1));
        }
      }
      auto ieee754_float64 = (sign_mask | excess_127_exponent_mask | normalized_mantissa_mask).to_ullong();
      serialized_object.emplace_back(float64);
      for (auto i = sizeof(ieee754_float64); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(ieee754_float64 >> (8U * (i - 1)) & 0xff));
      }
    }
  }

  void pack_type(const std::string &value) {
    if (value.size() < 32) {
      serialized_object.emplace_back(uint8_t(value.size()) | 0b10100000);
    } else if (value.size() < std::numeric_limits<uint8_t>::max()) {
      serialized_object.emplace_back(str8);
      serialized_object.emplace_back(uint8_t(value.size()));
    } else if (value.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(str16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (value.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(str32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
      }
    } else {
      return; // Give up if string is too long
    }
    for (char i : value) {
      serialized_object.emplace_back(static_cast<uint8_t>(i));
    }
  }

  void pack_type(const std::vector<uint8_t> &value) {
    if (value.size() < std::numeric_limits<uint8_t>::max()) {
      serialized_object.emplace_back(bin8);
      serialized_object.emplace_back(uint8_t(value.size()));
    } else if (value.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(bin16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (value.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(bin32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
      }
    } else {
      return; // Give up if vector is too large
    }
    for (const auto &elem : value) {
      serialized_object.emplace_back(elem);
    }
  }
};

template<bool nvp_unpacking = false>
class Unpacker {
 public:
  Unpacker() : data_pointer(nullptr), data_end(nullptr) {};

  Unpacker(const uint8_t *data_start, std::size_t bytes)
      : data_pointer(data_start), data_end(data_start + bytes) {};

  template<class ... Types>
  void operator()(Types &... args) {
    if constexpr(nvp_unpacking) {
      std::map<std::string, skip> placeholder{};
      const std::size_t n = sizeof...(Types);
      for (auto i = 0U; i < n; i++) {
        placeholder[std::to_string(i)];
      }
      unpack_type(placeholder);
      (unpack_nvp(std::forward<Types &>(args)), ...);
    } else {
      (unpack_type(std::forward<Types &>(args)), ...);
    }
  }

  template<class ... Types>
  void process(Types &... args) {
    (unpack_type(std::forward<Types &>(args)), ...);
  }

  void set_data(const uint8_t *pointer, std::size_t size) {
    data_pointer = pointer;
    data_end = data_pointer + size;
  }

  std::size_t bytes() {
    return bytes_consumed;
  }

  std::error_code ec{};

 private:
  const uint8_t *data_pointer;
  const uint8_t *data_end;
  std::size_t bytes_consumed{};

  uint8_t safe_data() {
    if (data_pointer < data_end)
      return *data_pointer;
    ec = UnpackerError::OutOfRange;
    return 0;
  }

  void safe_increment(int64_t bytes = 1) {
    if (data_end - data_pointer >= 0) {
      data_pointer += bytes;
      bytes_consumed += bytes;
    } else {
      ec = UnpackerError::OutOfRange;
    }
  }

  template<class T>
  void unpack_type(T &value) {
    if constexpr(is_map<T>::value) {
      unpack_map(value);
    } else if constexpr (is_container<T>::value) {
      unpack_array(value);
    } else if constexpr (is_stdarray<T>::value) {
      unpack_stdarray(value);
    } else {
      auto unpacker = Unpacker<nvp_unpacking>{data_pointer, static_cast<std::size_t>(data_end - data_pointer)};
      value.pack(unpacker);
      safe_increment(unpacker.bytes());
      ec = unpacker.ec;
    }
  }

  template<class Clock, class Duration>
  void unpack_type(std::chrono::time_point<Clock, Duration> &value) {
    using RepType = typename std::chrono::time_point<Clock, Duration>::rep;
    using DurationType = Duration;
    using TimepointType = typename std::chrono::time_point<Clock, Duration>;
    auto placeholder = RepType{};
    unpack_type(placeholder);
    value = TimepointType(DurationType(placeholder));
  }

  template<class T>
  void unpack_array(T &array) {
    using ValueType = typename T::value_type;
    if (safe_data() == array32) {
      safe_increment();
      std::size_t array_size = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        array_size += uint32_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
      std::vector<uint32_t> x{};
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    } else if (safe_data() == array16) {
      safe_increment();
      std::size_t array_size = 0;
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        array_size += uint16_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    } else {
      std::size_t array_size = safe_data() & 0b00001111;
      safe_increment();
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    }
  }

  template<class T>
  void unpack_stdarray(T &array) {
    using ValueType = typename T::value_type;
    auto vec = std::vector<ValueType>{};
    unpack_array(vec);
    std::copy(vec.begin(), vec.end(), array.begin());
  }

  template<class T>
  void unpack_map(T &map) {
    using KeyType = typename T::key_type;
    using MappedType = typename T::mapped_type;
    if (safe_data() == map32) {
      safe_increment();
      std::size_t map_size = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        map_size += uint32_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    } else if (safe_data() == map16) {
      safe_increment();
      std::size_t map_size = 0;
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        map_size += uint16_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    } else {
      std::size_t map_size = safe_data() & 0b00001111;
      safe_increment();
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    }
  }

  void unpack_map(std::map<std::string, skip> /*map*/) {
    if (safe_data() == map32) {
      safe_increment();
      std::size_t map_size = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        map_size += uint32_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else if (safe_data() == map16) {
      safe_increment();
      std::size_t map_size = 0;
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        map_size += uint16_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else {
      safe_increment();
    }
  }

  template<typename T>
  void unpack_nvp(T &value) {
    std::string throwaway{};
    unpack_type(throwaway);
    unpack_type(value);
  }

  void unpack_type(int8_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case int8: {
        safe_increment();
        value = safe_data();
        safe_increment();
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(int16_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case int16: {
        safe_increment();
        std::bitset<16> bits;
        for (auto i = sizeof(uint16_t); i > 0; --i) {
          bits |= uint16_t(safe_data()) << 8 * (i - 1);
          safe_increment();
        }
        if (bits[15]) {
          value = -1 * (uint16_t((~bits).to_ulong()) + 1);
        } else {
          value = uint16_t(bits.to_ulong());
        }
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(int32_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case int32: {
        safe_increment();
        std::bitset<32> bits;
        for (auto i = sizeof(uint32_t); i > 0; --i) {
          bits |= uint32_t(safe_data()) << 8 * (i - 1);
          safe_increment();
        }
        if (bits[31]) {
          value = -1 * ((~bits).to_ulong() + 1);
        } else {
          value = bits.to_ulong();
        }
        break;
      }
      case int16: {
        int16_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint16: {
        uint16_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(int64_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case int64: {
        safe_increment();
        std::bitset<64> bits;
        for (auto i = sizeof(value); i > 0; --i) {
          bits |= std::bitset<8>(safe_data()).to_ullong() << 8 * (i - 1);
          safe_increment();
        }
        if (bits[63]) {
          value = -1 * ((~bits).to_ullong() + 1);
        } else {
          value = bits.to_ullong();
        }
        break;
      }
      case int32: {
        int32_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case int16: {
        int16_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint32: {
        uint32_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint16: {
        uint16_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(uint8_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case uint8: {
        safe_increment();
        value = safe_data();
        safe_increment();
        break;
      }
      case int8: {
        safe_increment();
        value = safe_data();
        safe_increment();
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(uint16_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case uint16: {
        safe_increment();
        for (auto i = sizeof(uint16_t); i > 0; --i) {
          value += safe_data() << 8 * (i - 1);
          safe_increment();
        }
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = static_cast<uint16_t>(val);
        break;
      }
      case int16: {
        int16_t val{};
        unpack_type(val);
        value = static_cast<uint16_t>(val);
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = static_cast<uint16_t>(val);
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(uint32_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case uint32: {
        safe_increment();
        for (auto i = sizeof(uint32_t); i > 0; --i) {
          value += safe_data() << 8 * (i - 1);
          safe_increment();
        }
        break;
      }
      case uint16: {
        uint16_t val{};
        unpack_type(val);
        value = static_cast<uint32_t>(val);
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = static_cast<uint32_t>(val);
        break;
      }
      case int32: {
        int32_t val{};
        unpack_type(val);
        value = static_cast<uint32_t>(val);
        break;
      }
      case int16: {
        int16_t val{};
        unpack_type(val);
        value = static_cast<uint32_t>(val);
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = static_cast<uint32_t>(val);
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(uint64_t &value) {
    auto flag = safe_data();
    switch (flag) {
      case uint64:
        safe_increment();
        for (auto i = sizeof(uint64_t); i > 0; --i) {
          value += uint64_t(safe_data()) << 8 * (i - 1);
          safe_increment();
        }
        break;
      case uint32: {
        uint32_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint16: {
        uint16_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case uint8: {
        uint8_t val{};
        unpack_type(val);
        value = val;
        break;
      }
      case int64: {
        int64_t val{};
        unpack_type(val);
        value = static_cast<uint64_t>(val);
        break;
      }
      case int32: {
        int32_t val{};
        unpack_type(val);
        value = static_cast<uint64_t>(val);
        break;
      }
      case int16: {
        int16_t val{};
        unpack_type(val);
        value = static_cast<uint64_t>(val);
        break;
      }
      case int8: {
        int8_t val{};
        unpack_type(val);
        value = static_cast<uint64_t>(val);
        break;
      }
      default:
        value = safe_data();
        safe_increment();
        break;
    }
  }

  void unpack_type(std::nullptr_t &/*value*/) {
    safe_increment();
  }

  void unpack_type(bool &value) {
    if (safe_data() == 0xc3) {
      value = true;
    } else if (safe_data() == 0xc2) {
      value = false;
    } else {
      ec = UnpackerError::BadInput;
    }
    safe_increment();
  }

  void unpack_type(float &value) {
    if (safe_data() == float32) {
      safe_increment();
      uint32_t data = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        data += safe_data() << 8 * (i - 1);
        safe_increment();
      }
      auto bits = std::bitset<32>(data);
      auto mantissa = 1.0f;
      for (auto i = 23U; i > 0; --i) {
        if (bits[i - 1]) {
          mantissa += 1.0f / (1 << (24 - i));
        }
      }
      if (bits[31]) {
        mantissa *= -1;
      }
      uint8_t exponent = 0;
      for (auto i = 0U; i < 8; ++i) {
        exponent += bits[i + 23] << i;
      }
      exponent -= 127;
      value = ldexp(mantissa, exponent);
    } else {
      if (safe_data() == int8 || safe_data() == int16 || safe_data() == int32 || safe_data() == int64) {
        int64_t val = 0;
        unpack_type(val);
        value = float(val);
      } else {
        uint64_t val = 0;
        unpack_type(val);
        value = float(val);
      }
    }
  }

  void unpack_type(double &value) {
    if (safe_data() == float64) {
      safe_increment();
      uint64_t data = 0;
      for (auto i = sizeof(uint64_t); i > 0; --i) {
        data += uint64_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
      auto bits = std::bitset<64>(data);
      auto mantissa = 1.0;
      for (auto i = 52U; i > 0; --i) {
        if (bits[i - 1]) {
          mantissa += 1.0 / (uint64_t(1) << (53 - i));
        }
      }
      if (bits[63]) {
        mantissa *= -1;
      }
      uint16_t exponent = 0;
      for (auto i = 0U; i < 11; ++i) {
        exponent += bits[i + 52] << i;
      }
      exponent -= 1023;
      value = ldexp(mantissa, exponent);
    } else {
      if (safe_data() == int8 || safe_data() == int16 || safe_data() == int32 || safe_data() == int64) {
        int64_t val = 0;
        unpack_type(val);
        value = float(val);
      } else {
        uint64_t val = 0;
        unpack_type(val);
        value = float(val);
      }
    }
  }

  void unpack_type(std::string &value) {
    std::size_t str_size = 0;
    if (safe_data() == str32) {
      safe_increment();
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        str_size += uint32_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else if (safe_data() == str16) {
      safe_increment();
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        str_size += uint16_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else if (safe_data() == str8) {
      safe_increment();
      for (auto i = sizeof(uint8_t); i > 0; --i) {
        str_size += uint8_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else {
      str_size = safe_data() & 0b00001111;
      safe_increment();
    }
    if (data_pointer + str_size <= data_end) {
      value = std::string{data_pointer, data_pointer + str_size};
      safe_increment(str_size);
    } else {
      ec = UnpackerError::OutOfRange;
    }
  }

  void unpack_type(std::vector<uint8_t> &value) {
    std::size_t bin_size = 0;
    if (safe_data() == bin32) {
      safe_increment();
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        bin_size += uint32_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else if (safe_data() == bin16) {
      safe_increment();
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        bin_size += uint16_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    } else {
      safe_increment();
      for (auto i = sizeof(uint8_t); i > 0; --i) {
        bin_size += uint8_t(safe_data()) << 8 * (i - 1);
        safe_increment();
      }
    }
    if (data_pointer + bin_size <= data_end) {
      value = std::vector<uint8_t>{data_pointer, data_pointer + bin_size};
      safe_increment(bin_size);
    } else {
      ec = UnpackerError::OutOfRange;
    }
  }
};

template<class PackableObject>
std::vector<uint8_t> pack(PackableObject &obj) {
  auto packer = Packer{};
  obj.pack(packer);
  return packer.vector();
}

template<class PackableObject>
std::vector<uint8_t> pack(PackableObject &&obj) {
  auto packer = Packer{};
  obj.pack(packer);
  return packer.vector();
}

template<typename PackableObject>
std::vector<uint8_t> nvp_pack(PackableObject &obj) {
  auto packer = Packer<true>{};
  obj.pack(packer);
  return packer.vector();
}

template<typename PackableObject>
std::vector<uint8_t> nvp_pack(PackableObject &&obj) {
  auto packer = Packer<true>{};
  obj.pack(packer);
  return packer.vector();
}

template<class UnpackableObject>
UnpackableObject unpack(const uint8_t *data_start, const std::size_t size, std::error_code &ec) {
  auto obj = UnpackableObject{};
  auto unpacker = Unpacker(data_start, size);
  obj.pack(unpacker);
  ec = unpacker.ec;
  return obj;
}

template<class UnpackableObject>
UnpackableObject unpack(const uint8_t *data_start, const std::size_t size) {
  std::error_code ec{};
  return unpack<UnpackableObject>(data_start, size, ec);
}

template<class UnpackableObject>
UnpackableObject unpack(const std::vector<uint8_t> &data, std::error_code &ec) {
  return unpack<UnpackableObject>(data.data(), data.size(), ec);
}

template<class UnpackableObject>
UnpackableObject unpack(const std::vector<uint8_t> &data) {
  std::error_code ec;
  return unpack<UnpackableObject>(data.data(), data.size(), ec);
}

template<class UnpackableObject>
UnpackableObject nvp_unpack(const uint8_t *data_start, const std::size_t size, std::error_code &ec) {
  auto obj = UnpackableObject{};
  auto unpacker = Unpacker<true>(data_start, size);
  obj.pack(unpacker);
  ec = unpacker.ec;
  return obj;
}

template<class UnpackableObject>
UnpackableObject nvp_unpack(const uint8_t *data_start, const std::size_t size) {
  std::error_code ec{};
  return nvp_unpack<UnpackableObject>(data_start, size, ec);
}

template<class UnpackableObject>
UnpackableObject nvp_unpack(const std::vector<uint8_t> &data, std::error_code &ec) {
  return nvp_unpack<UnpackableObject>(data.data(), data.size(), ec);
}

template<class UnpackableObject>
UnpackableObject nvp_unpack(const std::vector<uint8_t> &data) {
  std::error_code ec;
  return nvp_unpack<UnpackableObject>(data.data(), data.size(), ec);
}
}

#endif //CPPACK_PACKER_HPP
