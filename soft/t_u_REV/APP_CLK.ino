#include "util/util_settings.h"
#include "util/util_turing.h"
#include "util/util_logistic_map.h"
#include "TU_apps.h"
#include "TU_outputs.h"
#include "TU_menus.h"
#include "TU_strings.h"
#include "TU_visualfx.h"
#include "extern/dspinst.h"

namespace menu = TU::menu; // Ugh. This works for all .ino files

const int8_t MODES = 7;
const int8_t CHANNELS = 6;
const int8_t _DAC_CHANNEL = 3; // ie, counting from zero
const float TICKS_TO_MS = 16.6667f; // 1 tick = 60 us;

/*
const char* const multipliers[] = {
  "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
};
*/

const float multipliers_[] = {
   8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.5f, 0.3333f, 0.25f, 0.2f, 0.16667f, 0.142857f, 0.125f
};


enum ChannelSetting {
  
  // shared
  CHANNEL_SETTING_MODE,
  CHANNEL_SETTING_MODE4,
  CHANNEL_SETTING_TRIGGER,
  CHANNEL_SETTING_RESET,
  CHANNEL_SETTING_MULT,
  CHANNEL_SETTING_PULSEWIDTH,
  CHANNEL_SETTING_INVERTED,
  CHANNEL_SETTING_DELAY,
  // mode specifics
  CHANNEL_SETTING_LFSR_LEN,
  CHANNEL_SETTING_LFSR_TAP1,
  CHANNEL_SETTING_LFSR_TAP2,
  CHANNEL_SETTING_RAND_N,
  CHANNEL_SETTING_EUCLID_N,
  CHANNEL_SETTING_EUCLID_K,
  CHANNEL_SETTING_EUCLID_OFFSET,
  CHANNEL_SETTING_LOGIC_TYPE,
  CHANNEL_SETTING_LOGIC_OP1,
  CHANNEL_SETTING_LOGIC_OP2,
  CHANNEL_SETTING_DAC_RANGE,
  CHANNEL_SETTING_DAC_MODE,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
  CHANNEL_SETTING_TURING_RANGE,
  CHANNEL_SETTING_LOGISTIC_MAP_R,
  CHANNEL_SETTING_LOGISTIC_MAP_RANGE,
  CHANNEL_SETTING_LOGISTIC_MAP_SEED,
  CHANNEL_SETTING_SEQ,
  CHANNEL_SETTING_LAST
};

enum ChannelTriggerSource {
  CHANNEL_TRIGGER_TR1,
  CHANNEL_TRIGGER_TR2,
  CHANNEL_TRIGGER_CONTINUOUS,
  CHANNEL_TRIGGER_LAST
};

enum ChannelCV_Mapping {
  CHANNEL_CV_MAPPING_CV1,
  CHANNEL_CV_MAPPING_CV2,
  CHANNEL_CV_MAPPING_CV3,
  CHANNEL_CV_MAPPING_CV4,
  CHANNEL_CV_MAPPING_LAST
};

enum CLOCKMODES 
{
  MULT,
  LFSR,
  RANDOM,
  EUCLID,
  LOGIC,
  SEQ,
  DAC,
  LAST_MODE
};

class Clocks : public settings::SettingsBase<Clocks, CHANNEL_SETTING_LAST> {
public:

  uint8_t get_mode() const {
    return values_[CHANNEL_SETTING_MODE];
  }

  uint8_t get_mode4() const {
    return values_[CHANNEL_SETTING_MODE4];
  }

  uint8_t get_clock_src() const {
    return values_[CHANNEL_SETTING_TRIGGER];
  }
  
  int8_t get_mult() const {
    return values_[CHANNEL_SETTING_MULT];
  }

  uint16_t get_pulsewidth() const {
    return values_[CHANNEL_SETTING_PULSEWIDTH];
  }

  uint8_t get_inverted() const {
    return values_[CHANNEL_SETTING_INVERTED];
  }

  uint16_t get_delay() const {
    return values_[CHANNEL_SETTING_DELAY];
  }

  uint8_t get_reset() const {
    return values_[CHANNEL_SETTING_RESET];
  }

  uint8_t lfsr_length() const {
    return values_[CHANNEL_SETTING_LFSR_LEN];
  }

  uint8_t lfsr_tap1() const {
    return values_[CHANNEL_SETTING_LFSR_TAP1];
  }

  uint8_t lfsr_tap2() const {
    return values_[CHANNEL_SETTING_LFSR_TAP2];
  }

  uint8_t rand_n() const {
    return values_[CHANNEL_SETTING_RAND_N];
  }

  uint8_t euclid_n() const {
    return values_[CHANNEL_SETTING_EUCLID_N];
  }

  uint8_t euclid_k() const {
    return values_[CHANNEL_SETTING_EUCLID_N];
  }

  uint8_t euclid_offset() const {
    return values_[CHANNEL_SETTING_EUCLID_OFFSET];
  }

  uint8_t logic_type() const {
    return values_[CHANNEL_SETTING_LOGIC_TYPE];
  }

  uint8_t logic_op1() const {
    return values_[CHANNEL_SETTING_LOGIC_OP1];
  }

  uint8_t logic_op2() const {
    return values_[CHANNEL_SETTING_LOGIC_OP2];
  }

  uint8_t dac_range() const {
    return values_[CHANNEL_SETTING_DAC_RANGE];
  }

  uint8_t dac_mode() const {
    return values_[CHANNEL_SETTING_DAC_MODE];
  }
  
  uint8_t get_turing_length() const {
    return values_[CHANNEL_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_prob() const {
    return values_[CHANNEL_SETTING_TURING_PROB];
  }

  uint8_t get_turing_range() const {
    return values_[CHANNEL_SETTING_TURING_RANGE];
  }

  uint8_t get_logistic_map_r() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_R];
  }

  uint8_t get_logistic_map_range() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_RANGE];
  }

  uint8_t get_logistic_map_seed() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_SEED];
  }

  uint16_t get_trigger_delay() const {
    return values_[CHANNEL_SETTING_DELAY];
  }
  
  ChannelTriggerSource get_trigger_source() const {
    return static_cast<ChannelTriggerSource>(values_[CHANNEL_SETTING_TRIGGER]);
  }

  void Init(ChannelTriggerSource trigger_source) {
    
    InitDefaults();
    apply_value(CHANNEL_SETTING_TRIGGER, trigger_source);

    force_update_ = true;
    trigger_delay_ = 0;
    ticks_ = 0;
    subticks_ = 0;
    frequency_in_ticks_ = (float)get_pulsewidth() * TICKS_TO_MS; // init to something...
    
    turing_machine_.Init();
    logistic_map_.Init();
    clock_display_.Init();
    update_enabled_settings(0);
  }
 
  void force_update() {
    force_update_ = true;
  }

  inline void Update(uint32_t triggers, CLOCK_CHANNEL clock_channel) {


     ticks_++; subticks_++;

     // ext clock ? 
     ChannelTriggerSource trigger_source = get_trigger_source();
     bool continous = CHANNEL_TRIGGER_CONTINUOUS == trigger_source;
     bool triggered = !continous && (triggers & DIGITAL_INPUT_MASK(trigger_source - CHANNEL_TRIGGER_TR1));
     
     if (triggered) {
        // reset
        frequency_in_ticks_ = ticks_;
        ticks_ = 0;
     }

     // update?
     
     uint32_t tock = (float)frequency_in_ticks_*multipliers_[get_mult()];

     uint8_t _update = subticks_ > tock ? true : false;

     uint16_t _out = 0x0;
      
     if (_update) {    
       
       // reset 
       subticks_ = 0x0;
       _out = output_state_ = 0x1;
      
     }

     // turn off?
     if (output_state_) {

        if (subticks_ > get_pulsewidth()*TICKS_TO_MS)
          _out = output_state_ = 0x0;
        else   
          _out = output_state_ = 0x1; 
     }
     
     
     TU::OUTPUTS::set(clock_channel, _out);
  } // end update

  uint8_t getTriggerState() const {
    return clock_display_.getState();
  }

  uint32_t get_shift_register() const {
    return turing_machine_.get_shift_register();
  }

  uint32_t get_logistic_map_register() const {
    return logistic_map_.get_register();
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }


  void update_enabled_settings(uint8_t channel_id) {

    
    ChannelSetting *settings = enabled_settings_;

    if (channel_id != _DAC_CHANNEL)
      *settings++ = CHANNEL_SETTING_MODE;
    else   
      *settings++ = CHANNEL_SETTING_MODE4;
      
    *settings++ = CHANNEL_SETTING_PULSEWIDTH;
    *settings++ = CHANNEL_SETTING_MULT;

    uint8_t mode = (channel_id != _DAC_CHANNEL) ? get_mode() : get_mode4();

    switch (mode) {

      case LFSR: 
       *settings++ = CHANNEL_SETTING_LFSR_LEN;
       *settings++ = CHANNEL_SETTING_LFSR_TAP1;
       *settings++ = CHANNEL_SETTING_LFSR_TAP2;
       break;
      case EUCLID:
       *settings++ = CHANNEL_SETTING_EUCLID_N;
       *settings++ = CHANNEL_SETTING_EUCLID_K;
       *settings++ = CHANNEL_SETTING_EUCLID_OFFSET;
       break; 
      case RANDOM:
       *settings++ = CHANNEL_SETTING_RAND_N;
       break;
      case LOGIC:
       *settings++ = CHANNEL_SETTING_LOGIC_TYPE;
       *settings++ = CHANNEL_SETTING_LOGIC_OP1;
       *settings++ = CHANNEL_SETTING_LOGIC_OP2;
       break; 
      case SEQ:
       break;
      case DAC:
        *settings++ = CHANNEL_SETTING_DAC_RANGE; 
        *settings++ = CHANNEL_SETTING_DAC_MODE;     
       break;  
      default:
       break;
    }
    
    *settings++ = CHANNEL_SETTING_INVERTED;
    *settings++ = CHANNEL_SETTING_DELAY;
    *settings++ = CHANNEL_SETTING_TRIGGER;
    *settings++ = CHANNEL_SETTING_RESET;

    num_enabled_settings_ = settings - enabled_settings_;
  }

  static bool indentSetting(ChannelSetting s) {
    switch (s) {
      case CHANNEL_SETTING_TURING_LENGTH:
      case CHANNEL_SETTING_TURING_RANGE:
      case CHANNEL_SETTING_TURING_PROB:
      case CHANNEL_SETTING_LOGISTIC_MAP_R:
      case CHANNEL_SETTING_LOGISTIC_MAP_RANGE:
      case CHANNEL_SETTING_LOGISTIC_MAP_SEED:
      case CHANNEL_SETTING_DELAY:
        return true;
      default: break;
    }
    return false;
  }


private:
  bool force_update_;
  uint16_t trigger_delay_;
  uint32_t ticks_;
  uint32_t subticks_;
  uint32_t frequency_in_ticks_;
  uint16_t output_state_;
  
  util::TuringShiftRegister turing_machine_;
  util::LogisticMap logistic_map_;
  int num_enabled_settings_;
  ChannelSetting enabled_settings_[CHANNEL_SETTING_LAST];
  TU::DigitalInputDisplay clock_display_;

};


const char* const channel_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "cont"
};

const char* const reset_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "off"
};

const char* const multipliers[] = {
  "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
};

const char* const clock_delays[] = {
  "off", "60us", "120us", "180us", "240us", "300us", "360us", "420us", "480us"
};

SETTINGS_DECLARE(Clocks, CHANNEL_SETTING_LAST) {
 
  { 0, 0, MODES-2, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { 0, 0, MODES-1, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR1, 0, CHANNEL_TRIGGER_LAST - 1, "clock src", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR2+1, 0, CHANNEL_TRIGGER_LAST - 1, "reset", reset_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 7, 0, 14, "mult/div", multipliers, settings::STORAGE_TYPE_U8 },
  { 10, 2, 255, "pulsewidth", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "invert", TU::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 8, "clock delay", clock_delays, settings::STORAGE_TYPE_U4 },
  //
  { 16, 3, 32, "LFSR length",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "LFSR tap1",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "LFSR tap2",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "rand > n", NULL, settings::STORAGE_TYPE_U8 },
  { 3, 3, 31, "euclid: N", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 1, 31, "euclid: K", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "euclid: OFFSET", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "logic type", TU::Strings::operators, settings::STORAGE_TYPE_U8 },
  { 0, 0, CHANNELS-1, "op_1", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 1, 0, CHANNELS-1, "op_2", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 1, 1, 31, "DAC: range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 3, "DAC: mode", TU::Strings::dac_modes, settings::STORAGE_TYPE_U4 },
  { 16, 1, 32, "TM length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "TM p(x)", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 1, 120, "TM range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "logistic r", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 1, 120, "logistic range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "logistic seed", NULL, settings::STORAGE_TYPE_U8 }
};

// WIP refactoring to better encapsulate and for possible app interface change
class ClocksState {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(CHANNEL_SETTING_MODE, CHANNEL_SETTING_LAST - 1);
  }

  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  int selected_channel;
  menu::ScreenCursor<menu::kScreenLines> cursor;
};

ClocksState clocks_state;
Clocks clocks[CHANNELS];

void CLOCKS_init() {

  clocks_state.Init();
  for (size_t i = 0; i < CHANNELS; ++i) {
    clocks[i].Init(static_cast<ChannelTriggerSource>(CHANNEL_TRIGGER_TR1));
  }
  clocks_state.cursor.AdjustEnd(clocks[0].num_enabled_settings() - 1);
 
}

size_t CLOCKS_storageSize() {
  return CHANNELS * Clocks::storageSize();
}

size_t CLOCKS_save(void *storage) {

  size_t used = 0;
  for (size_t i = 0; i < CHANNELS; ++i) {
    used += clocks[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}
size_t CLOCKS_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < CHANNELS; ++i) {
    used += clocks[i].Restore(static_cast<const char*>(storage) + used);
    clocks[i].update_enabled_settings(i);
  }
  clocks_state.cursor.AdjustEnd(clocks[0].num_enabled_settings() - 1);
  return used;
}

void CLOCKS_handleAppEvent(TU::AppEvent event) {
  switch (event) {
    case TU::APP_EVENT_RESUME:
      clocks_state.cursor.set_editing(false);
      break;
    case TU::APP_EVENT_SUSPEND:
    case TU::APP_EVENT_SCREENSAVER_ON:
    case TU::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void CLOCKS_loop() {
}

void CLOCKS_isr() {
  
  uint32_t triggers = TU::DigitalInputs::clocked();
  
  clocks[0].Update(triggers, CLOCK_CHANNEL_1);
  clocks[1].Update(triggers, CLOCK_CHANNEL_2);
  clocks[2].Update(triggers, CLOCK_CHANNEL_3);
  clocks[3].Update(triggers, CLOCK_CHANNEL_4);
  clocks[4].Update(triggers, CLOCK_CHANNEL_5);
  clocks[5].Update(triggers, CLOCK_CHANNEL_6);
}

void CLOCKS_handleButtonEvent(const UI::Event &event) {
  
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case TU::CONTROL_BUTTON_UP:
        CLOCKS_topButton();
        break;
      case TU::CONTROL_BUTTON_DOWN:
        CLOCKS_lowerButton();
        break;
      case TU::CONTROL_BUTTON_L:
        CLOCKS_leftButton();
        break;
      case TU::CONTROL_BUTTON_R:
        CLOCKS_rightButton();
        break;
    }
  } else {
    if (TU::CONTROL_BUTTON_L == event.control)
      CLOCKS_leftButtonLong();
  }
}

void CLOCKS_handleEncoderEvent(const UI::Event &event) {
 
  if (TU::CONTROL_ENCODER_L == event.control) {
    int selected_channel = clocks_state.selected_channel + event.value;
    CONSTRAIN(selected_channel, 0, CHANNELS-1);
    clocks_state.selected_channel = selected_channel;

    Clocks &selected = clocks[clocks_state.selected_channel];
    clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
    
  } else if (TU::CONTROL_ENCODER_R == event.control) {
    
       Clocks &selected = clocks[clocks_state.selected_channel];
    
       if (clocks_state.editing()) {
          ChannelSetting setting = selected.enabled_setting_at(clocks_state.cursor_pos());
          
           if (CHANNEL_SETTING_SEQ != setting) {
            if (selected.change_value(setting, event.value))
             selected.force_update();

            switch (setting) {
              case CHANNEL_SETTING_MODE:
              case CHANNEL_SETTING_MODE4:  
               selected.update_enabled_settings(clocks_state.selected_channel);
               clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
               break;
             default:
               break;
            }
        }
      } else {
      clocks_state.cursor.Scroll(event.value);
    }
  }
}

void CLOCKS_topButton() {

}

void CLOCKS_lowerButton() {

}

void CLOCKS_rightButton() {

  Clocks &selected = clocks[clocks_state.selected_channel];
  switch (selected.enabled_setting_at(clocks_state.cursor_pos())) {
   
    default:
      clocks_state.cursor.toggle_editing();
      break;
  }

}

void CLOCKS_leftButton() {

  Clocks &selected = clocks[clocks_state.selected_channel];
 
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);  
}

void CLOCKS_leftButtonLong() {
  
}

void CLOCKS_menu() {

  menu::SixTitleBar::Draw();
  
  for (int i = 0, x = 0; i < CHANNELS; ++i, x += 21) {

    const Clocks &channel = clocks[i];
    menu::SixTitleBar::SetColumn(i);
    graphics.print((char)('1' + i));
    graphics.movePrintPos(2, 0);
    //
    menu::SixTitleBar::DrawGateIndicator(i, channel.getTriggerState());
  }
  menu::SixTitleBar::Selected(clocks_state.selected_channel);

  const Clocks &channel = clocks[clocks_state.selected_channel];

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(clocks_state.cursor);
  
  menu::SettingsListItem list_item;

   while (settings_list.available()) {
    const int setting =
        channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = Clocks::value_attr(setting);

    switch (setting) {
      
      case CHANNEL_SETTING_SEQ:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, 1233, 2);
        list_item.DrawNoValue<false>(value, attr);
        break;
      default:
        list_item.DrawDefault(value, attr);
    }
  }
}

void CLOCKS_screensaver() {


}
