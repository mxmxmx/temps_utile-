#ifndef TU_PATTERN_EDIT_H_
#define TU_PATTERN_EDIT_H_

#include "TU_bitmaps.h"
#include "TU_patterns.h"
#include "TU_patterns_presets.h"

namespace TU {

// Pattern editor
// written by Patrick Dowling, adapted for TU
//

template <typename Owner>
class PatternEditor {
public:

  void Init() {
    owner_ = nullptr;
    pattern_name_ = "?!";
    pattern_ = mutable_pattern_ = &dummy_pattern;
    mask_ = 0;
    cursor_pos_ = 0;
    num_slots_ = 0;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int pattern) {
    if (TU::Patterns::PATTERN_NONE == pattern)
      return;

    if (pattern < TU::Patterns::PATTERN_USER_LAST) {
      pattern_ = mutable_pattern_ = &TU::user_patterns[pattern];
      pattern_name_ = TU::pattern_names_short[pattern];
      Serial.print("Editing user pattern "); Serial.println(pattern_name_);
    } else {
      pattern_ = &TU::Patterns::GetPattern(pattern);
      mutable_pattern_ = nullptr;
      pattern_name_ = TU::pattern_names_short[pattern];
      Serial.print("Editing const pattern "); Serial.println(pattern_name_);
    }
    owner_ = owner;

    BeginEditing();
  }

  void Close();

  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);

  static uint16_t RotateMask(uint16_t mask, int num_slots, int amount);

private:

  Owner *owner_;
  const char * pattern_name_;
  const TU::Pattern *pattern_;
  Pattern *mutable_pattern_;

  uint16_t mask_;
  size_t cursor_pos_;
  size_t num_slots_;

  void BeginEditing();

  void move_cursor(int offset);

  void toggle_mask();
  void invert_mask(); 

  void apply_mask(uint16_t mask) {
    if (mask_ != mask) {
      mask_ = mask;
      owner_->update_pattern_mask(mask_);
    }
  }

  void reset_pattern();
  void change_slot(size_t pos, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void PatternEditor<Owner>::Draw() {
  size_t num_slots = num_slots_;

  static constexpr weegfx::coord_t kMinWidth = 8 * 7;

  weegfx::coord_t w =
    mutable_pattern_ ? (num_slots + 1) * 7 : num_slots * 7;

  if (w < kMinWidth) w = kMinWidth;

  weegfx::coord_t x = 64 - (w + 1)/ 2;
  weegfx::coord_t y = 16 - 3;
  weegfx::coord_t h = 36;

  graphics.clearRect(x, y, w + 4, h);
  graphics.drawFrame(x, y, w + 5, h);

  x += 2;
  y += 3;

  //graphics.setPrintPos(x, y);
  //graphics.print(pattern_name_);

  graphics.setPrintPos(x, y + 24);
  if (cursor_pos_ != num_slots) {
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
    if (mutable_pattern_ && TU::ui.read_immediate(TU::CONTROL_BUTTON_L))
      graphics.drawBitmap8(x + 1, y + 23, kBitmapEditIndicatorW, bitmap_edit_indicators_8);
    //graphics.print(pattern_->slots[cursor_pos_], 4); // disable until pw works
  } else {
    graphics.print((int)num_slots, 2);
  }

  x += 2; y += 10;
  uint16_t mask = mask_;
  for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {
    if (mask & 0x1)
      graphics.drawRect(x, y, 4, 8);
    else
      graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4x8);

    if (i == cursor_pos_)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
  }
  if (mutable_pattern_) {
    graphics.drawBitmap8(x, y, 4, bitmap_end_marker4x8);
    if (cursor_pos_ == num_slots)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
  }
}

template <typename Owner>
void PatternEditor<Owner>::HandleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case TU::CONTROL_BUTTON_UP:
        handleButtonUp(event);
        break;
      case TU::CONTROL_BUTTON_DOWN:
        handleButtonDown(event);
        break;
      case TU::CONTROL_BUTTON_L:
        handleButtonLeft(event);
        break;
      case TU::CONTROL_BUTTON_R:
        Close();
        break;
    }
  }
}

template <typename Owner>
void PatternEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
  bool pattern_changed = false;
  uint16_t mask = mask_;

  if (TU::CONTROL_ENCODER_L == event.control) {
    move_cursor(event.value);
  } else if (TU::CONTROL_ENCODER_R == event.control) {
    bool handled = false;
    if (mutable_pattern_) {
      if (cursor_pos_ < num_slots_) {
        if (event.mask & TU::CONTROL_BUTTON_L) {
          TU::ui.IgnoreButton(TU::CONTROL_BUTTON_L);
          change_slot(cursor_pos_, event.value, false);
          pattern_changed = true;
          handled = true;
        }
      } else {
        if (cursor_pos_ == num_slots_) {
          int num_slots = num_slots_;
          num_slots += event.value;
          CONSTRAIN(num_slots, kMinPatternLength, kMaxPatternLength);

          num_slots_ = num_slots;
          if (event.value > 0) {
            for (size_t pos = cursor_pos_; pos < num_slots_; ++pos)
              change_slot(pos, 0, false);

            // Enable new slots by default
            mask |= ~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_;
          } else {
            // pattern might be shortened to where no slots are active in mask
            if (0 == (mask & ~(0xffff < num_slots_)))
              mask |= 0x1;
          }

          mutable_pattern_->num_slots = num_slots_;
          cursor_pos_ = num_slots_;
          handled = pattern_changed = true;
        }
      }
    }

    if (!handled) {
      mask = RotateMask(mask_, num_slots_, event.value);
    }
  }

  // This isn't entirely atomic
  apply_mask(mask);
  if (pattern_changed)
    owner_->pattern_changed();
}

template <typename Owner>
void PatternEditor<Owner>::move_cursor(int offset) {

  int cursor_pos = cursor_pos_ + offset;
  const int max = mutable_pattern_ ? num_slots_ : num_slots_ - 1;
  CONSTRAIN(cursor_pos, 0, max);
  cursor_pos_ = cursor_pos;
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonUp(const UI::Event &event) {

  if (event.mask & TU::CONTROL_BUTTON_L) {
    TU::ui.IgnoreButton(TU::CONTROL_BUTTON_L);
    if (cursor_pos_ == num_slots_)
      reset_pattern();
    else
      change_slot(cursor_pos_, 128, true);
  } else {
    invert_mask();
  }
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonDown(const UI::Event &event) {
  if (event.mask & TU::CONTROL_BUTTON_L) {
    TU::ui.IgnoreButton(TU::CONTROL_BUTTON_L);
    change_slot(cursor_pos_, -128, true);
  } else {
    invert_mask();
  }
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonLeft(const UI::Event &) {
  uint16_t m = 0x1 << cursor_pos_;
  uint16_t mask = mask_;

  if (cursor_pos_ < num_slots_) {
    // toggle slot active state; avoid 0 mask
    if (mask & m) {
      if ((mask & ~(0xffff << num_slots_)) != m)
        mask &= ~m;
    } else {
      mask |= m;
    }
    apply_mask(mask);
  }
}

template <typename Owner>
void PatternEditor<Owner>::invert_mask() {
  uint16_t m = ~(0xffffU << num_slots_);
  uint16_t mask = mask_;
  // Don't invert to zero
  if ((mask & m) != m)
    mask ^= m;
  apply_mask(mask);
}

template <typename Owner>
/*static*/ uint16_t PatternEditor<Owner>::RotateMask(uint16_t mask, int num_slots, int amount) {
  uint16_t used_bits = ~(0xffffU << num_slots);
  mask &= used_bits;

  if (amount < 0) {
    amount = -amount % num_slots;
    mask = (mask >> amount) | (mask << (num_slots - amount));
  } else {
    amount = amount % num_slots;
    mask = (mask << amount) | (mask >> (num_slots - amount));
  }
  return mask | ~used_bits; // fill upper bits
}

template <typename Owner>
void PatternEditor<Owner>::reset_pattern() {
  Serial.println("Resetting pattern ... ");

  *mutable_pattern_ = TU::Patterns::GetPattern(TU::Patterns::PATTERN_ALL);
  num_slots_ = mutable_pattern_->num_slots;
  cursor_pos_ = num_slots_;
  mask_ = ~(0xfff << num_slots_);
  apply_mask(mask_);
}

template <typename Owner> // this should be for changing pulsewith; and seq length
void PatternEditor<Owner>::change_slot(size_t pos, int delta, bool notify) {
  if (mutable_pattern_ && pos < num_slots_) {
    int32_t slot = mutable_pattern_->slots[pos] + delta;

    const int32_t min = pos > 0 ? mutable_pattern_->slots[pos - 1] : 0;
    const int32_t max = pos < num_slots_ - 1 ? mutable_pattern_->slots[pos + 1] : mutable_pattern_->span + 1;

    // TODO It's probably possible to construct a pothological pattern,
    // maybe factor cursor_pos into it somehow?
    if (slot < min) slot = pos > 0 ? min + 1 : 0;
    if (slot > max) slot = max - 1;
    //mutable_pattern_->slots[pos] = slot;
    Serial.print(pos); Serial.print(" - > "); Serial.println(slot);

    if (notify)
      owner_->pattern_changed();
  }
}

template <typename Owner>
void PatternEditor<Owner>::BeginEditing() {

  cursor_pos_ = 0;
  num_slots_ = pattern_->num_slots;
  mask_ = owner_->get_mask();
}

template <typename Owner>
void PatternEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace TU

#endif // TU_PATTERN_EDIT_H_
