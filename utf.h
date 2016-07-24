#ifndef UTF_H
#define UTF_H

#include "string_view.h"
#include <string>

std::string utf16_to_utf8(wstring_view s);

#endif // UTF_H
