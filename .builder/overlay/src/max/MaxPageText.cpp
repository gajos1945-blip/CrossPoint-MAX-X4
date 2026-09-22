#include "MaxPageText.h"
#include <cstring>

namespace MaxPageText {
std::string extract(const Page& page, const size_t maxBytes) {
  std::string out;
  out.reserve(2048);
  for (const auto& element : page.elements) {
    if (!element || element->getTag() != TAG_PageLine) continue;
    const auto& line = static_cast<const PageLine&>(*element);
    const auto& block = line.getBlock();
    if (!block || !block->valid()) continue;
    bool wroteWord = false;
    for (uint16_t i = 0; i < block->wordCount(); ++i) {
      const char* word = block->wordText(i);
      if (!word || *word == '\0') continue;
      const size_t len = std::strlen(word);
      const size_t extra = len + (wroteWord ? 1U : 0U) + 1U;
      if (out.size() + extra > maxBytes) return out;
      if (wroteWord) out.push_back(' ');
      out.append(word, len);
      wroteWord = true;
    }
    if (wroteWord && out.size() + 1 <= maxBytes) out.push_back('\n');
  }
  return out;
}
}
