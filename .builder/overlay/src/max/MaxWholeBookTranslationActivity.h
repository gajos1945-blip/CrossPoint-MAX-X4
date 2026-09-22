#pragma once

#include <Epub.h>
#include <Epub/ReaderRenderSpec.h>
#include <Epub/Section.h>

#include <memory>
#include <string>

#include "activities/Activity.h"
#include "MaxBookState.h"
#include "MaxTranslationConfig.h"

class MaxWholeBookTranslationActivity final : public Activity {
  enum class State {
    Starting,
    PreparingSpine,
    BuildingSpine,
    TranslatingPage,
    Complete,
    Error
  };

  std::shared_ptr<Epub> epub;
  std::string bookPath;
  ReaderRenderSpec renderSpec;
  MaxTranslationConfig config;
  std::string layoutKey;
  MaxBookCheckpoint checkpoint;
  std::unique_ptr<Section> workingSection;

  State state = State::Starting;
  std::string errorText;
  bool initialized = false;
  int translatedThisRun = 0;
  int cachedThisRun = 0;
  int completedSpinesThisRun = 0;

  bool initializeJob();
  void prepareSpine();
  void buildSpineStep();
  void translatePageStep();
  void advanceSpine();
  void persistStatus(const char* status);
  void fail(std::string message);
  void cancelAndClose();
  void closeToReader();
  int overallPercent() const;

 public:
  MaxWholeBookTranslationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                  std::shared_ptr<Epub> epub, ReaderRenderSpec renderSpec);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override {
    return state != State::Complete && state != State::Error;
  }
};
