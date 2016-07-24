#ifndef UTILS_H
#define UTILS_H

#include "string_view.h"
#include <utility>

std::pair<wstring_view, wstring_view> split_filename(wstring_view fname);

#endif // UTILS_H
