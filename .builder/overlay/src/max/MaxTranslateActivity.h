#pragma once
#include <string>
#include <vector>
#include "activities/Activity.h"
#include "MaxTranslationConfig.h"

class MaxTranslateActivity final : public Activity {
  enum class State { Pending, Ready, Error };
  std::string bookPath;
  int spine = 0;
  int page = 0;
  std::string sourceText;
  std::string translatedText;
  std::string errorText;
  MaxTranslationConfig config;
  State state = State::Pending;
  bool processed = false;
  std::vector<std::string> lines;
  size_t firstLine = 0;

  void process();
  void rebuildLines();
  size_t linesPerScreen() const;
  void closeOriginal();
  void closeNext();

 public:
  MaxTranslateActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                       std::string bookPath, int spine, int page, std::string sourceText);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return state == State::Pending; }
};
