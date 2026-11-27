#ifndef _HPP_MENU_MATCHOPTIONS
#define _HPP_MENU_MATCHOPTIONS

#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/iconselector.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/widgets/menu.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class MatchOptionsPage : public Gui2Page {
public:
  MatchOptionsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~MatchOptionsPage();

  virtual void Process();
  void GoLoadingMatchPage();

  Gui2Button* buttonStart;

protected:
  void UpdateMatchDurationCaption();
  void UpdateDifficultyCaption();
  void UpdateKnockoutCaption();

  Gui2Slider* difficultySlider;
  Gui2Slider* matchDurationSlider;
  Gui2Button* knockoutButton = nullptr;
  int knockoutMode = 0;  // 0 off, 1 penalties, 2 extra time + penalties
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
