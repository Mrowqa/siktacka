#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>


template<typename T, typename U>
T from_string(U &&string) {
    T result;
    std::istringstream str(std::forward<U&&>(string));
    str >> result;
    if (!str || str.peek() != EOF) {
        std::ostringstream msg;
        msg << "Cannot convert: " << str.str();
        throw std::runtime_error(msg.str());
    }

    return result;
}

template<typename T>
void exit_with_error(T &&error_msg) noexcept {
    std::cerr << std::forward<T>(error_msg) << std::endl;
    exit(1);
}
