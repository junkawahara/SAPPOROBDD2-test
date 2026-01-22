// SAPPOROBDD 2.0 - Exception classes
// MIT License

#ifndef SBDD2_EXCEPTION_HPP
#define SBDD2_EXCEPTION_HPP

#include <stdexcept>
#include <new>
#include <ios>
#include <string>

namespace sbdd2 {

// Base exception for DD operations
class DDException : public std::runtime_error {
public:
    explicit DDException(const std::string& msg)
        : std::runtime_error(msg) {}

    explicit DDException(const char* msg)
        : std::runtime_error(msg) {}
};

// Memory allocation failure
class DDMemoryException : public std::bad_alloc {
private:
    std::string msg_;

public:
    explicit DDMemoryException(const std::string& msg)
        : msg_(msg) {}

    explicit DDMemoryException(const char* msg)
        : msg_(msg) {}

    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

// Invalid argument
class DDArgumentException : public std::invalid_argument {
public:
    explicit DDArgumentException(const std::string& msg)
        : std::invalid_argument(msg) {}

    explicit DDArgumentException(const char* msg)
        : std::invalid_argument(msg) {}
};

// I/O error
class DDIOException : public std::ios_base::failure {
public:
    explicit DDIOException(const std::string& msg)
        : std::ios_base::failure(msg) {}

    explicit DDIOException(const char* msg)
        : std::ios_base::failure(msg) {}
};

// Operation on incompatible DD types or managers
class DDIncompatibleException : public DDException {
public:
    explicit DDIncompatibleException(const std::string& msg)
        : DDException(msg) {}

    explicit DDIncompatibleException(const char* msg)
        : DDException(msg) {}
};

} // namespace sbdd2

#endif // SBDD2_EXCEPTION_HPP
