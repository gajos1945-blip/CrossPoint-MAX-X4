#include "MaxRenderFingerprint.h"

#include <cstdint>
#include <cstdio>

namespace {
uint64_t fnv1a64(const std::string& value) {
  uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char c : value) {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}
}

namespace MaxRenderFingerprint {
std::string fromSpec(const ReaderRenderSpec& spec) {
  const int lineCompressionQ =
      static_cast<int>(spec.lineCompression * 1000000.0f + 0.5f);
  const std::string canonical =
      std::to_string(spec.fontId) + "|" +
      std::to_string(lineCompressionQ) + "|" +
      std::to_string(spec.extraParagraphSpacing ? 1 : 0) + "|" +
      std::to_string(spec.paragraphAlignment) + "|" +
      std::to_string(spec.viewportWidth) + "|" +
      std::to_string(spec.viewportHeight) + "|" +
      std::to_string(spec.hyphenationEnabled ? 1 : 0) + "|" +
      std::to_string(spec.embeddedStyle ? 1 : 0) + "|" +
      std::to_string(spec.imageRendering) + "|" +
      std::to_string(spec.focusReadingEnabled ? 1 : 0);

  const uint64_t hash = fnv1a64(canonical);
  char out[17]{};
  std::snprintf(out, sizeof(out), "%016llx",
                static_cast<unsigned long long>(hash));
  return std::string(out);
}
}
