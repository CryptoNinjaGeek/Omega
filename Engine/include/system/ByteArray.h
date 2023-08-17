#pragma once

#include <system/Global.h>

namespace omega {
namespace system {
template<typename T>
class ByteArray {
 public:
  ByteArray() = default;
  ByteArray(T* data, unsigned int size) {
    _data = data;
    _size = size;
  };

  auto setSize(unsigned int size) {
    _size = size;
    _data = std::shared_ptr<T[]>(new T[_size]);
  };

  const T* constData() { return _data; }
  T* data() { return _data.get(); }
  unsigned int size() { return _size; }

 private:
  unsigned int _size{0};
  std::shared_ptr<T[]> _data{nullptr};
};
}  // namespace system
}  // namespace omega
