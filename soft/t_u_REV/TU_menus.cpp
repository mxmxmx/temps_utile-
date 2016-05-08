#include <Arduino.h>
#include "TU_config.h"
#include "TU_core.h"
#include "TU_bitmaps.h"
#include "TU_menus.h"
#include "TU_outputs.h"

namespace TU {

static constexpr weegfx::coord_t note_circle_r = 28;

static struct coords {
  weegfx::coord_t x, y;
} circle_pos_lut[12];

static void init_circle_lut() {
  static const float pi = 3.14159265358979323846f;
  static const float semitone_radians = (2.f * pi / 12.f);

  for (int i = 0; i < 12; ++i) {
    float rads = ((i + 12 - 3) % 12) * semitone_radians;
    float x = note_circle_r * cos(rads);
    float y = note_circle_r * sin(rads);
    circle_pos_lut[i].x = x;
    circle_pos_lut[i].y = y;
  }
}

namespace menu {
void Init() {
  init_circle_lut();
};

}; // namespace menu


void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery) {
  graphics.drawCircle(centerx, centery, note_circle_r);

  coords last_pos = circle_pos_lut[normalized[0]];
  last_pos.x += centerx;
  last_pos.y += centery;
  for (size_t i = 1; i < 3; ++i) {
    graphics.drawBitmap8(last_pos.x - 3, last_pos.y - 3, 8, TU::circle_disk_bitmap_8x8);
    coords current_pos = circle_pos_lut[normalized[i]];
    current_pos.x += centerx;
    current_pos.y += centery;
    graphics.drawLine(last_pos.x, last_pos.y, current_pos.x, current_pos.y);
    last_pos = current_pos;
  }
  graphics.drawLine(last_pos.x, last_pos.y, circle_pos_lut[normalized[0]].x + centerx, circle_pos_lut[normalized[0]].y + centery);
  graphics.drawBitmap8(last_pos.x - 3, last_pos.y - 3, 8, TU::circle_disk_bitmap_8x8);
}

/* ----------- screensaver ----------------- */
void screensaver() {
  weegfx::coord_t x = 8;
  uint8_t y, width = 8;
  for(int i = 0; i < NUM_CHANNELS; i++, x += 21 ) { 
    y = OUTPUTS::value(i) >> 10; 
    y++; 
    graphics.drawRect(x, 64-y, width, width); // replace second 'width' with y for bars.
  }
}

static const size_t kScopeDepth = 64;

uint16_t scope_history[OUTPUTS::kHistoryDepth];
uint16_t averaged_scope_history[CLOCK_CHANNEL_LAST][kScopeDepth];
size_t averaged_scope_tail = 0;
CLOCK_CHANNEL scope_update_channel = CLOCK_CHANNEL_4;

template <size_t size>
inline uint16_t calc_average(const uint16_t *data) {
  uint32_t sum = 0;
  size_t n = size;
  while (n--)
    sum += *data++;
  return sum / size;
}

template <unsigned rshift, uint16_t bitmask>
void scope_averaging() {
    switch (scope_update_channel) {
    case CLOCK_CHANNEL_4:
      OUTPUTS::getHistory<CLOCK_CHANNEL_4>(scope_history);
      averaged_scope_history[CLOCK_CHANNEL_4][averaged_scope_tail] = ((65535U - calc_average<OUTPUTS::kHistoryDepth>(scope_history)) >> rshift) & bitmask;
      break;
    default: break;
  }
}

void scope_render() {
  scope_averaging<11, 0x1f>();

  for (weegfx::coord_t x = 0; x < (weegfx::coord_t)kScopeDepth - 1; ++x) {
    size_t index = (x + averaged_scope_tail + 1) % kScopeDepth;
    graphics.setPixel(x, 0 + averaged_scope_history[_DAC_CHANNEL][index]);
  }
}

void vectorscope_render() {
  scope_averaging<10, 0x3f>();

  for (weegfx::coord_t x = 0; x < (weegfx::coord_t)kScopeDepth - 1; ++x) {
    size_t index = (x + averaged_scope_tail + 1) % kScopeDepth;
    graphics.setPixel(averaged_scope_history[_DAC_CHANNEL][index], averaged_scope_history[_DAC_CHANNEL][index]);
  }
}

}; // namespace TU
