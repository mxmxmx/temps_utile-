// Copyright 2018 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
#ifndef TU_APP_MENU_H_
#define TU_APP_MENU_H_

#include <array>
#include "UI/ui_events.h"
#include "src/display.h"
#include "TU_menus.h"
#include "TU_global_config.h"

namespace TU {

class AppMenu {
public:

  void Init();

  enum PAGE {
    LOAD_PAGE,
    SAVE_PAGE,
    APPS_PAGE,
    CONF_PAGE,
    PAGE_LAST
  };

  enum ACTION_TYPE {
    ACTION_NONE,
    ACTION_EXIT,
    ACTION_LOAD,
    ACTION_SAVE,
    ACTION_INIT,
  };

  struct Action {
    PAGE page = PAGE_LAST;
    ACTION_TYPE type = ACTION_NONE;
    int index = 0;

    bool valid() const {
      return ACTION_NONE != type;
    }
  };

  void Resume();

  void Draw() const;
  Action HandleEvent(const UI::Event &event);

private:

  PAGE current_page_;
  struct Page {
    const char *name;
    menu::ScreenCursor<menu::kScreenLines> cursor;

    void Init(const char *n, int start, int end);
  };
  std::array<Page, PAGE_LAST> pages_;

  int num_enabled_settings_;
  GLOBAL_CONFIG_SETTING enabled_settings_[GLOBAL_CONFIG_SETTING_LAST];

  const char *message_;

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  GLOBAL_CONFIG_SETTING enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings();

  void DrawAppsPage() const;
  void DrawSlotsPage(PAGE page) const;
  void DrawConfPage() const;
};

}; // namespace TU

#endif // TU_APP_MENU_H_
