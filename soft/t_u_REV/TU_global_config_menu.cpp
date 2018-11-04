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
#include <Arduino.h>
#include "TU_global_config_menu.h"
#include "TU_ui.h"

namespace TU {

void GlobalConfigMenu::Init()
{ 
  visible_ = false;
  update_enabled_settings();
  cursor_.Init(0, GLOBAL_CONFIG_SETTING_LAST - 1);
  cursor_.AdjustEnd(num_enabled_settings() - 1);
}

void GlobalConfigMenu::update_enabled_settings()
{
  GLOBAL_CONFIG_SETTING *settings = enabled_settings_;
  *settings++ = GLOBAL_CONFIG_SETTING_DIV1;
  *settings++ = GLOBAL_CONFIG_SETTING_SLAVE_TR1;
  num_enabled_settings_ = settings - enabled_settings_;
}

void GlobalConfigMenu::Draw() const
{
  menu::TitleBar<0, 4, 0>::Draw();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(cursor_);
  menu::SettingsListItem list_item;
  graphics.print("global input config.");

  while (settings_list.available()) {
    const int setting = enabled_setting_at(settings_list.Next(list_item));
    const int value = global_config.get_value(setting);
    const settings::value_attr &attr = global_config.value_attr(setting);

    switch (setting) {
    default:
      list_item.DrawDefault(value, attr);
    break;
    }
  } 
}

void GlobalConfigMenu::HandleButtonEvent(const UI::Event &event)
{
  if (TU::CONTROL_BUTTON_R == event.control)
    cursor_.toggle_editing();
}

void GlobalConfigMenu::HandleEncoderEvent(const UI::Event &event)
{
  if (TU::CONTROL_ENCODER_R == event.control) {   
    if (cursor_.editing()) {
      GLOBAL_CONFIG_SETTING setting = enabled_setting_at(cursor_.cursor_pos());
      if (global_config.change_value(setting, event.value))
        global_config.Apply();
    } else {
      cursor_.Scroll(event.value);
    }
  }
}

}; // namespace TU
