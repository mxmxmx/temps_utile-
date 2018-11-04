#include <Arduino.h>
#include <algorithm>

#include "TU_apps.h"
#include "TU_bitmaps.h"
#include "TU_calibration.h"
#include "TU_config.h"
#include "TU_core.h"
#include "TU_gpio.h"
#include "TU_menus.h"
#include "TU_ui.h"
#include "TU_strings.h"
#include "TU_version.h"
#include "src/display.h"

extern uint_fast8_t MENU_REDRAW;

namespace TU {

Ui ui;

void Ui::Init() {
  ticks_ = 0;

  static const int button_pins[] = { but_top, but_bot, butL, butR };
  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    buttons_[i].Init(button_pins[i], TU_GPIO_BUTTON_PINMODE);
  }
  std::fill(button_press_time_, button_press_time_ + 4, 0);
  button_state_ = 0;
  button_ignore_mask_ = 0;
  screensaver_ = false;
  encoder_right_.Init(TU_GPIO_ENC_PINMODE);
  encoder_left_.Init(TU_GPIO_ENC_PINMODE);
  event_queue_.Init();
  global_config_menu_.Init();
  app_menu_.Init();
}

void Ui::configure_encoders(EncoderConfig encoder_config) {
  SERIAL_PRINTLN("Configuring encoders: %s (%x)", Strings::encoder_config_strings[encoder_config], encoder_config);
  encoder_right_.reverse(encoder_config & ENCODER_CONFIG_R_REVERSED);
  encoder_left_.reverse(encoder_config & ENCODER_CONFIG_L_REVERSED);
 }

void FASTRUN Ui::Poll() {

  uint32_t now = ++ticks_;
  uint16_t button_state = 0;

  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    if (buttons_[i].Poll())
      button_state |= control_mask(i);
  }

  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    auto &button = buttons_[i];
    if (button.just_pressed()) {
      button_press_time_[i] = now;
    } else if (button.released()) {
      if (now - button_press_time_[i] < kLongPressTicks)
        PushEvent(UI::EVENT_BUTTON_PRESS, control_mask(i), 0, button_state);
      else
        PushEvent(UI::EVENT_BUTTON_LONG_PRESS, control_mask(i), 0, button_state);
    }
  }

  encoder_right_.Poll();
  encoder_left_.Poll();

  int32_t increment;
  increment = encoder_right_.Read();
  if (increment)
    PushEvent(UI::EVENT_ENCODER, CONTROL_ENCODER_R, increment, button_state);

  increment = encoder_left_.Read();
  if (increment)
    PushEvent(UI::EVENT_ENCODER, CONTROL_ENCODER_L, increment, button_state);

  button_state_ = button_state;
}

UiMode Ui::DispatchEvents(const App *app) {

  while (event_queue_.available()) {
    const UI::Event event = event_queue_.PullEvent();
    if (IgnoreEvent(event))
      continue;

    switch (event.type) {
      case UI::EVENT_BUTTON_PRESS:
        app->HandleButtonEvent(event);
        break;
      case UI::EVENT_BUTTON_LONG_PRESS:
        if (TU::CONTROL_BUTTON_R != event.control)
          app->HandleButtonEvent(event);
        else
          return UI_MODE_APP_SETTINGS;
        break;
      case UI::EVENT_ENCODER:
        app->HandleEncoderEvent(event);
        break;
      default:
        break;
    }
    MENU_REDRAW = 1;
  }

  if (idle_time() > SCREENSAVER_TIMEOUT_MS) {
    if (!screensaver_)
      screensaver_ = true;
    return UI_MODE_SCREENSAVER;
  } else {
    return UI_MODE_MENU;
  }
}

UiMode Ui::Splashscreen(bool &reset_settings) {

  UiMode mode = UI_MODE_MENU;

  unsigned long start = millis();
  unsigned long now = start;
  do {

    mode = UI_MODE_MENU;
    if (read_immediate(CONTROL_BUTTON_L))
      mode = UI_MODE_CALIBRATE;
    if (read_immediate(CONTROL_BUTTON_R))
      mode = UI_MODE_APP_SETTINGS;

    reset_settings = 
      read_immediate(CONTROL_BUTTON_UP) && read_immediate(CONTROL_BUTTON_DOWN);

    now = millis();

    GRAPHICS_BEGIN_FRAME(true);

    menu::DefaultTitleBar::Draw();
#ifdef MODEL_2TT
  graphics.print("Time & Triggers");
#else
  graphics.print("temps_utile");
#endif
       weegfx::coord_t y = menu::CalcLineY(0);

    graphics.setPrintPos(menu::kIndentDx, y + menu::kTextDy);
    graphics.print("[L] => calibration");
    if (UI_MODE_CALIBRATE == mode)
      graphics.invertRect(menu::kIndentDx, y, 128, menu::kMenuLineH);

    y += menu::kMenuLineH;
    graphics.setPrintPos(menu::kIndentDx, y + menu::kTextDy);
    graphics.print("[R] =>  ... clocks");
    if (UI_MODE_APP_SETTINGS == mode)
      graphics.invertRect(menu::kIndentDx, y, 128, menu::kMenuLineH);

    y += menu::kMenuLineH;
    graphics.setPrintPos(menu::kIndentDx, y + menu::kTextDy);
    if (reset_settings)
      graphics.print("!! RESET EEPROM !!");

    y += menu::kMenuLineH;
    graphics.setPrintPos(menu::kIndentDx, y + menu::kTextDy);
    graphics.print(TU_VERSION);

    weegfx::coord_t w;
    if (now - start < SPLASHSCREEN_DELAY_MS)
      w = 128;
    else
      w = ((start + SPLASHSCREEN_DELAY_MS + SPLASHSCREEN_TIMEOUT_MS - now) << 7) / SPLASHSCREEN_TIMEOUT_MS;
    graphics.drawRect(0, 62, w, 2);

     /* fixes spurious button presses when booting ? */
     while (event_queue_.available())
        (void)event_queue_.PullEvent();

    GRAPHICS_END_FRAME();

  } while (now - start < SPLASHSCREEN_TIMEOUT_MS + SPLASHSCREEN_DELAY_MS);

  SetButtonIgnoreMask();
  return mode;
}

} // namespace TU
