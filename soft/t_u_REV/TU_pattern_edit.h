#ifndef TU_PATTERN_EDIT_H_
#define TU_PATTERN_EDIT_H_

#include "TU_bitmaps.h"
#include "TU_patterns.h"
#include "TU_patterns_presets.h"
#include "TU_options.h"

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
    edit_this_sequence_ = 0;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int pattern, bool mode) {
    
    if (TU::Patterns::PATTERN_NONE == pattern)
      return;
    
    pattern_ = mutable_pattern_ = &TU::user_patterns[pattern];
    pattern_name_ = TU::pattern_names_short[pattern];
    owner_ = owner;

    BeginEditing(mode);
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
  int8_t edit_this_sequence_;
  size_t cursor_pos_;
  size_t num_slots_;
  bool edit_mode_;

  void BeginEditing(bool mode);

  void move_cursor(int offset);
  void toggle_mask();
  void invert_mask();
  void clear_mask(); 
  void copy_sequence(); 
  void paste_sequence(); 

  void apply_mask(uint16_t mask) {

    if (!edit_mode_) {
    // trigger edit:  
      if (mask_ != mask) {
        mask_ = mask;
        owner_->update_pattern_mask(mask_, edit_this_sequence_);
      }
      
      bool force_update = (owner_->get_current_sequence() == edit_this_sequence_);
      owner_->pattern_changed(mask, force_update);
    }
    else { 
    // cv edit: // to do, enable seq 2-4
       if (mask_ != mask) {
        mask_ = mask;
        owner_->update_cv_pattern_mask(mask_);
        owner_->cv_pattern_changed(mask, true);
      } 
    }
  }
  
  //void change_slot(size_t pos, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void PatternEditor<Owner>::Draw() {
  size_t num_slots = num_slots_;
  weegfx::coord_t w, x, y, h, kMinWidth;
  
  if (!edit_mode_) {
    
    kMinWidth = 8 * 7;
    w = mutable_pattern_ ? (num_slots + 1) * 7 : num_slots * 7;
  
    if (w < kMinWidth) w = kMinWidth;
    x = 64 - (w + 1)/ 2;
    y = 16 - 3;
    h = 36;
  
    graphics.clearRect(x, y, w + 4, h);
    graphics.drawFrame(x, y, w + 5, h);

  }
  else {
    w = 128;
    h = 64;
    x = 0;
    y = 0;
    graphics.clearRect(x, y, w, h);
    graphics.drawFrame(x, y, w, h); 
  }
  
  x += 2;
  y += 3;

  graphics.setPrintPos(x, y);
  
  uint8_t id = edit_this_sequence_;
  
  if (edit_this_sequence_ == owner_->get_sequence())
    id += 4;  
    
  if (!edit_mode_)
    graphics.print(TU::Strings::seq_id[id]);
  else
    graphics.print("CV:");
  
  if (cursor_pos_ != num_slots) {
    //graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);

    if (edit_mode_) {
      // print pitch value at current step  ...
      int32_t v_oct  = TU::OUTPUTS::get_v_oct();
      int32_t pitch  = owner_->dac_offset() * v_oct + (int)owner_->get_pitch_at_step(cursor_pos_) - TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero];
      CONSTRAIN(pitch, -TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero], TU::OUTPUTS::PITCH_LIMIT - TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero]);
      
      if (pitch >= 0) 
        graphics.printf(" %.2fv", (float)pitch / v_oct);
      else 
        graphics.printf("%.2fv", (float)pitch / v_oct);
      if (owner_-> dac_correction() != 0xFFF)
        graphics.print(" (+ -)");
    } // 
  }
  else {
    if (!edit_mode_)
      graphics.print(":");
    if (num_slots > 9)
      graphics.print((int)num_slots, 2);
    else
      graphics.print((int)num_slots);
  }

  if (!edit_mode_) {

    x += 2; y += 10;
    uint16_t mask = mask_;
    uint8_t clock_pos = owner_->get_clock_cnt();
    
    for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {
      if (mask & 0x1)
        graphics.drawRect(x, y, 4, 8);
      else
        graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4x8);
  
      if (i == cursor_pos_)
        graphics.drawFrame(x - 2, y - 2, 8, 12);
      // draw clock
      if (!edit_mode_) {
        if (i == clock_pos && (owner_->get_current_sequence() == edit_this_sequence_))  
          graphics.drawRect(x, y + 10, 4, 2);
      }
      else if (i == clock_pos)  
          graphics.drawRect(x, y + 10, 4, 2);
    }
    
    if (mutable_pattern_) {
      graphics.drawBitmap8(x, y, 4, bitmap_end_marker4x8);
      if (cursor_pos_ == num_slots)
        graphics.drawFrame(x - 2, y - 2, 8, 12);
    }
  }
  else {
    
    x += 3 + (w >> 0x1) - (num_slots << 0x2); 
    #ifdef MOD_OFFSET
      y += 40;
    #else
      #ifdef MODEL_2TT
      y += 45;
      #else
      y += 35;
      #endif
    #endif
    uint16_t mask = mask_;
    uint8_t clock_pos = owner_->get_clock_cnt();

    for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {
  
      int pitch = owner_->dac_offset() * TU::OUTPUTS::get_v_oct() + (int)owner_->get_pitch_at_step(i) - TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero];
      
      bool _clock = (i == clock_pos);
       
      if (mask & 0x1 & (pitch >= 0)) {
        pitch += 0x100;
        if (_clock)
          graphics.drawRect(x - 1, y - (pitch >> 7), 6, pitch >> 7);
        else
          graphics.drawRect(x, y - (pitch >> 7), 4, pitch >> 7);
      }
      else if (mask & 0x1) {
        pitch -= 0x100;
        if (_clock)
          graphics.drawRect(x - 1, y, 6, abs(pitch) >> 7);
        else 
          graphics.drawRect(x, y, 4, abs(pitch) >> 7);
      }
      else if (pitch > - 0x200 && pitch < 0x200) {
       // disabled steps not visible otherwise..
       graphics.drawRect(x + 1, y - 1, 2, 2);
      }
      else if (pitch >= 0) {
        pitch += 0x100;
        graphics.drawFrame(x, y - (pitch >> 7), 4, pitch >> 7);
      }
      else {
        pitch -= 0x100;
        graphics.drawFrame(x, y, 4, abs(pitch) >> 7);
      }
  
      if (i == cursor_pos_) {
        if (TU::ui.read_immediate(TU::CONTROL_BUTTON_L))
          graphics.drawFrame(x - 3, y - 5, 10, 10);
        else 
          graphics.drawFrame(x - 2, y - 4, 8, 8);
      }
      
      #ifdef MOD_OFFSET
        if (_clock)
          graphics.drawRect(x, y + 17, 4, 2);
      #else 
        if (_clock)
          #ifdef MODEL_2TT
          graphics.drawRect(x, y + 12, 4, 2);
          #else
          graphics.drawRect(x, y + 22, 4, 2);
          #endif
      #endif
         
    }
    if (mutable_pattern_) {
       graphics.drawFrame(x, y - 2, 4, 4);
      if (cursor_pos_ == num_slots)
        graphics.drawFrame(x - 2, y - 4, 8, 8);
    }
    
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
  else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case TU::CONTROL_BUTTON_UP:
        //invert_mask();
        copy_sequence();
        break;
      case TU::CONTROL_BUTTON_DOWN:
        //clear_mask();
        paste_sequence();
      break;
      default:
      break;
     }
  }
}

template <typename Owner>
void PatternEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
 
  uint16_t mask = mask_;

  if (TU::CONTROL_ENCODER_L == event.control) {
    move_cursor(event.value);
  } else if (TU::CONTROL_ENCODER_R == event.control) {
    bool handled = false;
    if (mutable_pattern_) {
      if (cursor_pos_ >= num_slots_) {
       
        if (cursor_pos_ == num_slots_) {
          int num_slots = num_slots_;
          num_slots += event.value;
          CONSTRAIN(num_slots, kMinPatternLength, kMaxPatternLength);

          num_slots_ = num_slots;
           if (event.value > 0) {
            // erase  slots when expanding?
            if (TU::ui.read_immediate(TU::CONTROL_BUTTON_L)) 
               mask &= ~(~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_);
             //mask |= ~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_; // alternative behaviour would be to fill them
          } 
          
          if (!edit_mode_)
            owner_->set_sequence_length(num_slots_, edit_this_sequence_);
          else 
            owner_->set_cv_sequence_length(num_slots_);  
          cursor_pos_ = num_slots_;
          handled = true;
        }
      }
    }

    if (!handled) {
   
      if (!edit_mode_) // we edit triggers only
        mask = RotateMask(mask_, num_slots_, event.value);
      else {
        // adjust pitch value at current step:
        int16_t pitch = owner_->get_pitch_at_step(cursor_pos_);  
        int16_t delta = event.value;

        if (TU::ui.read_immediate(TU::CONTROL_BUTTON_L)) 
            pitch += delta; // fine
        else 
            pitch += (delta << 4); // coarse
            
        CONSTRAIN(pitch, TU::calibration_data.dac.calibration_points[0x0][0x0], TU::OUTPUTS::PITCH_LIMIT);    
        // set:
        owner_->set_pitch_at_step(cursor_pos_, pitch);
      }
    }
  }
  // This isn't entirely atomic
  apply_mask(mask);
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

    if (!edit_mode_) {
      // next pattern / edit 'offline'//
      edit_this_sequence_++;
      if (edit_this_sequence_ > TU::Patterns::PATTERN_USER_LAST-1)
        edit_this_sequence_ = 0;
        
      cursor_pos_ = 0;
      num_slots_ = owner_->get_sequence_length(edit_this_sequence_);
      mask_ = owner_->get_mask(edit_this_sequence_);
    }
    // else  // todo
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonDown(const UI::Event &event) {

    if (!edit_mode_) {
      // next pattern / edit 'offline':
      edit_this_sequence_--;
      if (edit_this_sequence_ < 0)
        edit_this_sequence_ = TU::Patterns::PATTERN_USER_LAST-1;
        
      cursor_pos_ = 0;
      num_slots_ = owner_->get_sequence_length(edit_this_sequence_);
      mask_ = owner_->get_mask(edit_this_sequence_);
    }
    // else  // todo
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonLeft(const UI::Event &) {
  uint16_t m = 0x1 << cursor_pos_;
  uint16_t mask = mask_;

  if (cursor_pos_ < num_slots_) {
    // toggle slot active state; allow 0 mask
    // disable toggle for CV-SEQ, bc it doesn't make sense
    if (!edit_mode_ || owner_->get_cv_playmode() == 5) {
        if (mask & m)
          mask &= ~m;
        else 
          mask |= m;
        apply_mask(mask);
    }
  }
}

template <typename Owner>
void PatternEditor<Owner>::invert_mask() {
  uint16_t m = ~(0xffffU << num_slots_);
  uint16_t mask = mask_ ^ m;
  apply_mask(mask);
}

template <typename Owner>
void PatternEditor<Owner>::clear_mask() {
  apply_mask(0x00);
}

template <typename Owner>
void PatternEditor<Owner>::copy_sequence() {
  owner_->copy_seq(num_slots_, mask_);
}

template <typename Owner>
void PatternEditor<Owner>::paste_sequence() {
  
     uint8_t newslots = owner_->paste_seq(edit_this_sequence_);
     num_slots_ = newslots ? newslots : num_slots_;
     mask_ = owner_->get_mask(edit_this_sequence_);
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
void PatternEditor<Owner>::BeginEditing(bool mode) {

  cursor_pos_ = 0;
  edit_mode_ = mode;
  uint8_t seq = owner_->get_sequence();
  
  if (!edit_mode_) {
    // trigger edit:
    edit_this_sequence_ = seq;
    num_slots_ = owner_->get_sequence_length(seq);
    mask_ = owner_->get_mask(seq);
  }
  else { 
    // to do: enable seq 2-4
    edit_this_sequence_ = 0x0;
    num_slots_ = owner_->get_cv_sequence_length();
    mask_ = owner_->get_cv_mask();
  }
}

template <typename Owner>
void PatternEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace TU

#endif // TU_PATTERN_EDIT_H_
