#ifndef RFL_INTERNAL_BITSET_HPP_
#define RFL_INTERNAL_BITSET_HPP_

#include <bitset>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "../Result.hpp"

namespace rfl::internal {

template <std::size_t N>
Result<std::bitset<N>> bitset_from_string(const std::string_view _str) {
  if (_str.size() != N) {
    std::string message = "Expected a bitset string of length ";
    message += std::to_string(N);
    message += ", got ";
    message += std::to_string(_str.size());
    message += ".";
    return error(std::move(message));
  }
  std::bitset<N> bits;
  for (std::size_t i = 0; i < N; ++i) {
    if (_str[i] != '0' && _str[i] != '1') {
      return error("Bitset strings may contain only '0' and '1'.");
    }
    bits[N - 1 - i] = _str[i] == '1';
  }
  return bits;
}

// Bit zero occupies the most significant bit of the first byte, matching
// Cereal's packed payload. Never access the implementation's bitset storage.
template <class Bytes, std::size_t N>
Bytes bitset_to_bytes(const std::bitset<N>& _bits) {
  Bytes bytes{};
  if constexpr (requires { bytes.resize(0); }) {
    bytes.resize(N / 8 + (N % 8 != 0));
  }
  for (std::size_t i = 0; i < N; ++i) {
    if (_bits[i]) {
      bytes[i / 8] |= static_cast<typename Bytes::value_type>(0x80u >> (i % 8));
    }
  }
  return bytes;
}

template <std::size_t N, class Bytes>
Result<std::bitset<N>> bitset_from_bytes(const Bytes& _bytes) {
  constexpr auto size = N / 8 + (N % 8 != 0);
  if (_bytes.size() != size) {
    std::string message = "Expected ";
    message += std::to_string(size);
    message += " packed bitset bytes, got ";
    message += std::to_string(_bytes.size());
    message += ".";
    return error(std::move(message));
  }
  if constexpr (N % 8 != 0) {
    constexpr auto mask = (1u << (8 - N % 8)) - 1;
    if ((static_cast<unsigned char>(_bytes.back()) & mask) != 0) {
      return error("Packed bitset padding bits must be zero.");
    }
  }
  std::bitset<N> bits;
  for (std::size_t i = 0; i < N; ++i) {
    bits[i] =
        (static_cast<unsigned char>(_bytes[i / 8]) & (0x80u >> (i % 8))) != 0;
  }
  return bits;
}

}  // namespace rfl::internal

#endif
