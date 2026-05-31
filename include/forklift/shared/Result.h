// Result<T, E> — lightweight expected-style return type for fallible operations.
// Use it in application/infrastructure boundary calls instead of exceptions, which
// are reserved for unrecoverable programmer errors.

#ifndef FORKLIFT_SHARED_RESULT_H_
#define FORKLIFT_SHARED_RESULT_H_

#include <string>
#include <utility>
#include <variant>

namespace forklift::shared {

struct Error {
    int         code{0};
    std::string message;
};

template <typename T>
class Result {
public:
    Result(T value) : data_{std::move(value)} {}                  // NOLINT(google-explicit-constructor)
    Result(Error err) : data_{std::move(err)} {}                  // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(data_); }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }

    T&        value()       { return std::get<T>(data_); }
    const T&  value() const { return std::get<T>(data_); }
    const Error& error() const { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    Result() : err_{} {}
    Result(Error e) : err_{std::move(e)}, has_err_{true} {}        // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool ok() const noexcept { return !has_err_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
    const Error& error() const { return err_; }

private:
    Error err_{};
    bool  has_err_{false};
};

}  // namespace forklift::shared

#endif  // FORKLIFT_SHARED_RESULT_H_
