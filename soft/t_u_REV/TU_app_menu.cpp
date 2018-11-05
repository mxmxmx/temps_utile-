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
#include "TU_app_menu.h"
#include "TU_ui.h"
#include "TU_apps.h"

namespace TU {

void AppMenu::Page::Init(const char *n, int start, int end)
{
  name = n;
  cursor.Init(start, end);
}

void AppMenu::Init()
{
  current_page_ = APPS_PAGE;
  pages_[LOAD_PAGE].Init("Load", 0, app_storage.num_slots() - 1);
  pages_[SAVE_PAGE].Init("Save", 0, app_storage.num_slots() - 1);
  pages_[APPS_PAGE].Init("Init", 0, apps::num_available_apps() - 1);

  update_enabled_settings();
  pages_[CONF_PAGE].Init("Conf", 0, GLOBAL_CONFIG_SETTING_LAST - 1);
  pages_[CONF_PAGE].cursor.AdjustEnd(num_enabled_settings() - 1);
}

void AppMenu::Resume()
{
  pages_[CONF_PAGE].cursor.set_editing(false);
}

void AppMenu::Draw() const
{
  using TitleBar = menu::TitleBar<menu::kDefaultMenuStartX, 4, 2>;

  TitleBar::Draw();
  int column = 0;
  for (auto &page : pages_) {
    TitleBar::SetColumn(column++);
    graphics.print(page.name);
  }
  TitleBar::Selected(current_page_);

  if (APPS_PAGE == current_page_)
    DrawAppsPage();
  else if (CONF_PAGE == current_page_)
    DrawConfPage();
  else
    DrawSlotsPage(current_page_);
}

void AppMenu::DrawAppsPage() const
{
  menu::SettingsListItem item;
  item.x = 0;
  item.y = menu::CalcLineY(0);

  auto &cursor = pages_[APPS_PAGE].cursor;
  for (int current = cursor.first_visible();
       current <= cursor.last_visible();
       ++current, item.y += menu::kMenuLineH) {
    auto app_desc = apps::app_desc(current);
    item.selected = current == cursor.cursor_pos();
    item.SetPrintPos();
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);  
    graphics.print(app_desc->name);
    if (apps::current_app_id() == app_desc->id)
       graphics.drawBitmap8(item.x + 2, item.y + 1, 4, bitmap_indicator_4x8);
    item.DrawCustom();
  }
}

void AppMenu::DrawSlotsPage(PAGE page) const
{
  menu::SettingsListItem item;
  item.x = 0;
  item.y = menu::CalcLineY(0);

  auto &cursor = pages_[page].cursor;
  for (int current = cursor.first_visible();
       current <= cursor.last_visible();
       ++current, item.y += menu::kMenuLineH) {

    auto &slot = app_storage[current];
    auto app = apps::find(slot.id);

    item.selected = current == cursor.cursor_pos();
    item.SetPrintPos();
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
    switch(slot.state) {
      case SLOT_STATE::EMPTY: graphics.print("<empty>"); break;
      default:
        graphics.print(app ? app->name : "???");
    }

    // if (apps::current_app_id() == slot.id)
    //    graphics.drawBitmap8(item.x + 2, item.y + 1, 4, bitmap_indicator_4x8);
    item.DrawCustom();
  }
}

void AppMenu::DrawConfPage() const
{
  auto &cursor = pages_[CONF_PAGE].cursor;

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int setting = enabled_setting_at(settings_list.Next(list_item));
    list_item.DrawDefault(global_config.get_value(setting), global_config.value_attr(setting));
  } 
}

AppMenu::Action AppMenu::HandleEvent(const UI::Event &event)
{
  auto &current_page_cursor = pages_[current_page_].cursor;

  if (UI::EVENT_ENCODER == event.type) {
    if (CONTROL_ENCODER_L == event.control) {
      int page = current_page_ + event.value;
      CONSTRAIN(page, 0, PAGE_LAST - 1);
      current_page_ = static_cast<PAGE>(page);
    } else if (CONTROL_ENCODER_R == event.control) {
      if (CONF_PAGE == current_page_ && current_page_cursor.editing()) {
        GLOBAL_CONFIG_SETTING setting = enabled_setting_at(current_page_cursor.cursor_pos());
        if (global_config.change_value(setting, event.value))
          global_config.Apply();
      } else {
        current_page_cursor.Scroll(event.value);
      }
    }
  } else {
    if (CONTROL_BUTTON_DOWN == event.control)
      return { current_page_, ACTION_EXIT, 0 };
 
    if (CONTROL_BUTTON_R == event.control) {
      if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
        switch(current_page_) {
        case LOAD_PAGE: return { current_page_, ACTION_LOAD, current_page_cursor.cursor_pos() };
        case SAVE_PAGE: return { current_page_, ACTION_SAVE, current_page_cursor.cursor_pos() };
        case APPS_PAGE: return { current_page_, ACTION_INIT, current_page_cursor.cursor_pos() };
        default: break;
        }       
      } else {
        if (CONF_PAGE == current_page_)
          current_page_cursor.toggle_editing();
      }
    }
  }
  return { current_page_, ACTION_NONE, 0 };
}

void AppMenu::update_enabled_settings()
{
  GLOBAL_CONFIG_SETTING *settings = enabled_settings_;
  *settings++ = GLOBAL_CONFIG_SETTING_DIV1;
  *settings++ = GLOBAL_CONFIG_SETTING_SLAVE_TR1;
  num_enabled_settings_ = settings - enabled_settings_;
}

};
