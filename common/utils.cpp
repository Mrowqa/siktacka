#include "utils.hpp"


unimplemented_error::unimplemented_error(const std::string &message)
    : message("unimplemented_error: " + message) {
}


const char *unimplemented_error::what() const noexcept {
    return message.c_str();
}
