//
// Created by Mrowqa on 2017-06-02.
//

#ifndef SIKTACKA_UTILS_IMPL_HPP
#define SIKTACKA_UTILS_IMPL_HPP


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


#endif //SIKTACKA_UTILS_IMPL_HPP
