#include "MaxWholeBookTranslationActivity.h"

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
constexpr int TOP = 48;
constexpr int BUILD_PAGES_PER_TICK = 3;
}

MaxWholeBookTranslationActivity::MaxWholeBookTranslationActivity(
    GfxRenderer& renderer, MappedInputManager& mappedInput,
    std::shared_ptr<Epub> epub, ReaderRenderSpec renderSpec)
    : Activity("MaxWholeBookTranslate", renderer, mappedInput),
      epub(std::move(epub)),
      bookPath(this->epub ? this->epub->getPath() : ""),
      renderSpec(renderSpec),
      layoutKey(MaxRenderFingerprint::fromSpec(renderSpec)) {}

void MaxWholeBookTranslationActivity::onEnter() {
  Activity::onEnter();
  config = MaxTranslationConfig::load();
  requestUpdateAndWait();
}

bool MaxWholeBookTranslationActivity::initializeJob() {
  if (initialized) return state != State::Error;
  initialized = true;

  std::string cfgError;
  if (!config.valid(cfgError)) {
    fail(std::move(cfgError));
    return false;
  }
  if (!config.cache) {
    fail("Tlumaczenie calej ksiazki wymaga cache=true");
    return false;
  }
  if (!epub || bookPath.empty()) {
    fail("Brak aktywnej ksiazki EPUB");
    return false;
  }
  if (renderSpec.viewportWidth == 0 || renderSpec.viewportHeight == 0) {
    fail("Nieznany rozmiar obszaru czytania");
    return false;
  }

  const int total = epub->getSpineItemsCount();
  if (total <= 0) {
    fail("Ksiazka nie ma elementow spine");
    return false;
  }

  MaxBookCheckpoint saved;
  const bool hasSaved = MaxBookState::load(bookPath, config.target, saved);
  if (hasSaved && saved.totalSpines == total && saved.layoutKey == layoutKey &&
      saved.nextSpine >= 0 && saved.nextSpine <= total &&
      saved.nextPage >= 0 && saved.status != "complete") {
    checkpoint = std::move(saved);
    checkpoint.source = config.source;
    checkpoint.target = config.target;
    checkpoint.layoutKey = layoutKey;
    checkpoint.status = "running";
  } else if (hasSaved && saved.totalSpines == total && saved.layoutKey == layoutKey &&
             saved.nextSpine >= total && saved.status == "complete") {
    checkpoint = std::move(saved);
    state = State::Complete;
    return true;
  } else {
    checkpoint.nextSpine = 0;
    checkpoint.nextPage = 0;
    checkpoint.totalSpines = total;
    checkpoint.currentSpinePages = 0;
    checkpoint.source = config.source;
    checkpoint.target = config.target;
    checkpoint.layoutKey = layoutKey;
    checkpoint.status = "running";
  }

  if (!MaxBookState::save(bookPath, checkpoint)) {
    fail("Nie mozna zapisac checkpointu ksiazki");
    return false;
  }

  state = State::PreparingSpine;
  return true;
}

void MaxWholeBookTranslationActivity::persistStatus(const char* status) {
  checkpoint.status = status;
  if (!MaxBookState::save(bookPath, checkpoint)) {
    state = State::Error;
    errorText = "Nie mozna zapisac checkpointu ksiazki";
  }
}

void MaxWholeBookTranslationActivity::fail(std::string message) {
  errorText = std::move(message);
  if (!bookPath.empty() && !checkpoint.target.empty()) {
    checkpoint.status = "error";
    MaxBookState::save(bookPath, checkpoint);
  }
  state = State::Error;
}

void MaxWholeBookTranslationActivity::prepareSpine() {
  if (!epub) {
    fail("Utracono kontekst EPUB");
    return;
  }

  if (checkpoint.nextSpine >= checkpoint.totalSpines) {
    checkpoint.nextPage = 0;
    checkpoint.currentSpinePages = 0;
    persistStatus("complete");
    if (state != State::Error) state = State::Complete;
    return;
  }

  workingSection.reset(new Section(epub, checkpoint.nextSpine, renderer));
  const bool loaded = workingSection->loadSectionFile(renderSpec);
  const bool completeCache =
      loaded && !workingSection->isPartial() && workingSection->isBuildComplete();

  if (completeCache) {
    checkpoint.currentSpinePages = static_cast<int>(workingSection->pageCount);
    if (checkpoint.nextPage > checkpoint.currentSpinePages) checkpoint.nextPage = 0;
    persistStatus("running");
    if (state != State::Error) state = State::TranslatingPage;
    return;
  }

  bool started = false;
  {
    GfxRenderer::FrameBufferLoan loan(renderer);
    started = workingSection->startBuild(renderSpec);
  }
  if (!started) {
    fail("Nie mozna zbudowac rozdzialu " + std::to_string(checkpoint.nextSpine + 1));
    return;
  }
  state = State::BuildingSpine;
}

void MaxWholeBookTranslationActivity::buildSpineStep() {
  if (!workingSection) {
    fail("Brak sekcji podczas indeksowania");
    return;
  }

  if (!workingSection->isBuildComplete()) {
    if (!workingSection->buildSomeMore(BUILD_PAGES_PER_TICK)) {
      fail("Blad indeksowania rozdzialu " + std::to_string(checkpoint.nextSpine + 1));
      return;
    }
    yield();
  }

  if (workingSection->isBuildComplete()) {
    checkpoint.currentSpinePages = static_cast<int>(workingSection->pageCount);
    if (checkpoint.nextPage > checkpoint.currentSpinePages) checkpoint.nextPage = 0;
    persistStatus("running");
    if (state != State::Error) state = State::TranslatingPage;
  }
}

void MaxWholeBookTranslationActivity::advanceSpine() {
  checkpoint.nextSpine++;
  checkpoint.nextPage = 0;
  checkpoint.currentSpinePages = 0;
  workingSection.reset();
  completedSpinesThisRun++;

  if (checkpoint.nextSpine >= checkpoint.totalSpines) {
    persistStatus("complete");
    if (state != State::Error) state = State::Complete;
  } else {
    persistStatus("running");
    if (state != State::Error) state = State::PreparingSpine;
  }
}

void MaxWholeBookTranslationActivity::translatePageStep() {
  if (!workingSection) {
    fail("Brak sekcji podczas tlumaczenia");
    return;
  }

  const int totalPages = static_cast<int>(workingSection->pageCount);
  checkpoint.currentSpinePages = totalPages;

  if (checkpoint.nextPage >= totalPages) {
    advanceSpine();
    return;
  }

  const int pageIndex = checkpoint.nextPage;
  auto page = workingSection->loadPage(pageIndex);
  if (!page) {
    fail("Nie mozna odczytac strony " + std::to_string(pageIndex + 1) +
         " w rozdziale " + std::to_string(checkpoint.nextSpine + 1));
    return;
  }

  const std::string sourceText = MaxPageText::extract(*page);
  if (!sourceText.empty()) {
    std::string cached;
    if (MaxTranslationCache::load(bookPath, checkpoint.nextSpine, pageIndex,
                                  config.target, sourceText, cached)) {
      cachedThisRun++;
    } else {
      const MaxTranslationResponse response =
          MaxTranslationClient().translate(config.gateway, config.source,
                                           config.target, sourceText);
      if (!response.ok) {
        fail("Rozdzial " + std::to_string(checkpoint.nextSpine + 1) +
             ", strona " + std::to_string(pageIndex + 1) + ": " + response.error);
        return;
      }
      if (!MaxTranslationCache::store(bookPath, checkpoint.nextSpine, pageIndex,
                                      config.target, sourceText, response.translation)) {
        fail("Nie mozna zapisac cache rozdzialu " +
             std::to_string(checkpoint.nextSpine + 1) +
             ", strony " + std::to_string(pageIndex + 1));
        return;
      }
      translatedThisRun++;
    }
  }

  checkpoint.nextPage++;
  persistStatus("running");
  yield();

  if (state != State::Error && checkpoint.nextPage >= totalPages) {
    advanceSpine();
  }
}

int MaxWholeBookTranslationActivity::overallPercent() const {
  if (!epub || checkpoint.totalSpines <= 0) return 0;
  if (checkpoint.nextSpine >= checkpoint.totalSpines) return 100;

  float spineProgress = 0.0f;
  if (checkpoint.currentSpinePages > 0) {
    spineProgress =
        static_cast<float>(std::min(checkpoint.nextPage, checkpoint.currentSpinePages)) /
        static_cast<float>(checkpoint.currentSpinePages);
  }
  const float progress = epub->calculateProgress(checkpoint.nextSpine, spineProgress);
  return std::clamp(static_cast<int>(progress * 100.0f + 0.5f), 0, 100);
}

void MaxWholeBookTranslationActivity::cancelAndClose() {
  if (state != State::Complete && state != State::Error) {
    persistStatus("cancelled");
  }
  workingSection.reset();
  ActivityResult r;
  r.isCancelled = true;
  setResult(std::move(r));
  finish();
}

void MaxWholeBookTranslationActivity::closeToReader() {
  workingSection.reset();
  ActivityResult r;
  r.isCancelled = false;
  setResult(std::move(r));
  finish();
}

void MaxWholeBookTranslationActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (state == State::Complete || state == State::Error) {
      closeToReader();
    } else {
      cancelAndClose();
    }
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

  switch (state) {
    case State::PreparingSpine:
      prepareSpine();
      break;
    case State::BuildingSpine:
      buildSpineStep();
      break;
    case State::TranslatingPage:
      translatePageStep();
      break;
    case State::Starting:
    case State::Complete:
    case State::Error:
      break;
  }
  requestUpdateAndWait();
}

void MaxWholeBookTranslationActivity::render(RenderLock&&) {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 10, "CrossPoint MAX - Cala ksiazka");

  if (state == State::Starting) {
    renderer.drawCenteredText(UI_12_FONT_ID, TOP, "Przygotowanie...");
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                              "BACK: Anuluj");
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
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                              "BACK/OK: Wroc - checkpoint zachowany");
    renderer.displayBuffer();
    return;
  }

  if (state == State::Complete) {
    renderer.drawCenteredText(UI_12_FONT_ID, TOP, "Tlumaczenie ksiazki zakonczone");
    const std::string stats =
        "Nowe: " + std::to_string(translatedThisRun) +
        "  Cache: " + std::to_string(cachedThisRun);
    renderer.drawCenteredText(UI_10_FONT_ID, TOP + 34, stats.c_str());
    renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                              "BACK/OK: Wroc");
    renderer.displayBuffer();
    return;
  }

  const int displaySpine =
      std::min(checkpoint.nextSpine + 1, std::max(1, checkpoint.totalSpines));
  const std::string spineLine =
      "Rozdzial/spine " + std::to_string(displaySpine) + " / " +
      std::to_string(checkpoint.totalSpines);

  renderer.drawCenteredText(UI_12_FONT_ID, TOP, spineLine.c_str());

  if (state == State::BuildingSpine) {
    const int built = workingSection ? static_cast<int>(workingSection->pageCount) : 0;
    const std::string line = "Indeksowanie: " + std::to_string(built) + " stron";
    renderer.drawCenteredText(UI_10_FONT_ID, TOP + 32, line.c_str());
  } else {
    const std::string pageLine =
        "Strona " + std::to_string(checkpoint.nextPage) + " / " +
        std::to_string(checkpoint.currentSpinePages);
    renderer.drawCenteredText(UI_10_FONT_ID, TOP + 32, pageLine.c_str());
  }

  const std::string percentLine = std::to_string(overallPercent()) + "% ksiazki";
  renderer.drawCenteredText(UI_12_FONT_ID, TOP + 64, percentLine.c_str());

  const std::string stats =
      "Nowe: " + std::to_string(translatedThisRun) +
      " Cache: " + std::to_string(cachedThisRun) +
      " Spine: " + std::to_string(completedSpinesThisRun);
  renderer.drawCenteredText(UI_10_FONT_ID, TOP + 94, stats.c_str());

  renderer.drawCenteredText(UI_10_FONT_ID, renderer.getScreenHeight() - 22,
                            "BACK: Anuluj - checkpoint zostaje");
  renderer.displayBuffer();
}
