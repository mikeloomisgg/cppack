//
// Created by Mike Loomis on 6/22/2019.
//

#ifndef CPPACK_PACKER_HPP
#define CPPACK_PACKER_HPP

#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <any>

namespace msgpack {

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
struct is_container<std::set<T, Alloc> > {
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

class Packer {
 public:
  template<class ... Types>
  void process(Types &&... args) {
    std::clog << "Attempting to pack " << sizeof...(args) << " values.\n";
    (process_type(std::forward<Types>(args)), ...);
  }

  std::vector<uint8_t> vector() const {
    return serialized_object;
  }

 private:
  std::vector<uint8_t> serialized_object;

  template<class T>
  void process_type(T &value) {
    if (is_map<T>::value) {
      processMap(std::forward<T &>(value));
    } else if (is_container<T>::value) {
      processArray(std::forward<T &>(value));
    } else {
      std::clog << "Packing value type: " << "Unknown" << '\n';
    }
  }

  template<class T>
  void processArray(T &array) {
    std::clog << "Packing value type: " << "object array" << '\n';
  }

  template<class T>
  void processMap(T &map) {
    std::clog << "Packing value type: " << "map" << '\n';
  }
};

template<>
void Packer::process_type(int8_t &value) {
  std::clog << "Packing value type: " << "int8_t" << '\n';
}

template<>
void Packer::process_type(int16_t &value) {
  std::clog << "Packing value type: " << "int16_t" << '\n';
}

template<>
void Packer::process_type(int32_t &value) {
  std::clog << "Packing value type: " << "int32_t" << '\n';
}

template<>
void Packer::process_type(int64_t &value) {
  std::clog << "Packing value type: " << "int64_t" << '\n';
}

template<>
void Packer::process_type(uint8_t &value) {
  std::clog << "Packing value type: " << "uint8_t" << '\n';
}

template<>
void Packer::process_type(uint16_t &value) {
  std::clog << "Packing value type: " << "uint16_t" << '\n';
}

template<>
void Packer::process_type(uint32_t &value) {
  std::clog << "Packing value type: " << "uint32_t" << '\n';
}

template<>
void Packer::process_type(uint64_t &value) {
  std::clog << "Packing value type: " << "uint64_t" << '\n';
}

template<>
void Packer::process_type(std::nullptr_t &value) {
  std::clog << "Packing value type: " << "std::nullptr_t" << '\n';
}

template<>
void Packer::process_type(bool &value) {
  std::clog << "Packing value type: " << "boolean" << '\n';
}

template<>
void Packer::process_type(float &value) {
  std::clog << "Packing value type: " << "float" << '\n';
}

template<>
void Packer::process_type(double &value) {
  std::clog << "Packing value type: " << "double" << '\n';
}

template<>
void Packer::process_type(std::string &value) {
  std::clog << "Packing value type: " << "string" << '\n';
}

template<>
void Packer::process_type(std::vector<uint8_t> &value) {
  std::clog << "Packing value type: " << "byte array" << '\n';
}

class Unpacker {
 public:
  explicit Unpacker(uint8_t *data_start) {};

  template<class ... Types>
  void process(Types &&... args) {
    std::clog << "Attempting to unpack " << sizeof...(args) << " values.\n";
    (process_type(std::forward<Types>(args)), ...);
  }

 private:
  template<class T>
  void process_type(T &value) {
    if (is_map<T>::value) {
      processMap(std::forward<T &>(value));
    } else if (is_container<T>::value) {
      processArray(std::forward<T &>(value));
    } else {
      std::clog << "Unpacking value type: " << "Unknown" << '\n';
    }
  }

  template<class T>
  void processArray(T &array) {
    std::clog << "Unpacking value type: " << "object array" << '\n';
  }

  template<class T>
  void processMap(T &map) {
    std::clog << "Unpacking value type: " << "map" << '\n';
  }
};

template<>
void Unpacker::process_type(int8_t &value) {
  std::clog << "Unpacking value type: " << "int8_t" << '\n';
}

template<>
void Unpacker::process_type(int16_t &value) {
  std::clog << "Unpacking value type: " << "int16_t" << '\n';
}

template<>
void Unpacker::process_type(int32_t &value) {
  std::clog << "Unpacking value type: " << "int32_t" << '\n';
}

template<>
void Unpacker::process_type(int64_t &value) {
  std::clog << "Unpacking value type: " << "int64_t" << '\n';
}

template<>
void Unpacker::process_type(uint8_t &value) {
  std::clog << "Unpacking value type: " << "uint8_t" << '\n';
}

template<>
void Unpacker::process_type(uint16_t &value) {
  std::clog << "Unpacking value type: " << "uint16_t" << '\n';
}

template<>
void Unpacker::process_type(uint32_t &value) {
  std::clog << "Unpacking value type: " << "uint32_t" << '\n';
}

template<>
void Unpacker::process_type(uint64_t &value) {
  std::clog << "Unpacking value type: " << "uint64_t" << '\n';
}

template<>
void Unpacker::process_type(std::nullptr_t &value) {
  std::clog << "Unpacking value type: " << "std::nullptr_t" << '\n';
}

template<>
void Unpacker::process_type(bool &value) {
  std::clog << "Unpacking value type: " << "boolean" << '\n';
}

template<>
void Unpacker::process_type(float &value) {
  std::clog << "Unpacking value type: " << "float" << '\n';
}

template<>
void Unpacker::process_type(double &value) {
  std::clog << "Unpacking value type: " << "double" << '\n';
}

template<>
void Unpacker::process_type(std::string &value) {
  std::clog << "Unpacking value type: " << "string" << '\n';
}

template<>
void Unpacker::process_type(std::vector<uint8_t> &value) {
  std::clog << "Unpacking value type: " << "byte array" << '\n';
}

template<class PackableObject>
std::vector<uint8_t> pack(PackableObject &&obj) {
  auto vec = std::vector<uint8_t>{};
  auto packer = Packer{};
  obj.pack(packer);
  return packer.vector();
}

template<class UnpackableObject>
UnpackableObject unpack(uint8_t *data_start) {
  auto obj = UnpackableObject{};
  auto unpacker = Unpacker(data_start);
  obj.pack(unpacker);
  return obj;
}
}

#endif //CPPACK_PACKER_HPP
