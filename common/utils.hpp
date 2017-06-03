#pragma once

template<typename T, typename U>
T from_string(U &&string);

template<typename T>
void exit_with_error(T&& error_msg) noexcept;


#include "utils_impl.hpp"
