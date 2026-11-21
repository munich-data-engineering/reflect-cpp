#ifndef RFL_INTERNAL_CPP20_RESULT_HPP_
#define RFL_INTERNAL_CPP20_RESULT_HPP_

#include <array>
#include <functional>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "../Error.hpp"
#include "../always_inline.hpp"

namespace rfl::internal::cpp20 {

template <class E>
struct Unexpected {
  /// Constructs an Unexpected object with the given error (perfect forwarding).
  /// @param _err The error to store in this Unexpected object.
  Unexpected(E&& _err) : err_{std::forward<E>(_err)} {}

  /// Constructs an Unexpected object with the given error (copy).
  /// @param _err The error to store in this Unexpected object (copy)
  Unexpected(const E& _err) : err_{_err} {}

  /// Move constructor.
  /// @param _other The Unexpected object to move from
  Unexpected(Unexpected&&) = default;

  /// Copy constructor.
  /// @param _other The Unexpected object to copy from
  Unexpected(const Unexpected&) = default;

  /// Move assignment operator.
  /// @param _other The Unexpected object to move from
  Unexpected& operator=(Unexpected&&) = default;

  /// Copy assignment operator.
  /// @param _other The Unexpected object to copy from
  Unexpected& operator=(const Unexpected&) = default;

  /// Returns the error stored in this Unexpected object (const reference).
  const E& error() const& { return err_; }

  /// Returns the error stored in this Unexpected object (rvalue reference).
  E&& error() && { return std::move(err_); }

  /// Returns the error stored in this Unexpected object (lvalue reference).
  E& error() & { return err_; }

 private:
  E err_;
};

/// The Result class is used for monadic error handling.
/// It can either contain a value of type T (indicating success) or an Error
/// (indicating failure). The class provides various methods for chaining
/// operations, transforming values, and handling errors in a functional style.
template <class T>
class Result {
  static_assert(!std::is_same<T, Error>(), "The result type cannot be Error.");

  using TOrErr = std::array<unsigned char, std::max(sizeof(T), sizeof(Error))>;

 public:
  using value_type = T;
  using error_type = Error;
  /// @brief Constructs a successful Result holding a copy of the given value.
  /// @param _val The value to copy into the Result.
  Result(const T& _val) : success_(true) { new (&get_t()) T(_val); }

  /// @brief Constructs a successful Result by moving the given value.
  /// @param _val The value to move into the Result.
  Result(T&& _val) noexcept : success_(true) {
    new (&get_t()) T(std::move(_val));
  }

  /// @brief Constructs an unsuccessful Result holding a copy of the given
  /// error.
  /// @param _err The Unexpected error to copy into the Result.
  Result(const Unexpected<Error>& _err) : success_(false) {
    new (&get_err()) Error(_err.error());
  }

  /// @brief Constructs an unsuccessful Result by moving the given error.
  /// @param _err The Unexpected error to move into the Result.
  Result(Unexpected<Error>&& _err) : success_(false) {
    new (&get_err()) Error(std::move(_err.error()));
  }

  /// @brief Move constructor. Transfers ownership of the value or error from
  /// another Result.
  /// @param _other The Result to move from. After the move, _other is in a
  /// valid but unspecified state.
  Result(Result<T>&& _other) noexcept : success_(_other.success_) {
    move_from_other(_other);
  }

  /// @brief Copy constructor for Result.
  /// @param _other The other Result object to copy from.
  /// @details Copies the success state and the contained value (if any) from
  /// another Result of the same type.
  Result(const Result<T>& _other) : success_(_other.success_) {
    copy_from_other(_other);
  }

  /// @brief Move constructor template for Result from a convertible type.
  /// @tparam U The type of the value contained in the other Result.
  /// @param _other The other Result object to move from.
  /// @details Constructs a Result<T> by moving from a Result<U>, where U is
  /// convertible to T. The contained value is transformed and moved into the
  /// new Result. If the other Result contains an error,
  /// the error is propagated to this Result.
  template <class U>
    requires std::is_convertible_v<U, T>
  Result(Result<U>&& _other) : success_(_other && true) {
    auto temp = std::forward<Result<U> >(_other).transform(
        [](U&& _u) { return T(std::forward<U>(_u)); });
    move_from_other(temp);
  }

  /// @brief Copy constructor template for Result from a convertible type.
  /// @tparam U The type of the value contained in the other Result.
  /// @param _other The other Result object to copy from.
  /// @details Constructs a Result<T> by copying from a Result<U>, where U is
  /// convertible to T. The contained value is transformed and copied into the
  /// new Result. If the other Result contains an error, the error is propagated
  /// to this Result.
  template <class U>
    requires std::is_convertible_v<U, T>
  Result(const Result<U>& _other) : success_(_other && true) {
    auto temp = _other.transform([](const U& _u) { return T(_u); });
    move_from_other(temp);
  }

  /// @brief Destructor for Result.
  /// @details Destroys the contained value (if any) and releases any resources
  /// held by the Result.
  ~Result() { destroy(); }

  /// @brief Monadic operation that applies a function to the contained value if
  /// the result is successful.
  ///
  /// This overload is for rvalue-qualified objects (i.e., temporaries or
  /// moved-from objects). If the Result holds a value (success_), the provided
  /// function F is invoked with the value (moved), and its result (which must
  /// be another Result<U>) is returned. If the Result holds an error, the error
  /// is propagated to the returned Result<U>.
  ///
  /// @tparam F A callable type accepting T and returning Result<U>.
  /// @param _f The function to apply to the contained value if present.
  /// @return Result<U> The result of applying _f to the value, or propagating
  /// the error.
  template <class F>
  auto and_then(const F& _f) && {
    /// Result_U is expected to be of type Result<U>.
    using Result_U = typename std::invoke_result<F, T>::type;
    if (success_) {
      return Result_U(_f(std::move(*this).get_t()));
    } else {
      return Result_U(std::move(*this).get_err());
    }
  }

  /// @brief Monadic operation that applies a function to the contained value if
  /// the result is successful.
  ///
  /// This overload is for const lvalue-qualified objects (i.e., non-modifiable
  /// references). If the Result holds a value (success_), the provided function
  /// F is invoked with the value, and its result (which must be another
  /// Result<U>) is returned. If the Result holds an error, the error is
  /// propagated to the returned Result<U>.
  ///
  /// @tparam F A callable type accepting T and returning Result<U>.
  /// @param _f The function to apply to the contained value if present.
  /// @return Result<U> The result of applying _f to the value, or propagating
  /// the error.
  template <class F>
  auto and_then(const F& _f) const& {
    /// Result_U is expected to be of type Result<U>.
    using Result_U = typename std::invoke_result<F, T>::type;
    if (success_) {
      return Result_U(_f(get_t()));
    } else {
      return Result_U(get_err());
    }
  }

  /// Returns true if the result contains a value, false otherwise.
  operator bool() const noexcept { return success_; }

  /// Allows access to the underlying value. Careful: Will result in undefined
  /// behavior, if the result contains an error.
  T&& operator*() && noexcept { return std::move(*this).get_t(); }

  /// Allows access to the underlying value. Careful: Will result in undefined
  /// behavior, if the result contains an error.
  T& operator*() & noexcept { return get_t(); }

  /// Allows read access to the underlying value. Careful: Will result in
  /// undefined behavior, if the result contains an error.
  const T& operator*() const& noexcept { return get_t(); }

  /// @brief Copy assignment operator.
  ///
  /// Assigns the contents of another Result<T> to this object. If the source
  /// and destination are the same, no action is taken. Otherwise, destroys the
  /// current contents, copies the success state, and copies the underlying
  /// value or error from the other object.
  ///
  /// @param _other The Result<T> object to copy from.
  /// @return Reference to this Result<T>.
  Result<T>& operator=(const Result<T>& _other) {
    if (this == &_other) {
      return *this;
    }
    destroy();
    success_ = _other.success_;
    copy_from_other(_other);
    return *this;
  }

  /// @brief Move assignment operator.
  ///
  /// Assigns the contents of another Result<T> to this object by moving. If the
  /// source and destination are the same, no action is taken. Otherwise,
  /// destroys the current contents, copies the success state, and moves the
  /// underlying value or error from the other object.
  ///
  /// @param _other The Result<T> object to move from.
  /// @return Reference to this Result<T>.
  Result<T>& operator=(Result<T>&& _other) noexcept {
    if (this == &_other) {
      return *this;
    }
    destroy();
    success_ = _other.success_;
    move_from_other(_other);
    return *this;
  }

  /// @brief Move assignment from Unexpected<Error>.
  ///
  /// Assigns an error to this Result<T> by moving from an Unexpected<Error>
  /// object. Destroys the current contents, sets the state to failure, and
  /// constructs the error in place.
  ///
  /// @param _err The Unexpected<Error> object to move the error from.
  /// @return Reference to this Result<T>.
  Result<T>& operator=(Unexpected<Error>&& _err) noexcept {
    destroy();
    success_ = false;
    new (&get_err()) Error(_err.error());
    return *this;
  }

  /// @brief Copy assignment from Unexpected<Error>.
  ///
  /// Assigns an error to this Result<T> by copying from an Unexpected<Error>
  /// object. Destroys the current contents, sets the state to failure, and
  /// constructs the error in place.
  ///
  /// @param _err The Unexpected<Error> object to copy the error from.
  /// @return Reference to this Result<T>.
  Result<T>& operator=(const Unexpected<Error>& _err) noexcept {
    destroy();
    success_ = false;
    new (&get_err()) Error(_err.error());
    return *this;
  }

  /// @brief Assigns the underlying object from another Result<U>.
  /// @tparam U The type of the value in the other Result.
  /// @param _other The other Result<U> to assign from.
  /// @return Reference to this Result<T> after assignment.
  /// @details This assignment operator allows assigning from a Result<U> where
  /// U is convertible to T. It transforms the contained value (if any) from U
  /// to T, destroys the current value, and moves the result.
  template <class U>
    requires std::is_convertible_v<U, T>
  auto& operator=(const Result<U>& _other) {
    const auto to_t = [](const U& _u) -> T { return _u; };
    auto temp = _other.transform(to_t);
    destroy();
    success_ = temp.success_;
    move_from_other(temp);
    return *this;
  }

  /// @brief Applies a recovery function if the Result is an error (rvalue
  /// overload).
  /// @tparam F The type of the recovery function.
  /// @param _f A function that takes an Error and returns a Result<T>.
  /// @return Result<T> after applying the recovery function if this is an
  /// error, otherwise the contained value.
  /// @details This method is used for error recovery in a monadic style. If the
  /// Result is successful, it returns the value. If it contains an error, it
  /// calls the provided function with the error and returns its result.
  template <class F>
  Result<T> or_else(const F& _f) && {
    if (success_) {
      return std::move(*this).get_t();
    } else {
      return _f(std::move(*this).get_err());
    }
  }

  /// @brief Applies a recovery function if the Result is an error (const lvalue
  /// overload).
  /// @tparam F The type of the recovery function.
  /// @param _f A function that takes an Error and returns a Result<T>.
  /// @return Result<T> after applying the recovery function if this is an
  /// error, otherwise the contained value.
  /// @details This method is used for error recovery in a monadic style. If the
  /// Result is successful, it returns the value. If it contains an error, it
  /// calls the provided function with the error and returns its result.
  template <class F>
  Result<T> or_else(const F& _f) const& {
    if (success_) {
      return get_t();
    } else {
      return _f(get_err());
    }
  }

  /// @brief Applies a transformation function to the contained value if
  /// successful, moving the value.
  /// @tparam F The type of the transformation function, must be callable as U
  /// f(T).
  /// @param _f The transformation function to apply to the contained value.
  /// @return Result<U> containing the transformed value if successful,
  /// otherwise propagates the error.
  /// @note This overload is for rvalue references (&&), so the contained value
  /// is moved.
  template <class F>
  RFL_ALWAYS_INLINE auto transform(const F& _f) && {
    /// Result_U is expected to be of type Result<U>.
    using U = decltype(std::declval<F>()(std::declval<T>()));
    if (success_) {
      return Result<U>(_f(std::move(*this).get_t()));
    } else {
      return Result<U>(Unexpected(std::move(*this).get_err()));
    }
  }

  /// @brief Applies a transformation function to the contained value if
  /// successful, without moving the value.
  /// @tparam F The type of the transformation function, must be callable as U
  /// f(T).
  /// @param _f The transformation function to apply to the contained value.
  /// @return Result<U> containing the transformed value if successful,
  /// otherwise propagates the error.
  /// @note This overload is for const lvalue references (const&), so the
  /// contained value is not moved.
  template <class F>
  auto transform(const F& _f) const& {
    /// Result_U is expected to be of type Result<U>.
    using U = typename std::invoke_result<F, T>::type;
    if (success_) {
      return Result<U>(_f(get_t()));
    } else {
      return Result<U>(get_err());
    }
  }

  /// @brief Returns the value if the result does not contain an error.
  ///
  /// This method is an rvalue-qualified overload, meaning it can only be called
  /// on temporary objects. If the result is successful (i.e., contains a
  /// value), it returns the value by rvalue reference, transferring ownership
  /// using std::move. If the result contains an error, it throws a
  /// std::runtime_error with the error message. This is similar to the
  /// .unwrap() method in Rust, providing a convenient way to extract the value
  /// or fail fast if an error is present.
  ///
  /// @return T&& The contained value, moved from the result object.
  /// @throws std::runtime_error if the result contains an error.
  T&& value() && {
    if (success_) {
      return std::move(*this).get_t();
    } else {
      throw std::runtime_error(get_err().what());
    }
  }

  /// @brief Returns the value if the result does not contain an error.
  ///
  /// This method is an lvalue-qualified overload, meaning it can only be called
  /// on non-temporary objects. If the result is successful (i.e., contains a
  /// value), it returns the value by reference. If the result contains an
  /// error, it throws a std::runtime_error with the error message. This method
  /// is similar to .unwrap() in Rust, allowing safe extraction of the value or
  /// exception-based error handling.
  ///
  /// @return T& Reference to the contained value.
  /// @throws std::runtime_error if the result contains an error.
  T& value() & {
    if (success_) {
      return get_t();
    } else {
      throw std::runtime_error(get_err().what());
    }
  }

  /// @brief Returns the value if the result does not contain an error.
  ///
  /// This method is a const lvalue-qualified overload, meaning it can only be
  /// called on const, non-temporary objects. If the result is successful (i.e.,
  /// contains a value), it returns a const reference to the value. If the
  /// result contains an error, it throws a std::runtime_error with the error
  /// message. This method is similar to .unwrap() in Rust, providing
  /// exception-based error handling for const objects.
  ///
  /// @return const T& Const reference to the contained value.
  /// @throws std::runtime_error if the result contains an error.
  const T& value() const& {
    if (success_) {
      return get_t();
    } else {
      throw std::runtime_error(get_err().what());
    }
  }

  /// @brief Returns the contained value if present, otherwise returns the
  /// provided default value.
  /// @param _default The value to return if the Result does not contain a
  /// value.
  /// @return The contained value if present, otherwise the provided default
  /// value.
  /// @note This overload is for rvalue-qualified objects and moves the
  /// contained value.
  T value_or(T&& _default) && noexcept {
    if (success_) {
      return std::move(*this).get_t();
    } else {
      return std::forward<T>(_default);
    }
  }

  /// @brief Returns the contained value if present, otherwise returns the
  /// provided default value.
  /// @param _default The value to return if the Expected does not contain a
  /// value.
  /// @return The contained value if present, otherwise the provided default
  /// value.
  /// @note This overload is for const lvalue-qualified objects and copies the
  /// contained value.
  T value_or(const T& _default) const& noexcept {
    if (success_) {
      return get_t();
    } else {
      return _default;
    }
  }

  /// @brief Returns the contained error if present, otherwise returns the
  /// provided default error.
  /// @tparam G The type of the default error to return if no error is present.
  /// @param _default The error to return if the Expected contains a value.
  /// @return The contained error if present, otherwise the provided default
  /// error.
  /// @note This overload is for rvalue-qualified objects and moves the
  /// contained error.
  template <class G = Error>
  Error error_or(G&& _default) && {
    if (success_) {
      return std::forward<G>(_default);
    } else {
      return std::move(*this).get_err();
    }
  }

  //  As specified by the standard :
  //  https://en.cppreference.com/w/cpp/utility/expected
  //  Observers

  /// @brief Returns the contained error if present, otherwise returns the
  /// provided default error.
  /// @tparam G The type of the default error to return if no error is present.
  /// @param _default The error to return if the Expected contains a value.
  /// @return The contained error if present, otherwise the provided default
  /// error.
  /// @note This overload is for const lvalue-qualified objects and copies the
  /// contained error.
  template <class G = Error>
  Error error_or(G&& _default) const& {
    if (success_) {
      return std::forward<G>(_default);
    } else {
      return get_err();
    }
  }

  /// @brief Checks whether the Expected contains a value.
  /// @return true if the Expected contains a value, false if it contains an
  /// error.
  bool has_value() const noexcept { return success_; }

  /// @brief Returns the contained error, moving it if present.
  /// @return The contained error, moved from the Expected.
  /// @throws std::runtime_error if the Expected contains a value instead of an
  /// error.
  Error&& error() && {
    if (success_) throw std::runtime_error("Expected does not contain value");
    return std::move(*this).get_err();
  }

  /// @brief Returns the contained error by reference.
  /// @return Reference to the contained error.
  /// @throws std::runtime_error if the Expected contains a value instead of an
  /// error.
  Error& error() & {
    if (success_) throw std::runtime_error("Expected does not contain value");
    return get_err();
  }

  /// @brief Returns the contained error by const reference.
  /// @return Const reference to the contained error.
  /// @throws std::runtime_error if the Expected contains a value instead of an
  /// error.
  const Error& error() const& {
    if (success_) throw std::runtime_error("Expected does not contain value");
    return get_err();
  }

  /// @brief Provides pointer-like access to the contained value.
  /// @return Pointer to the contained value of type T.
  T* operator->() noexcept { return &get_t(); }

  /// @brief Provides pointer-like access to the contained value (const
  /// overload).
  /// @return Const pointer to the contained value of type T.
  const T* operator->() const noexcept { return &get_t(); }

  /**
   * @brief Transforms the error contained in this Result using the provided
   * function, if there is an error.
   *
   * This method is an rvalue overload and allows the caller to provide a
   * function that takes an Error and returns a new Error. If this
   * Result does not contain a value (i.e., it contains an error), the error is
   * transformed using the provided function and a new Result is returned
   * containing the transformed error. If this Result contains a value, the
   * value is moved into a new Result and returned as-is.
   *
   * @tparam F A callable type that takes an Error and returns an
   * Error.
   * @param f The function to apply to the error if present.
   * @return A new Result<T> containing either the transformed error or the
   * original value.
   */
  template <class F>
   Result<T> transform_error(F&& f) && {
     static_assert(
         std::is_same<decltype(std::declval<F>()(std::declval<Error>())), Error>(),
         "A function passed to transform_error must return an error.");
    if (!has_value()) {
      return Result<T>{std::invoke(f, std::move(*this).get_err())};
    } else {
      return Result<T>{std::move(*this).value()};
    }
  }

  /**
   * @brief Transforms the error contained in this Result using the provided
   * function, if there is an error (const lvalue overload).
   *
   * This method is a const lvalue overload and allows the caller to provide a
   * function that takes an Error and returns a new Error. If this
   * Result does not contain a value (i.e., it contains an error), the error is
   * transformed using the provided function and a new Result is returned
   * containing the transformed error. If this Result contains a value, the
   * value is copied into a new Result and returned as-is.
   *
   * @tparam F A callable type that takes an Error and returns an
   * Error.
   * @param f The function to apply to the error if present.
   * @return A new Result<T> containing either the transformed error or the
   * original value.
   */
  template <class F>
   Result<T> transform_error(F&& f) const& {
     static_assert(
         std::is_same<decltype(std::declval<F>()(std::declval<Error>())), Error>(),
         "A function passed to transform_error must return an error.");
    if (!has_value()) {
      return Result<T>{std::invoke(f, get_err())};
    } else {
      return Result<T>{value()};
    }
  }

 private:
  /// @brief Copies the value or error from another Result<T> instance.
  /// @param _other The other Result<T> to copy from.
  /// @details If this Result holds a value, constructs a new T from the other's
  /// value. Otherwise, constructs a new Error from the other's error.
  void copy_from_other(const Result<T>& _other) {
    if (success_) {
      new (&get_t()) T(_other.get_t());
    } else {
      new (&get_err()) Error(_other.get_err());
    }
  }

  /// @brief Destroys the contained value or error.
  /// @details If this Result holds a value and T is destructible, calls the
  /// destructor for T. Otherwise, calls the destructor for Error.
  void destroy() {
    if (success_) {
      if constexpr (std::is_destructible_v<std::remove_cv_t<T> >) {
        get_t().~T();
      }
    } else {
      get_err().~Error();
    }
  }

  /// @brief Retrieves the contained value as an rvalue reference.
  /// @return Rvalue reference to the contained T.
  /// @note Only call if this Result holds a value.
  T&& get_t() && noexcept {
    return std::move(*std::launder(reinterpret_cast<T*>(t_or_err_.data())));
  }

  /// @brief Retrieves the contained value as an lvalue reference.
  /// @return Lvalue reference to the contained T.
  /// @note Only call if this Result holds a value.
  T& get_t() & noexcept {
    return *std::launder(reinterpret_cast<T*>(t_or_err_.data()));
  }

  /// @brief Retrieves the contained value as a const lvalue reference.
  /// @return Const lvalue reference to the contained T.
  /// @note Only call if this Result holds a value.
  const T& get_t() const& noexcept {
    return *std::launder(reinterpret_cast<const T*>(t_or_err_.data()));
  }

  /// @brief Retrieves the contained error as an rvalue reference.
  /// @return Rvalue reference to the contained Error.
  /// @note Only call if this Result holds an error.
  Error&& get_err() && noexcept {
    return std::move(*std::launder(reinterpret_cast<Error*>(t_or_err_.data())));
  }

  /// @brief Retrieves the contained error as an lvalue reference.
  /// @return Lvalue reference to the contained Error.
  /// @note Only call if this Result holds an error.
  Error& get_err() & noexcept {
    return *std::launder(reinterpret_cast<Error*>(t_or_err_.data()));
  }

  /// @brief Retrieves the contained error as a const lvalue reference.
  /// @return Const lvalue reference to the contained Error.
  /// @note Only call if this Result holds an error.
  const Error& get_err() const& noexcept {
    return *std::launder(reinterpret_cast<const Error*>(t_or_err_.data()));
  }

  /// @brief Moves the contents from another Result<T> instance.
  ///
  /// This method transfers ownership of the value or error from the given
  /// Result<T> reference (_other) into this instance. If this Result holds a
  /// successful value (success_ is true), it move-constructs the value of type
  /// T from _other into this instance. Otherwise, it move-constructs the Error
  /// from _other. This operation does not throw exceptions.
  ///
  /// @param _other Reference to another Result<T> to move from.
  /// @note After this operation, the state of _other is valid but unspecified.
  /// @note This function is noexcept and should not throw exceptions.
  void move_from_other(Result<T>& _other) noexcept {
    if (success_) {
      new (&get_t()) T(std::move(_other.get_t()));
    } else {
      new (&get_err()) Error(std::move(_other.get_err()));
    }
  }

  /// Signifies whether this was a success.
  bool success_;

  /// The underlying data, can either be T or Error.
  alignas(std::max(alignof(T), alignof(Error))) TOrErr t_or_err_;
};

}  // namespace rfl::internal::cpp20

#endif
