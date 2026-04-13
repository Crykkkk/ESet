#ifndef SJTU_EXCEPTIONS_HPP
#define SJTU_EXCEPTIONS_HPP

#include <cstddef>
#include <string>
#include <exception>

namespace sjtu {

class exception : public std::exception {
protected:
    std::string detail;
    std::string variant;
    std::string full_msg; 

public:
    exception(const std::string &v = "Exception", const std::string &d = "") 
        : variant(v), detail(d) {
        full_msg = "[" + variant + "] " + detail;
    }

    virtual const char* what() const noexcept override {
        return full_msg.c_str();
    }
};

class index_out_of_bound : public exception {
public:
    index_out_of_bound(const std::string &d = "") : exception("Index Out Of Bound", d) {}
};

class runtime_error : public exception {
public:
    runtime_error(const std::string &d = "") : exception("Runtime Error", d) {}
};

class invalid_iterator : public exception {
public:
    invalid_iterator(const std::string &d = "") : exception("Invalid Iterator", d) {}
};

class container_is_empty : public exception {
public:
    container_is_empty(const std::string &d = "") : exception("Container Is Empty", d) {}
};

} // namespace sjtu

#endif