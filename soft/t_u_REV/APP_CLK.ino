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

const uint8_t MODES = 7;
const uint8_t DAC_MODES = 4;

const float TICKS_TO_MS = 16.6667f; // 1 tick = 60 us;
const uint32_t SCALE_PULSEWIDTH = 58982; // 0.9 for signed_multiply_32x16b
//const uint32_t TICKS_TO_MS = 43691; // 0.6667f for signed_multiply_32x16b


//  "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
const float multipliers_[] = {
   8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.5f, 0.333333f, 0.25f, 0.2f, 0.166667f, 0.142857f, 0.125f
};

/*
const int32_t multipliers_[] = {

  0x80000000, 0x70000000, 0x60000000, 0x50000000, 0x40000000, 0x30000000, 0x20000000, 
  0x10000000, // = 1
  0x8000000,  // 1/2
  0x5555555,  // 1/3
  0x4000000,  // 1/4
  0x3333333,  // 1/5
  0x2AAAAAB,  // 1/6
  0x2492492,  // 1/7
  0x2000000   // 1/8
}; // float-multiplier / 8.0f * 2^31
*/

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
  CHANNEL_SETTING_HISTORY_WEIGHT,
  CHANNEL_SETTING_HISTORY_DEPTH,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
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

enum DACMODES 
{
  _BINARY,
  _RANDOM,
  _TURING,
  _LOGISTIC,
  LAST_DACMODE
};

enum CLOCKSTATES
{
  OFF,
  ON = 4095
};

class Clock_channel : public settings::SettingsBase<Clock_channel, CHANNEL_SETTING_LAST> {
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

  uint8_t get_tap1() const {
    return values_[CHANNEL_SETTING_LFSR_TAP1];
  }

  uint8_t get_tap2() const {
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

  uint8_t get_history_weight() const {
    return values_[CHANNEL_SETTING_HISTORY_WEIGHT];
  }

  uint8_t get_history_depth() const {
    return values_[CHANNEL_SETTING_HISTORY_DEPTH];
  }
  
  uint8_t get_turing_length() const {
    return values_[CHANNEL_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_probability() const {
    return values_[CHANNEL_SETTING_TURING_PROB];
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
    output_state_ = OFF;
    trigger_delay_ = 0;
    ticks_ = 0;
    subticks_ = 0;
    totalticks_ = 0;
 
    prev_multiplier_ = 0;
    prev_pulsewidth_ = get_pulsewidth();
    
    ext_frequency_in_ticks_ = get_pulsewidth() << 5; // init to something...
    channel_frequency_in_ticks_ = get_pulsewidth() << 5;
    pulse_width_in_ticks_ = get_pulsewidth() << 4;
     
    _ZERO = TU::calibration_data.dac.calibrated_Zero[0x0][0x0];
    
    turing_machine_.Init();
    logistic_map_.Init();
    clock_display_.Init();
    update_enabled_settings(0);
  }
 
  void force_update() {
    force_update_ = true;
  }

  inline void Update(uint32_t triggers, CLOCK_CHANNEL clock_channel) {

     ticks_++; subticks_++; totalticks_++;
     
     ChannelTriggerSource trigger_source = get_trigger_source();
     
     bool _continous = CHANNEL_TRIGGER_CONTINUOUS == trigger_source;
     bool _triggered = !_continous && (triggers & DIGITAL_INPUT_MASK(trigger_source - CHANNEL_TRIGGER_TR1));
     bool _tock = false;
     uint8_t _multiplier = get_mult();
     uint8_t _mode = (clock_channel != CLOCK_CHANNEL_4) ? get_mode() : get_mode4();
     uint16_t _output = OFF;

     // ext clock ? 
     if (_triggered) {
        // new frequency; and reset: 
        ext_frequency_in_ticks_ = ticks_;
        ticks_ = 0;  
        _tock = true;  
     }
     
     // new multiplier ?
     if (prev_multiplier_ != _multiplier)
       _tock |= true;  
     prev_multiplier_ = _multiplier; 

     // recalculate channel frequency:
     //if (_tock) 
       // channel_frequency_in_ticks_ = multiply_32x32_rshift32_rounded(ext_frequency_in_ticks_, multipliers_[_multiplier]) << 4; // << 1 : fract mult; << 3 : times 8
       // or i suppose better to use multiply_u32xu32_rshift32
     if (_tock) 
        channel_frequency_in_ticks_ = (uint32_t)(0.5f + (float)ext_frequency_in_ticks_*multipliers_[_multiplier]);
        
     // time to output ? 
     if (subticks_ >= channel_frequency_in_ticks_) { 

         // if so, reset ticks: 
         subticks_ = 0x0;
         // ... and turn on ? 
         _output =  output_state_ = process_clock_channel(_mode); // = either ON, OFF, or anything (DAC)
     }

     // on/off...?
     if (output_state_ && _mode < DAC) { 

        // recalculate pulsewidth ? 
        uint8_t _pulsewidth = get_pulsewidth();
        if (prev_pulsewidth_ != _pulsewidth)
           pulse_width_in_ticks_ = (float)_pulsewidth*TICKS_TO_MS;
           // int32_t p = signed_multiply_32x16b(TICKS_TO_MS, _pulsewidth); // = * 0.6667f
           // p = signed_saturate_rshift(p, 16, 0);
           // pulse_width_in_ticks_  = (_pulsewidth << 4) + p;
        prev_pulsewidth_ = _pulsewidth;
        
        // limit pulsewidth, if need be:
        if (pulse_width_in_ticks_ >= channel_frequency_in_ticks_) {
            pulse_width_in_ticks_ = signed_multiply_32x16b(channel_frequency_in_ticks_, SCALE_PULSEWIDTH); // = * 0.9f
            pulse_width_in_ticks_ = signed_saturate_rshift(pulse_width_in_ticks_, 16, 0);
        }
        
        // turn off output? 
        if (subticks_ >= pulse_width_in_ticks_) 
          _output = output_state_ = OFF;
        else // keep on 
          _output = ON; 
     }

     // DAC channel needs extra treatment / zero offset: 
     if (clock_channel == CLOCK_CHANNEL_4 && _mode < DAC && output_state_ == OFF)
       _output += _ZERO;
       
     // update outputs: 
     TU::OUTPUTS::set(clock_channel, _output);
  } // end update


  inline uint16_t process_clock_channel(uint8_t mode) {
 
      uint16_t _out = ON;
  
      switch (mode) {
  
          case MULT:
          case LOGIC:
            break;
          case LFSR: {

              uint8_t _length, _probability, _myfirstbit;
              
              _length = get_turing_length();
              _probability = get_turing_probability();
              
              turing_machine_.set_length(_length);
              turing_machine_.set_probability(_probability); 
              
              uint32_t _shift_register = turing_machine_.Clock();
  
              _myfirstbit =  _shift_register & 1u; // --> output
              _out = _myfirstbit ? ON : OFF; // DAC needs special care ... 
              
              // now update LFSR (even more):
              
              uint8_t  _tap1 = get_tap1(); 
              uint8_t  _tap2 = get_tap2(); 

              // should be constrained via UI -
              if (_tap1 >= _length)
                _tap1 = _length;
              if (_tap2 >= _length)
                _tap2 = _length;
     
              _tap1 = (_shift_register >> _tap1) & 1u;  // bit at tap1
              _tap2 = (_shift_register >> _tap2) & 1u;  // bit at tap1
  
              _shift_register = (_shift_register >> 1) | ((_myfirstbit ^ _tap1 ^ _tap2) << (_length - 1)); 
              turing_machine_.set_shift_register(_shift_register);
            }
            break;
          case RANDOM: {
            // mmh, is this really worth keeping?
               uint8_t _n = rand_n();
               
               _out = (analogRead(A12) - analogRead(A13) + random(_n + 1)) & 1u;
               _out = _out ? ON : OFF; // DAC needs special care ... 
            }
            break;
          case EUCLID: {

              uint8_t _n, _k, _offset;
              
              _n = euclid_n();
              _k = euclid_k();
              _offset = euclid_offset();

              if (_k >= _n ) 
                _k = _n - 1; // should be constrained via UI
                
              _out = ((totalticks_ + _offset) * _k) % _n;
              _out = (_out < _k) ? ON : OFF;
            }
            break;
          case SEQ:
            break; 
          case DAC: {

               int16_t _range = dac_range(); // 1-255

               switch (dac_mode()) {

                case _BINARY: {

                   int16_t _binary = 0;
                  
                   _binary += (TU::OUTPUTS::value(CLOCK_CHANNEL_1) & 1u) << 4;
                   _binary += (TU::OUTPUTS::value(CLOCK_CHANNEL_2) & 1u) << 3;
                   _binary += (TU::OUTPUTS::value(CLOCK_CHANNEL_3) & 1u) << 2;
                   // CLOCK_CHANNEL_4 = DAC
                   _binary += (TU::OUTPUTS::value(CLOCK_CHANNEL_5) & 1u) << 1;
                   _binary += (TU::OUTPUTS::value(CLOCK_CHANNEL_6) & 1u);

                   _binary = (_binary << 7) - 0x800; // +/- 2048
                   _binary = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _binary);
                   _binary = signed_saturate_rshift(_binary, 16, 0);
                   _out = _ZERO - _binary;
                 }
                 break;
                case _RANDOM: {

                   uint16_t _history[TU::OUTPUTS::kHistoryDepth];
                   int16_t _rand_history;
                   int16_t _rand_new;

                   TU::OUTPUTS::getHistory<CLOCK_CHANNEL_4>(_history);
                   
                   _rand_history = calc_average(_history, get_history_depth());     
                   _rand_history = signed_multiply_32x16b((static_cast<int32_t>(get_history_weight()) * 65535U) >> 8, _rand_history);
                   _rand_history = signed_saturate_rshift(_rand_history, 16, 0) - 0x800; // +/- 2048
                   
                   _rand_new =  (analogRead(A12) - analogRead(A13)) + random(0xFFF) - 0x800; // +/- 2048
                   _rand_new = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _rand_new);
                   _rand_new = signed_saturate_rshift(_rand_new, 16, 0);
                   _out = _ZERO - (_rand_new + _rand_history);
                 }
                 break;
                case _TURING: {

                   uint8_t _length = get_turing_length();
                   uint8_t _probability = get_turing_probability();
                
                   turing_machine_.set_length(_length);
                   turing_machine_.set_probability(_probability); 
                
                   int16_t _shift_register = (static_cast<int16_t>(turing_machine_.Clock()) & 0xFFF) - 0x800; // +/- 2048
                   _shift_register = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _shift_register);
                   _shift_register = signed_saturate_rshift(_shift_register, 16, 0);
                   _out = _ZERO - _shift_register;
                 }
                 break;
                case _LOGISTIC:{

                  
                 }
                 break;
                default:
                 break;   
               }   // end DAC mode switch 
            }
            break;
          default:
            break; // end mode switch       
      }
      return _out; 
  }

  inline uint16_t calc_average(const uint16_t *data, uint8_t depth) {
    uint32_t sum = 0;
    uint8_t n = depth;
    while (n--)
      sum += *data++;
    return sum / depth;
  }
  
  uint8_t getTriggerState() const {
    return clock_display_.getState();
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

    if (channel_id != CLOCK_CHANNEL_4)
      *settings++ = CHANNEL_SETTING_MODE;
    else   
      *settings++ = CHANNEL_SETTING_MODE4;
      
    *settings++ = CHANNEL_SETTING_PULSEWIDTH;
    *settings++ = CHANNEL_SETTING_MULT;

    uint8_t mode = (channel_id != CLOCK_CHANNEL_4) ? get_mode() : get_mode4();

    switch (mode) {

      case LFSR: 
       *settings++ = CHANNEL_SETTING_TURING_LENGTH;
       *settings++ = CHANNEL_SETTING_TURING_PROB;
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
        *settings++ = CHANNEL_SETTING_DAC_MODE; 
        *settings++ = CHANNEL_SETTING_DAC_RANGE;    
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

/*
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
*/

private:
  bool force_update_;
  uint16_t _ZERO;
  uint16_t trigger_delay_;
  int32_t ticks_;
  int32_t subticks_;
  uint32_t totalticks_;
  int32_t ext_frequency_in_ticks_;
  int32_t channel_frequency_in_ticks_;
  int32_t pulse_width_in_ticks_;
  uint16_t output_state_;
  uint8_t prev_multiplier_;
  uint8_t prev_pulsewidth_;
 
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

SETTINGS_DECLARE(Clock_channel, CHANNEL_SETTING_LAST) {
 
  { 0, 0, MODES - 2, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { 0, 0, MODES - 1, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR1, 0, CHANNEL_TRIGGER_LAST - 1, "clock src", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR2 + 1, 0, CHANNEL_TRIGGER_LAST - 1, "reset", reset_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 7, 0, 14, "mult/div", multipliers, settings::STORAGE_TYPE_U8 },
  { 10, 1, 255, "pulsewidth", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "invert", TU::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 8, "clock delay", clock_delays, settings::STORAGE_TYPE_U4 },
  //
  { 0, 0, 31, "LFSR tap1",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "LFSR tap2",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "rand > n", NULL, settings::STORAGE_TYPE_U8 },
  { 3, 3, 31, "euclid: N", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 1, 31, "euclid: K", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "euclid: OFFSET", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "logic type", TU::Strings::operators, settings::STORAGE_TYPE_U8 },
  { 0, 0, NUM_CHANNELS - 1, "op_1", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 1, 0, NUM_CHANNELS - 1, "op_2", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "DAC: range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, DAC_MODES-1, "DAC: mode", TU::Strings::dac_modes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 255, "rnd hist.", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { 0, 0, TU::OUTPUTS::kHistoryDepth - 1, "hist. depth", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { 16, 1, 32, "LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "LFSR p(x)", NULL, settings::STORAGE_TYPE_U8 },
  //{ 24, 1, 120, "LFSR range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "logistic r", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 1, 120, "logistic range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "logistic seed", NULL, settings::STORAGE_TYPE_U8 }
};


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
Clock_channel clock_channel[NUM_CHANNELS];

void CLOCKS_init() {

  clocks_state.Init();
  for (size_t i = 0; i < NUM_CHANNELS; ++i) {
    clock_channel[i].Init(static_cast<ChannelTriggerSource>(CHANNEL_TRIGGER_TR1));
  }
  clocks_state.cursor.AdjustEnd(clock_channel[0].num_enabled_settings() - 1);
 
}

size_t CLOCKS_storageSize() {
  return NUM_CHANNELS * Clock_channel::storageSize();
}

size_t CLOCKS_save(void *storage) {

  size_t used = 0;
  for (size_t i = 0; i < NUM_CHANNELS; ++i) {
    used += clock_channel[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}
size_t CLOCKS_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUM_CHANNELS; ++i) {
    used += clock_channel[i].Restore(static_cast<const char*>(storage) + used);
    clock_channel[i].update_enabled_settings(i);
  }
  clocks_state.cursor.AdjustEnd(clock_channel[0].num_enabled_settings() - 1);
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
  
  clock_channel[0].Update(triggers, CLOCK_CHANNEL_1);
  clock_channel[1].Update(triggers, CLOCK_CHANNEL_2);
  clock_channel[2].Update(triggers, CLOCK_CHANNEL_3);
  // ...
  clock_channel[4].Update(triggers, CLOCK_CHANNEL_5);
  clock_channel[5].Update(triggers, CLOCK_CHANNEL_6);
  // update channel 4 last, because of the binary Sequencer thing:
  clock_channel[3].Update(triggers, CLOCK_CHANNEL_4);
 
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
    CONSTRAIN(selected_channel, 0, NUM_CHANNELS-1);
    clocks_state.selected_channel = selected_channel;

    Clock_channel &selected = clock_channel[clocks_state.selected_channel];
    clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
    
  } else if (TU::CONTROL_ENCODER_R == event.control) {
    
       Clock_channel &selected = clock_channel[clocks_state.selected_channel];
    
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

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];
  switch (selected.enabled_setting_at(clocks_state.cursor_pos())) {
   
    default:
      clocks_state.cursor.toggle_editing();
      break;
  }

}

void CLOCKS_leftButton() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];
 
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);  
}

void CLOCKS_leftButtonLong() {
  
}

void CLOCKS_menu() {

  menu::SixTitleBar::Draw();
  
  for (int i = 0, x = 0; i < NUM_CHANNELS; ++i, x += 21) {

    const Clock_channel &channel = clock_channel[i];
    menu::SixTitleBar::SetColumn(i);
    graphics.print((char)('1' + i));
    graphics.movePrintPos(2, 0);
    //
    menu::SixTitleBar::DrawGateIndicator(i, channel.getTriggerState());
  }
  menu::SixTitleBar::Selected(clocks_state.selected_channel);

  const Clock_channel &channel = clock_channel[clocks_state.selected_channel];

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(clocks_state.cursor);
  
  menu::SettingsListItem list_item;

   while (settings_list.available()) {
    const int setting =
        channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = Clock_channel::value_attr(setting);

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
