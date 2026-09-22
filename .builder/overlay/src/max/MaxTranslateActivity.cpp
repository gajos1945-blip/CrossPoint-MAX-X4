#include "MaxTranslateActivity.h"
#include <GfxRenderer.h>
#include <algorithm>
#include <utility>
#include "MaxTranslationCache.h"
#include "MaxTranslationClient.h"
#include "fontIds.h"

namespace {
constexpr int MARGIN = 12;
constexpr int HEADER_Y = 8;
constexpr int BODY_TOP = 34;
constexpr int FOOTER_RESERVED = 34;

void pushWrappedLine(GfxRenderer& renderer, std::vector<std::string>& out,
                     const std::string& paragraph, const int width) {
  if (paragraph.empty()) {
    out.emplace_back();
    return;
  }
  std::string current;
  size_t pos = 0;
  while (pos < paragraph.size()) {
    while (pos < paragraph.size() && paragraph[pos] == ' ') ++pos;
    const size_t end = paragraph.find(' ', pos);
    const size_t stop = end == std::string::npos ? paragraph.size() : end;
    const std::string word = paragraph.substr(pos, stop - pos);
    pos = stop;
    if (word.empty()) continue;
    std::string candidate = current.empty() ? word : current + " " + word;
    if (renderer.getTextWidth(UI_12_FONT_ID, candidate.c_str()) <= width) {
      current = std::move(candidate);
    } else {
      if (!current.empty()) out.push_back(std::move(current));
      if (renderer.getTextWidth(UI_12_FONT_ID, word.c_str()) <= width) {
        current = word;
      } else {
        out.push_back(renderer.truncatedText(UI_12_FONT_ID, word.c_str(), width));
        current.clear();
      }
    }
  }
  if (!current.empty()) out.push_back(std::move(current));
}
}

MaxTranslateActivity::MaxTranslateActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                           std::string bookPath, const int spine, const int page,
                                           std::string sourceText)
    : Activity("MaxTranslate", renderer, mappedInput),
      bookPath(std::move(bookPath)), spine(spine), page(page),
      sourceText(std::move(sourceText)) {}

void MaxTranslateActivity::onEnter() {
  Activity::onEnter();
  config = MaxTranslationConfig::load();
  requestUpdateAndWait();
}

void MaxTranslateActivity::process() {
  if (processed) return;
  processed = true;
  std::string cfgError;
  if (!config.valid(cfgError)) {
    state = State::Error;
    errorText = std::move(cfgError);
    return;
  }
  if (config.cache &&
      MaxTranslationCache::load(bookPath, spine, page, config.target, translatedText)) {
    state = State::Ready;
    rebuildLines();
    return;
  }
  const MaxTranslationResponse response =
      MaxTranslationClient().translate(config.gateway, config.source, config.target, sourceText);
  if (!response.ok) {
    state = State::Error;
    errorText = response.error;
    return;
  }
  translatedText = response.translation;
  if (config.cache) {
    MaxTranslationCache::store(bookPath, spine, page, config.target, translatedText);
  }
  state = State::Ready;
  rebuildLines();
}

void MaxTranslateActivity::rebuildLines() {
  lines.clear();
  firstLine = 0;
  const int width = std::max(80, renderer.getScreenWidth() - 2 * MARGIN);
  size_t begin = 0;
  while (begin <= translatedText.size()) {
    const size_t end = translatedText.find('\n', begin);
    const size_t stop = end == std::string::npos ? translatedText.size() : end;
    pushWrappedLine(renderer, lines, translatedText.substr(begin, stop - begin), width);
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  if (lines.empty()) lines.emplace_back("(puste tlumaczenie)");
}

size_t MaxTranslateActivity::linesPerScreen() const {
  const int lineHeight = std::max(1, renderer.getLineHeight(UI_12_FONT_ID));
  const int bodyHeight = std::max(lineHeight, renderer.getScreenHeight() - BODY_TOP - FOOTER_RESERVED);
  return std::max<size_t>(1, static_cast<size_t>(bodyHeight / lineHeight));
}

void MaxTranslateActivity::closeOriginal() {
  ActivityResult r;
  r.isCancelled = true;
  setResult(std::move(r));
  finish();
}

void MaxTranslateActivity::closeNext() {
  setResult(PageResult{1});
  finish();
}

void MaxTranslateActivity::loop() {
  if (!processed) {
    process();
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    closeOriginal();
    return;
  }
  if (state != State::Ready) return;

  const size_t screenLines = linesPerScreen();
  if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    firstLine = firstLine > screenLines ? firstLine - screenLines : 0;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (firstLine + screenLines < lines.size()) {
      firstLine = std::min(firstLine + screenLines, lines.size() - 1);
      requestUpdate();
    }
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    closeNext();
    return;
  }
}

void MaxTranslateActivity::render(RenderLock&&) {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, HEADER_Y, "CrossPoint MAX - Translate");

  if (state == State::Pending) {
    renderer.drawCenteredText(UI_12_FONT_ID,
                              std::max(BODY_TOP, renderer.getScreenHeight() / 2),
                              "Translating...");
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22, "BACK: Original");
    renderer.displayBuffer();
    return;
  }

  if (state == State::Error) {
    const int width = std::max(80, renderer.getScreenWidth() - 2 * MARGIN);
    const auto wrapped = renderer.wrappedText(UI_12_FONT_ID, errorText.c_str(), width, 12);
    int y = BODY_TOP;
    const int lh = renderer.getLineHeight(UI_12_FONT_ID);
    for (const auto& line : wrapped) {
      renderer.drawText(UI_12_FONT_ID, MARGIN, y, line.c_str());
      y += lh;
    }
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22, "BACK: Original");
    renderer.displayBuffer();
    return;
  }

  const size_t count = linesPerScreen();
  const int lh = renderer.getLineHeight(UI_12_FONT_ID);
  int y = BODY_TOP;
  for (size_t i = firstLine; i < lines.size() && i < firstLine + count; ++i) {
    renderer.drawText(UI_12_FONT_ID, MARGIN, y, lines[i].c_str());
    y += lh;
  }
  const bool more = firstLine + count < lines.size();
  renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                            more ? "BACK Original | UP/DOWN text | OK Next"
                                 : "BACK Original | OK/RIGHT Next");
  renderer.displayBuffer();
}
