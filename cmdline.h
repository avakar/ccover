#ifndef CMDLINE_H
#define CMDLINE_H

#include "string_view.h"
#include <string>

std::wstring win_split_cmdline_arg(wstring_view & cmdline);

#endif // CMDLINE_H
