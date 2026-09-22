#pragma once
#include <Epub/Page.h>
#include <string>
namespace MaxPageText {
std::string extract(const Page& page, size_t maxBytes = 12288);
}
