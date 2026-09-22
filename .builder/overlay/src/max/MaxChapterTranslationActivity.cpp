#include "MaxChapterTranslationActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>

#include <algorithm>
#include <utility>

#include "MaxPageText.h"
#include "MaxRenderFingerprint.h"
#include "MaxTranslationCache.h"
#include "MaxTranslationClient.h"
#include "fontIds.h"

namespace {
constexpr int TOP = 54;
}

MaxChapterTranslationActivity::MaxChapterTranslationActivity(
    GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
    const int spine, Section* section, ReaderRenderSpec renderSpec)
    : Activity("MaxChapterTranslate", renderer, mappedInput),
      bookPath(std::move(bookPath)),
      spine(spine),
      chapterSection(section),
      renderSpec(renderSpec),
      layoutKey(MaxRenderFingerprint::fromSpec(renderSpec)) {}

void MaxChapterTranslationActivity::onEnter() {
  Activity::onEnter();
  config = MaxTranslationConfig::load();
  requestUpdateAndWait();
}

bool MaxChapterTranslationActivity::initializeJob() {
  if (initialized) return state == State::Running;
  initialized = true;

  std::string cfgError;
  if (!config.valid(cfgError)) {
    state = State::Error;
    errorText = std::move(cfgError);
    return false;
  }

  if (!chapterSection) {
    state = State::Error;
    errorText = "Brak aktywnego rozdzialu";
    return false;
  }

  if (chapterSection->isBuilding() || chapterSection->isPartial() ||
      !chapterSection->isBuildComplete()) {
    state = State::Error;
    errorText = "Rozdzial jest jeszcze indeksowany. Wroc do czytania i sprobuj ponownie.";
    return false;
  }

  const int total = static_cast<int>(chapterSection->pageCount);
  if (total <= 0) {
    state = State::Error;
    errorText = "Rozdzial nie ma stron do tlumaczenia";
    return false;
  }

  MaxChapterCheckpoint saved;
  const bool hasSaved = MaxChapterState::load(bookPath, spine, config.target, saved);
  if (hasSaved && saved.spine == spine && saved.target == config.target &&
      saved.layoutKey == layoutKey && saved.totalPages == total &&
      saved.nextPage >= 0 && saved.nextPage <= total && saved.status != "complete") {
    checkpoint = std::move(saved);
    checkpoint.source = config.source;
    checkpoint.layoutKey = layoutKey;
    checkpoint.status = "running";
  } else {
    checkpoint.spine = spine;
    checkpoint.nextPage = 0;
    checkpoint.totalPages = total;
    checkpoint.source = config.source;
    checkpoint.target = config.target;
    checkpoint.layoutKey = layoutKey;
    checkpoint.status = "running";
  }

  if (checkpoint.nextPage >= checkpoint.totalPages) {
    checkpoint.status = "complete";
    MaxChapterState::save(bookPath, checkpoint);
    state = State::Complete;
    return false;
  }

  MaxChapterState::save(bookPath, checkpoint);
  state = State::Running;
  return true;
}

void MaxChapterTranslationActivity::persistStatus(const char* status) {
  checkpoint.status = status;
  MaxChapterState::save(bookPath, checkpoint);
}

void MaxChapterTranslationActivity::processOnePage() {
  if (state != State::Running || !chapterSection) return;

  if (checkpoint.nextPage >= checkpoint.totalPages) {
    persistStatus("complete");
    state = State::Complete;
    return;
  }

  const int pageIndex = checkpoint.nextPage;
  auto page = chapterSection->loadPage(pageIndex);
  if (!page) {
    state = State::Error;
    errorText = "Nie mozna odczytac strony " + std::to_string(pageIndex + 1);
    persistStatus("error");
    return;
  }

  const std::string sourceText = MaxPageText::extract(*page);
  if (!sourceText.empty()) {
    std::string cached;
    if (config.cache &&
        MaxTranslationCache::load(bookPath, spine, pageIndex, config.target, sourceText, cached)) {
      cachedThisRun++;
    } else {
      const MaxTranslationResponse response =
          MaxTranslationClient().translate(config.gateway, config.source, config.target, sourceText);
      if (!response.ok) {
        state = State::Error;
        errorText = "Strona " + std::to_string(pageIndex + 1) + ": " + response.error;
        persistStatus("error");
        return;
      }

      if (config.cache &&
          !MaxTranslationCache::store(bookPath, spine, pageIndex, config.target,
                                      sourceText, response.translation)) {
        state = State::Error;
        errorText = "Nie mozna zapisac cache strony " + std::to_string(pageIndex + 1);
        persistStatus("error");
        return;
      }
      translatedThisRun++;
    }
  }

  checkpoint.nextPage++;
  persistStatus(checkpoint.nextPage >= checkpoint.totalPages ? "complete" : "running");
  if (checkpoint.nextPage >= checkpoint.totalPages) state = State::Complete;
  yield();
}

void MaxChapterTranslationActivity::cancelAndClose() {
  if (state == State::Running) persistStatus("cancelled");
  ActivityResult r;
  r.isCancelled = true;
  setResult(std::move(r));
  finish();
}

void MaxChapterTranslationActivity::closeToReader() {
  ActivityResult r;
  r.isCancelled = false;
  setResult(std::move(r));
  finish();
}

void MaxChapterTranslationActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    cancelAndClose();
    return;
  }

  if (state == State::Complete || state == State::Error) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      closeToReader();
    }
    return;
  }

  if (!initialized) {
    initializeJob();
    requestUpdateAndWait();
    return;
  }

  if (state == State::Running) {
    processOnePage();
    requestUpdateAndWait();
  }
}

void MaxChapterTranslationActivity::render(RenderLock&&) {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 10, "CrossPoint MAX - Rozdzial");

  if (state == State::Starting) {
    renderer.drawCenteredText(UI_12_FONT_ID, TOP, "Przygotowanie...");
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22, "BACK: Anuluj");
    renderer.displayBuffer();
    return;
  }

  if (state == State::Error) {
    const int width = std::max(80, renderer.getScreenWidth() - 24);
    const auto lines = renderer.wrappedText(UI_12_FONT_ID, errorText.c_str(), width, 8);
    int y = TOP;
    const int lh = renderer.getLineHeight(UI_12_FONT_ID) + 2;
    for (const auto& line : lines) {
      renderer.drawText(UI_12_FONT_ID, 12, y, line.c_str());
      y += lh;
    }
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22, "BACK/OK: Wroc");
    renderer.displayBuffer();
    return;
  }

  if (state == State::Complete) {
    renderer.drawCenteredText(UI_12_FONT_ID, TOP, "Rozdzial przetlumaczony");
    const std::string stats = "Nowe: " + std::to_string(translatedThisRun) +
                              "  Cache: " + std::to_string(cachedThisRun);
    renderer.drawCenteredText(UI_10_FONT_ID, TOP + 36, stats.c_str());
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22, "BACK/OK: Wroc");
    renderer.displayBuffer();
    return;
  }

  const int done = std::min(checkpoint.nextPage, checkpoint.totalPages);
  const int total = std::max(1, checkpoint.totalPages);
  const int percent = (done * 100) / total;

  const std::string pageLine =
      "Strona " + std::to_string(done) + " / " + std::to_string(checkpoint.totalPages);
  const std::string percentLine = std::to_string(percent) + "%";
  const std::string stats = "Nowe: " + std::to_string(translatedThisRun) +
                            "  Cache: " + std::to_string(cachedThisRun);

  renderer.drawCenteredText(UI_12_FONT_ID, TOP, pageLine.c_str());
  renderer.drawCenteredText(UI_12_FONT_ID, TOP + 34, percentLine.c_str());
  renderer.drawCenteredText(UI_10_FONT_ID, TOP + 68, stats.c_str());
  renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                            "BACK: Anuluj (checkpoint zostaje)");
  renderer.displayBuffer();
}
