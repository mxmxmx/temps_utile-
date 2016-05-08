#include <Arduino.h>
#include "TU_ADC.h"
#include "TU_config.h"
#include "TU_core.h"
#include "TU_debug.h"
#include "TU_menus.h"
#include "TU_ui.h"
#include "util/util_misc.h"
#include "extern/dspinst.h"

// extern void CLOCKS_debug();


namespace TU {

namespace DEBUG {
  debug::AveragedCycles ISR_cycles;
  debug::AveragedCycles UI_cycles;
  debug::AveragedCycles MENU_draw_cycles;
  uint32_t UI_event_count;
  uint32_t UI_max_queue_depth;
  uint32_t UI_queue_overflow;

  void Init() {
    debug::CycleMeasurement::Init();
    DebugPins::Init();
  }
}; // namespace DEBUG

static void debug_menu_core() {

  graphics.setPrintPos(2, 12);
  graphics.printf("%uMHz %uus+%uus", F_CPU / 1000 / 1000, TU_CORE_TIMER_RATE, TU_UI_TIMER_RATE);
  
  graphics.setPrintPos(2, 22);
  uint32_t isr_us = debug::cycles_to_us(DEBUG::ISR_cycles.value());
  graphics.printf("CORE%3u/%3u/%3u %2u%%",
                  debug::cycles_to_us(DEBUG::ISR_cycles.min_value()),
                  isr_us,
                  debug::cycles_to_us(DEBUG::ISR_cycles.max_value()),
                  (isr_us * 100) /  TU_CORE_TIMER_RATE);

  graphics.setPrintPos(2, 32);
  graphics.printf("POLL%3u/%3u/%3u",
                  debug::cycles_to_us(DEBUG::UI_cycles.min_value()),
                  debug::cycles_to_us(DEBUG::UI_cycles.value()),
                  debug::cycles_to_us(DEBUG::UI_cycles.max_value()));

#ifdef TU_UI_DEBUG
  graphics.setPrintPos(2, 42);
  graphics.printf("UI   !%u #%u", DEBUG::UI_queue_overflow, DEBUG::UI_event_count);
  graphics.setPrintPos(2, 52);
#endif
}

static void debug_menu_gfx() {
  graphics.drawFrame(0, 0, 128, 64);

  graphics.setPrintPos(0, 12);
  graphics.print("W");

  graphics.setPrintPos(2, 22);
  graphics.printf("MENU %3u/%3u/%3u",
                  debug::cycles_to_us(DEBUG::MENU_draw_cycles.min_value()),
                  debug::cycles_to_us(DEBUG::MENU_draw_cycles.value()),
                  debug::cycles_to_us(DEBUG::MENU_draw_cycles.max_value()));
}

static void debug_menu_adc() {
  graphics.setPrintPos(2, 12);
  graphics.printf("CV1 %5d %5u", ADC::value<ADC_CHANNEL_1>(), ADC::raw_value(ADC_CHANNEL_1));

  graphics.setPrintPos(2, 22);
  graphics.printf("CV2 %5d %5u", ADC::value<ADC_CHANNEL_2>(), ADC::raw_value(ADC_CHANNEL_2));

  graphics.setPrintPos(2, 32);
  graphics.printf("CV3 %5d %5u", ADC::value<ADC_CHANNEL_3>(), ADC::raw_value(ADC_CHANNEL_3));

  graphics.setPrintPos(2, 42);
  graphics.printf("CV4 %5d %5u", ADC::value<ADC_CHANNEL_4>(), ADC::raw_value(ADC_CHANNEL_4));

//      graphics.setPrintPos(2, 42);
//      graphics.print((long)ADC::busy_waits());
//      graphics.setPrintPos(2, 42); graphics.print(ADC::fail_flag0());
//      graphics.setPrintPos(2, 52); graphics.print(ADC::fail_flag1());
}

struct DebugMenu {
  const char *title;
  void (*display_fn)();
};

static const DebugMenu debug_menus[] = {
  { " CORE", debug_menu_core },
  { " GFX", debug_menu_gfx },
  { " ADC", debug_menu_adc },
 // { " CLOC", POLYLFO_debug },
  { nullptr, nullptr }
};

void Ui::DebugStats() {
  SERIAL_PRINTLN("DEBUG/STATS MENU");

  const DebugMenu *current_menu = &debug_menus[0];
  bool exit_loop = false;
  while (!exit_loop) {

    GRAPHICS_BEGIN_FRAME(false);
      graphics.setPrintPos(2, 2);
      graphics.printf("%d/%u", (int)(current_menu - &debug_menus[0]) + 1, ARRAY_SIZE(debug_menus) - 1);
      graphics.print(current_menu->title);
      current_menu->display_fn();
    GRAPHICS_END_FRAME();

    while (event_queue_.available()) {
      UI::Event event = event_queue_.PullEvent();
      if (CONTROL_BUTTON_R == event.control) {
        exit_loop = true;
      } else if (CONTROL_BUTTON_L == event.control) {
        ++current_menu;
        if (!current_menu->title || !current_menu->display_fn)
          current_menu = &debug_menus[0];
      }
    }
  }

  event_queue_.Flush();
  event_queue_.Poke();
}

}; // namespace TU
