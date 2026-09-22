#pragma once

#include <Epub/Section.h>
#include <string>

#include "activities/Activity.h"
#include "MaxChapterState.h"
#include "MaxTranslationConfig.h"

class MaxChapterTranslationActivity final : public Activity {
  enum class State { Starting, Running, Complete, Error };

  std::string bookPath;
  int spine = 0;
  Section* chapterSection = nullptr;
  MaxTranslationConfig config;
  MaxChapterCheckpoint checkpoint;

  State state = State::Starting;
  std::string errorText;
  int translatedThisRun = 0;
  int cachedThisRun = 0;
  bool initialized = false;

  bool initializeJob();
  void processOnePage();
  void persistStatus(const char* status);
  void cancelAndClose();
  void closeToReader();

 public:
  MaxChapterTranslationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                std::string bookPath, int spine, Section* section);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return state == State::Running || state == State::Starting; }
};
