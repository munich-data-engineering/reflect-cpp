#ifndef RFL_INTERNAL_BASE64_HPP_
#define RFL_INTERNAL_BASE64_HPP_

#include <cstddef>
#include <string>
#include <string_view>

#include "../Bytestring.hpp"
#include "../Result.hpp"

namespace rfl::internal {

/// Encodes bytes using the standard Base64 alphabet and padding (RFC 4648).
inline std::string encode_base64(const Bytestring& _bytes) {
  constexpr std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string encoded;
  encoded.reserve((_bytes.size() / 3 + (_bytes.size() % 3 != 0)) * 4);
  unsigned int buffer = 0;
  unsigned int bits = 0;
  for (const auto byte : _bytes) {
    buffer = (buffer << 8) | std::to_integer<unsigned int>(byte);
    bits += 8;
    while (bits >= 6) {
      bits -= 6;
      encoded.push_back(alphabet[(buffer >> bits) & 0x3f]);
    }
  }
  if (bits != 0) {
    encoded.push_back(alphabet[(buffer << (6 - bits)) & 0x3f]);
  }
  while (encoded.size() % 4 != 0) {
    encoded.push_back('=');
  }
  return encoded;
}

/// Accepts standard and URL-safe Base64, with or without padding, as in
/// ProtoJSON. Invalid characters, lengths and padding are rejected.
inline Result<Bytestring> decode_base64(const std::string_view _encoded) {
  auto size = _encoded.size();
  while (size != 0 && _encoded[size - 1] == '=') {
    --size;
  }
  const auto padding = _encoded.size() - size;
  if (size % 4 == 1 || padding > 2 ||
      (padding != 0 && _encoded.size() % 4 != 0)) {
    return error("Invalid Base64 length or padding.");
  }

  Bytestring decoded;
  decoded.reserve(size / 4 * 3 + (size % 4 * 6) / 8);
  unsigned int buffer = 0;
  unsigned int bits = 0;
  for (size_t i = 0; i < size; ++i) {
    const auto c = _encoded[i];
    unsigned int value;
    if (c >= 'A' && c <= 'Z') {
      value = static_cast<unsigned int>(c - 'A');
    } else if (c >= 'a' && c <= 'z') {
      value = static_cast<unsigned int>(c - 'a') + 26;
    } else if (c >= '0' && c <= '9') {
      value = static_cast<unsigned int>(c - '0') + 52;
    } else if (c == '+' || c == '-') {
      value = 62;
    } else if (c == '/' || c == '_') {
      value = 63;
    } else {
      return error("Invalid character in Base64 string.");
    }
    buffer = (buffer << 6) | value;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      decoded.push_back(static_cast<std::byte>((buffer >> bits) & 0xff));
    }
  }
  return decoded;
}

}  // namespace rfl::internal

#endif
