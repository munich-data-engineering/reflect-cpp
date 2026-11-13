#include <rfl.hpp>
#include <string>
#include <variant>

#include "write_and_read.hpp"

namespace test_monostate {

struct Order {
  std::variant<std::monostate, std::string, int> price;
};

struct RflOrder {
  rfl::Variant<std::monostate, std::string, int> price;
};

TEST(avro, test_monostate) {
  write_and_read(Order{.price = std::monostate{}});
  write_and_read(Order{.price = "1.25"});
  write_and_read(Order{.price = 2});
}

TEST(avro, test_monostate_rfl_variant) {
  write_and_read(RflOrder{.price = std::monostate{}});
  write_and_read(RflOrder{.price = std::string("1.25")});
  write_and_read(RflOrder{.price = 2});
}

}  // namespace test_monostate
