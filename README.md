[![Build Status](https://travis-ci.com/mikeloomisgg/cppack.svg?branch=master)](https://travis-ci.com/mikeloomisgg/cppack)
# cppack
A modern (c++17 required) implementation of the [msgpack spec](https://github.com/msgpack/msgpack/blob/master/spec.md).

Msgpack is a binary serialization specification. It allows you to save and load application objects like classes and structs over networks, to files, and between programs and even different languages.

Check out [this blog](https://mikeloomisgg.github.io/2019-07-02-making-a-serialization-library/) for my rational creating this library.

## Features
- Fast and compact
- Full test coverage
- Easy to use
- Automatic type handling
- Open source MIT license
- Easy error handling

### Single Header only template library
Want to use this library? Just #include the header and you're good to go. You can also install it like a normal cmake package if you know how that works.

### Cereal style packaging
Easily pack objects into byte arrays using a pack free function:

Be aware that you will need to use the nvp_pack (name value pair) methods to have interop with python or javascript msgpack libraries.

```c++
struct Person {
  std::string name;
  uint16_t age;
  std::vector<std::string> aliases;

  template<class T>
  void pack(T &pack) {
    pack(name, age, aliases);
  }
};

int main() {
    auto person = Person{"John", 22, {"Ripper", "Silverhand"}};

    auto data = msgpack::pack(person); // Pack your object
    auto john = msgpack::unpack<Person>(data.data()); // Unpack it
}
```

[More Examples](msgpack/tests/examples.cpp)


### Roadmap
- Support for extension types
  - The msgpack spec allows for additional types to be enumerated as Extensions. If reasonable use cases come about for this feature then it may be added.
- Support for unpacking types memberwise with callbacks for async reading of packed objects
- Endian conversion shortcuts
  - On platforms that already hold types in big endian, the serialization could be optimized using type traits.
