#pragma once


#include <stdexcept>


// TODO is that deadcode?
class unimplemented_error : public std::exception {
public:
    unimplemented_error(const std::string &message);
    virtual const char* what() const noexcept;

private:
    const std::string message;
};


template<typename T, typename U>
T from_string(U &&string);

template<typename T>
void exit_with_error(T&& error_msg) noexcept;


#include "utils_impl.hpp"
