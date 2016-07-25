#ifndef UTILS_H
#define UTILS_H

#include "string_view.h"
#include <string>
#include <vector>
#include <utility>

std::pair<wstring_view, wstring_view> split_filename(wstring_view fname);

std::string to_base64(uint8_t const * p, size_t size);
std::vector<uint8_t> from_base64(string_view s);

#endif // UTILS_H
