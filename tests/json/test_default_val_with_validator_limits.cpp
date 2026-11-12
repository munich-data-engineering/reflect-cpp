#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// What does not change, and the limits of reading structs that contain a
// Validator without a valid default value.
namespace test_default_val_with_validator_limits {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

const std::string positive_error =
    "Value expected to be greater than or equal to 1, but got 0.";

std::string construction_error(const std::string& _type) {
  return "Could not default-construct "
         "test_default_val_with_validator_limits::" +
         _type +
         ", which is required because it contains rfl::DefaultVal. Give every "
         "field a valid default value. Original error: " +
         positive_error;
}

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

// A default member initializer that catches the exception of a Validator
// still sees it, as long as the struct can be default-constructed. If it
// cannot and its validation is deferred, that depends on where the compiler
// constructs the Validator that port_or_fallback() returns: outside of the
// struct, where it throws as usual (Positive is returned in registers under
// the Itanium ABI), or directly inside the struct, where its validation is
// deferred like that of any other field (MSVC).
inline Positive port_or_fallback() {
  try {
    return Positive();
  } catch (std::exception&) {
    return Positive(8080);
  }
}

struct Fallback {
  rfl::DefaultVal<std::string> host = "localhost";
  Positive port = port_or_fallback();
};

struct FallbackAndInvalid {
  Positive port = port_or_fallback();
  Positive retries;
};

TEST(json, test_default_val_with_validator_limits_fallback) {
  const auto res = rfl::json::read<Fallback, rfl::DefaultIfMissing>(R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::json::write(*res), R"({"host":"localhost","port":8080})");

  const auto res2 = rfl::json::read<FallbackAndInvalid, rfl::DefaultIfMissing>(
      R"({"retries": 2})");
  if (res2) {
    EXPECT_EQ(rfl::json::write(*res2), R"({"port":8080,"retries":2})");
  } else {
    EXPECT_EQ(std::string(res2.error().what()),
              "Field named 'port' not found, and its default value is "
              "invalid: " +
                  positive_error);
  }

  // If the input contains the field, it does not matter.
  const auto res3 = rfl::json::read<FallbackAndInvalid, rfl::DefaultIfMissing>(
      R"({"port": 443, "retries": 2})");
  ASSERT_TRUE(res3) << res3.error().what();
  EXPECT_EQ(rfl::json::write(*res3), R"({"port":443,"retries":2})");
}

// A read inside a default member initializer has its own invalid default
// values deferred, not those of the struct around it.
struct WithNestedRead {
  Person person =
      rfl::json::read<Person>(R"({"favorite_foods": ["beer"]})").value();
  rfl::DefaultVal<Positive> retries;
};

TEST(json, test_default_val_with_validator_limits_nested_read) {
  const auto res = rfl::json::read<WithNestedRead, rfl::DefaultIfMissing>(
      R"({"retries": 2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->person.favorite_foods->at(0), "beer");
  EXPECT_EQ(res->retries.value().value(), 2);
}

// Validators that are default-constructed on other threads still throw.
TEST(json, test_default_val_with_validator_limits_threads) {
  std::atomic<bool> done = false;
  std::atomic<int> constructed = 0;
  auto other = std::thread([&] {
    while (!done) {
      try {
        Foods foods;
        ++constructed;
      } catch (std::exception&) {
      }
    }
  });
  auto readers = std::vector<std::thread>();
  std::atomic<int> failed_reads = 0;
  for (int i = 0; i < 2; ++i) {
    readers.emplace_back([&] {
      for (int j = 0; j < 2000; ++j) {
        if (!rfl::json::read<Person>(R"({"favorite_foods": ["donuts"]})")) {
          ++failed_reads;
        }
      }
    });
  }
  for (auto& reader : readers) {
    reader.join();
  }
  done = true;
  other.join();
  EXPECT_EQ(failed_reads, 0);
  EXPECT_EQ(constructed, 0);
}

// The regressions found in the review of an earlier attempt to fix upstream
// issue #700.
struct WithMap {
  rfl::DefaultVal<int> x;
  std::map<Positive, std::string> m;
};

struct NoDefaultConstructor {
  NoDefaultConstructor(int _v) : v_(_v) {}
  using ReflectionType = int;
  int reflection() const { return v_; }
  int v_;
};

struct NotDefaultConstructible {
  NoDefaultConstructor n;
  Positive p;
};

static_assert(!std::is_default_constructible_v<NotDefaultConstructible>);

struct ConstValidator {
  const rfl::Validator<unsigned, rfl::Minimum<1>> age;
  rfl::DefaultVal<std::string> name = "Maggie";
};

TEST(json, test_default_val_with_validator_limits_earlier_regressions) {
  EXPECT_FALSE(rfl::json::read<WithMap>(R"({"m": {"0": "zero"}})"));
  const auto map = rfl::json::read<WithMap>(R"({"m": {"1": "one"}})");
  ASSERT_TRUE(map) << map.error().what();
  EXPECT_EQ(map->m.begin()->first.value(), 1);

  const auto ndc =
      rfl::json::read<NotDefaultConstructible>(R"({"n": 3, "p": 4})");
  ASSERT_TRUE(ndc) << ndc.error().what();
  EXPECT_EQ(ndc->p.value(), 4);

  const auto con = rfl::json::read<ConstValidator>(R"({"age": 8})");
  ASSERT_TRUE(con) << con.error().what();
  EXPECT_EQ(rfl::json::write(*con), R"({"age":8,"name":"Maggie"})");
}

// A class that is not reflectable, kept out of reading by rfl::Skip.
class Cache {
 public:
  int hits() const { return hits_; }

 private:
  int hits_ = 0;
  std::string last_;
};

struct WithCache {
  rfl::DefaultVal<int> port = 8080;
  rfl::Skip<Cache> cache;
  Positive retries;
};

struct Schema {
  rfl::DefaultVal<bool> nullable = false;
  std::optional<std::vector<Schema>> all_of;
  std::optional<std::map<std::string, Schema>> properties;
  std::unique_ptr<Schema> items;
};

TEST(json, test_default_val_with_validator_limits_other_types) {
  const auto cache = rfl::json::read<WithCache>(R"({"retries": 1})");
  ASSERT_TRUE(cache) << cache.error().what();
  EXPECT_EQ(cache->port.value(), 8080);

  const auto schema = rfl::json::read<Schema>(
      R"({"properties": {"a": {"nullable": true}}, "items": {}})");
  ASSERT_TRUE(schema) << schema.error().what();
  EXPECT_TRUE(schema->properties->at("a").nullable.value());
  EXPECT_FALSE(schema->items->nullable.value());
}

// The JSON schema leaves out the default values of a struct that cannot be
// default-constructed.
struct WithPerson {
  int id;
  Person person;
};

TEST(json, test_default_val_with_validator_limits_to_schema) {
  const auto schema = rfl::json::to_schema<Person>();
  EXPECT_NE(schema.find(R"("favorite_foods":{"type":"array")"),
            std::string::npos)
      << schema;
  EXPECT_EQ(schema.find("Homer Simpson"), std::string::npos) << schema;

  const auto nested = rfl::json::to_schema<WithPerson>();
  EXPECT_NE(nested.find(R"("person":{"$ref":)"), std::string::npos) << nested;
}

// [[no_unique_address]] can place a field in the tail padding of another.
struct InTailPadding {
  rfl::Validator<int64_t, rfl::Minimum<0>> a;
  int8_t c = 0;
};

struct SharesPadding {
  [[no_unique_address]] InTailPadding inner;
  rfl::DefaultVal<Positive> p;
};

TEST(json, test_default_val_with_validator_limits_no_unique_address) {
  const auto res =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().details().at(0).path, Path({"p"}));

  const auto res2 =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}, "p": 3})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->p.value().value(), 3);
}

// A field in the input replaces the default value as a whole. Should a field
// ever be read into its default value instead, the invalid default of p would
// survive here.
struct Inner {
  Positive p;
  rfl::DefaultVal<int> q;
};

struct Outer {
  rfl::DefaultVal<int> x;
  Inner inner;
};

TEST(json, test_default_val_with_validator_limits_found_fields_are_replaced) {
  const auto res =
      rfl::json::read<Outer, rfl::DefaultIfMissing>(R"({"inner": {"q": 1}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().details().at(0).path, Path({"inner", "p"}));
}

// Only a Validator that is default-constructed inside the struct can be
// deferred. Anywhere else, it throws as before, and so does T{}.
struct OnTheHeap {
  rfl::DefaultVal<int> x;
  std::vector<Positive> v = std::vector<Positive>(1);
};

struct FromTemporary {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p = Positive();
};

struct InRflTuple {
  rfl::DefaultVal<int> x;
  rfl::Tuple<Positive, int> t;
};

TEST(json, test_default_val_with_validator_limits_not_in_place) {
  const auto heap = rfl::json::read<OnTheHeap>(R"({"v": [2]})");
  ASSERT_FALSE(heap);
  EXPECT_EQ(heap.error().what(), construction_error("OnTheHeap"));

  const auto temp = rfl::json::read<FromTemporary>(R"({"p": 2})");
  ASSERT_FALSE(temp);
  EXPECT_EQ(temp.error().what(), construction_error("FromTemporary"));

  const auto tuple = rfl::json::read<InRflTuple>(R"({"t": [2, 3]})");
  ASSERT_FALSE(tuple);
  EXPECT_EQ(tuple.error().what(), construction_error("InRflTuple"));
}

// A field that is never read keeps its default value.
struct SkipsValidator {
  rfl::DefaultVal<int> x;
  rfl::Skip<Positive> s;
};

TEST(json, test_default_val_with_validator_limits_skip) {
  const auto res = rfl::json::read<SkipsValidator>(R"({"x": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 's' not found, and its default value is invalid: " +
                positive_error);
}

// Known gap: a default member initializer that copies a Validator whose
// default value is invalid. Outside of read, Copies{} throws, but the copy
// in b is not checked, so the read succeeds with an invalid value.
struct Copies {
  Positive a;
  rfl::DefaultVal<Positive> b = a;
};

TEST(json, test_default_val_with_validator_limits_copy_is_not_checked) {
  EXPECT_THROW((void)Copies{}, std::exception);
  const auto res = rfl::json::read<Copies>(R"({"a": 5})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->a.value(), 5);
  EXPECT_EQ(res->b.value().value(), 0);
}

}  // namespace test_default_val_with_validator_limits
