#ifndef MYLD_H
#define MYLD_H
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t i32;
typedef int64_t i64;

template <typename T> std::vector<u8> to_bytes(T data) {
    const char *ptr = reinterpret_cast<char *>(&data);
    std::vector<u8> v(ptr, ptr + sizeof(T));
    return v;
}

class Raw {
  public:
    Raw(std::shared_ptr<const std::vector<u8>> raw) : raw(raw), offset(0), size(raw->size()) {}

    Raw get_sub(u64 offset_, u64 size_) {
        assert(offset + offset + size_ < raw->size());
        return Raw(raw, offset + offset_, size_);
    }

    inline u8 operator[](std::size_t index) const { return (*raw)[index]; }

    u8 *to_pointer() const { return (u8 *)&(*raw)[offset]; }

    std::vector<u8> to_vec() {
        std::vector<u8> v1;
        v1.reserve(size);
        for (u64 u = 0; u < size; u++) {
            v1.push_back((*raw)[offset + u]);
        }
        return v1;
    }

  private:
    Raw(std::shared_ptr<const std::vector<u8>> raw, u64 offset, u64 size) : raw(raw), offset(offset), size(size) {}

    std::shared_ptr<const std::vector<u8>> raw;
    u64 offset;
    u64 size;
};

#endif
