#pragma once


#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>


// TODO is that deadcode?
class unimplemented_error : public std::exception {
public:
    unimplemented_error(const std::string &message);
    virtual const char* what() const noexcept;

private:
    const std::string message;
};


template<typename T, typename U, typename V>
T to_number(U &&name, V &&string,
            T min_val = std::numeric_limits<T>::min(),
            T max_val = std::numeric_limits<T>::max()) {
    T result;
    std::istringstream str(std::forward<V&&>(string));

    auto fail = [&str,&name]() {
        std::ostringstream msg;
        msg << "Option \"" << name << "\": cannot convert \""
            << str.str() << "\" to number or value not in proper range.";
        throw std::runtime_error(msg.str());
    };

    for (auto c : str.str()) {
        if (!isdigit(c)) {
            fail();
        }
    }

    str >> result;
    if (!str || str.peek() != EOF) {
        fail();
    }

    if (result < min_val || max_val < result) {
        fail();
    }

    return result;
}


template<typename T>
void exit_with_error(T &&error_msg) noexcept {
    std::cerr << std::forward<T>(error_msg) << std::endl;
    exit(1);
}
