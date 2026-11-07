#ifndef RFL_PARSING_ARRAYREADER_HPP_
#define RFL_PARSING_ARRAYREADER_HPP_

#include <array>
#include <optional>
#include <string>
#include <type_traits>

#include "../Result.hpp"
#include "../internal/is_array.hpp"
#include "Parser_base.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class ProcessorsType, class T, size_t _size>
class ArrayReader {
 private:
  using InputVarType = typename R::InputVarType;
  static constexpr size_t size_ = _size;

 public:
  /**
   * @brief Constructor.
   * @param _r The reader to use.
   * @param _array The array to read into.
   */
  ArrayReader(const R* _r, std::array<T, _size>* _array)
      : array_(_array), num_set_(0), r_(_r) {}

  ~ArrayReader() = default;

  /**
   * @brief Checks if the correct number of elements has been read.
   * @return An error if the number of elements is incorrect, std::nullopt otherwise.
   */
  std::optional<Error> check_size() const {
    if (num_set_ != size_) {
      return Error("Expected " + std::to_string(size_) + " elements, got " +
                   std::to_string(num_set_) + ".");
    }
    return std::nullopt;
  }

  /**
   * @brief Returns the number of elements that have been set.
   * @return The number of elements.
   */
  size_t num_set() const { return num_set_; }

  /**
   * @brief Reads a variable from the input and adds it to the array.
   * @param _var The input variable to read.
   * @return An error if reading fails or if the array is already full, std::nullopt otherwise.
   */
  std::optional<Error> read(const InputVarType& _var) const {
    if (num_set_ == size_) {
      return Error("Expected " + std::to_string(size_) +
                   " elements, got at least " + std::to_string(size_ + 1) +
                   ".");
    }
    auto res =
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(*r_, _var);
    if (res) {
      move_to(&((*array_)[num_set_]), &(*res));
    } else {
      return wrap_error(
          "Failed to parse element " + std::to_string(num_set_) + ": ",
          num_set_, std::move(res).error());
    }
    ++num_set_;
    return std::optional<Error>();
  }

 private:
  /**
   * @brief Moves the value from Source to Target using placement new.
   * @tparam Target The target type.
   * @tparam Source The source type.
   * @param _t The target pointer.
   * @param _s The source pointer.
   */
  template <class Target, class Source>
  void move_to(Target* _t, Source* _s) const {
    if constexpr (std::is_const_v<Target>) {
      return move_to(const_cast<std::remove_const_t<Target>*>(_t), _s);
    } else if constexpr (!internal::is_array_v<Source> &&
                         !std::is_array_v<Target>) {
      ::new (_t) Target(std::move(*_s));
    } else if constexpr (internal::is_array_v<Source>) {
      for (size_t i = 0; i < _s->arr_.size(); ++i) {
        move_to(&((*_t)[i]), &(_s->arr_[i]));
      }
    } else {
      for (size_t i = 0; i < _s->size(); ++i) {
        move_to(&((*_t)[i]), &((*_s)[i]));
      }
    }
  }

 private:
  /// The underlying array.
  std::array<T, size_>* array_;

  /// Indicates the current field
  mutable size_t num_set_;

  /// The underlying reader.
  const R* r_;
};

}  // namespace rfl::parsing

#endif
