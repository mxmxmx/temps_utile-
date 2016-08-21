/* 
*   TODO
* - prevent channels getting out of sync (mult/div) [-> offset]
* - invert (? or maybe just get rid of it)
* - something's not quite right with LFSR mode
* - expand to div/16
* - implement reset
* - implement CV 
* - pattern seq: 
*    - get rid of pattern/mask terminology confusion (ie "mask" = pattern; pattern = pulsewidth pattern)
*    - user patterns per channel
*    - implement pulsewidth patterns
*    - store pattern when contracting/re-expanding pattern length (?)
* - menu details: 
*    - constrain interdependent values
*    - display clocks, patterns, etc
* - DAC mode should have additional/internal trigger sources: channels 1-3, 5, and 6
* - fix screensaver DAC channel [DAC mode]
* - make screen saver nice ...
*
*/

#include "util/util_settings.h"
#include "util/util_turing.h"
#include "util/util_logistic_map.h"
#include "TU_apps.h"
#include "TU_outputs.h"
#include "TU_menus.h"
#include "TU_strings.h"
#include "TU_visualfx.h"
#include "TU_pattern_edit.h"
#include "TU_patterns.h"
#include "extern/dspinst.h"

namespace menu = TU::menu; 

const uint8_t MODES = 7;        // # clock modes
const uint8_t DAC_MODES = 4;    // # DAC submodes
const uint8_t RND_MAX = 31;     // max random (n)

const uint32_t SCALE_PULSEWIDTH = 58982; // 0.9 for signed_multiply_32x16b
const uint32_t TICKS_TO_MS = 43691; // 0.6667f : fraction, if TU_CORE_TIMER_RATE = 60 us : 65536U * ((1000 / TU_CORE_TIMER_RATE) - 16)
const uint32_t TICK_JITTER = 0xFFFFFFF; // 1/16 : threshold/double triggers reject -> ext_frequency_in_ticks_


uint32_t ticks_src1 = 0; // main clock frequency (top)
uint32_t ticks_src2 = 0; // sec. clock frequency (bottom)

const uint64_t multipliers_[] = {

  0x100000000,// /8
  0xE0000000, // /7
  0xC0000000, // /6
  0xA0000000, // /5
  0x80000000, // /4
  0x60000000, // /3
  0x40000000, // /2
  0x20000000, // x1
  0x10000000, // x2
  0xAAAAAAB,  // x3
  0x8000000,  // x4
  0x6666667,  // x5
  0x5555556,  // x6
  0x4924926,  // x7
  0x4000000   // x8
}; // = multiplier / 8.0f * 2^32

enum ChannelSetting { 
  // shared
  CHANNEL_SETTING_MODE,
  CHANNEL_SETTING_MODE4,
  CHANNEL_SETTING_CLOCK,
  CHANNEL_SETTING_RESET,
  CHANNEL_SETTING_MULT,
  CHANNEL_SETTING_PULSEWIDTH,
  // mode specific
  CHANNEL_SETTING_LFSR_TAP1,
  CHANNEL_SETTING_LFSR_TAP2,
  CHANNEL_SETTING_RAND_N,
  CHANNEL_SETTING_EUCLID_N,
  CHANNEL_SETTING_EUCLID_K,
  CHANNEL_SETTING_EUCLID_OFFSET,
  CHANNEL_SETTING_LOGIC_TYPE,
  CHANNEL_SETTING_LOGIC_OP1,
  CHANNEL_SETTING_LOGIC_OP2,
  CHANNEL_SETTING_LOGIC_TRACK_WHAT,
  CHANNEL_SETTING_DAC_RANGE,
  CHANNEL_SETTING_DAC_MODE,
  CHANNEL_SETTING_DAC_TRACK_WHAT,
  CHANNEL_SETTING_HISTORY_WEIGHT,
  CHANNEL_SETTING_HISTORY_DEPTH,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
  CHANNEL_SETTING_LOGISTIC_MAP_R,
  CHANNEL_SETTING_CLOCK_MASK,
  CHANNEL_SETTING_CLOCK_PATTERN,
  // cv sources
  CHANNEL_SETTING_MULT_CV_SOURCE,
  CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE,
  CHANNEL_SETTING_CLOCK_CV_SOURCE,
  CHANNEL_SETTING_LFSR_TAP1_CV_SOURCE,
  CHANNEL_SETTING_LFSR_TAP2_CV_SOURCE,
  CHANNEL_SETTING_RAND_N_CV_SOURCE,
  CHANNEL_SETTING_EUCLID_N_CV_SOURCE,
  CHANNEL_SETTING_EUCLID_K_CV_SOURCE,
  CHANNEL_SETTING_EUCLID_OFFSET_CV_SOURCE,
  CHANNEL_SETTING_LOGIC_TYPE_CV_SOURCE,
  CHANNEL_SETTING_LOGIC_OP1_CV_SOURCE,
  CHANNEL_SETTING_LOGIC_OP2_CV_SOURCE,
  CHANNEL_SETTING_TURING_PROB_CV_SOURCE,
  CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE,
  CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE,
  CHANNEL_SETTING_SEQ_CV_SOURCE,
  CHANNEL_SETTING_MASK_CV_SOURCE,
  CHANNEL_SETTING_DAC_RANGE_CV_SOURCE,
  CHANNEL_SETTING_DAC_MODE_CV_SOURCE,
  CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE,
  CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE,
  CHANNEL_SETTING_DUMMY,
  CHANNEL_SETTING_LAST
};
  
enum ChannelTriggerSource {
  CHANNEL_TRIGGER_TR1,
  CHANNEL_TRIGGER_TR2,
  CHANNEL_TRIGGER_NONE,
  CHANNEL_TRIGGER_LAST
};

enum ChannelCV_Mapping {
  CHANNEL_CV_MAPPING_CV1,
  CHANNEL_CV_MAPPING_CV2,
  CHANNEL_CV_MAPPING_CV3,
  CHANNEL_CV_MAPPING_CV4,
  CHANNEL_CV_MAPPING_LAST
};

enum CLOCKMODES {
  MULT,
  LFSR,
  RANDOM,
  EUCLID,
  LOGIC,
  SEQ,
  DAC,
  LAST_MODE
};

enum DACMODES {
  _BINARY,
  _RANDOM,
  _TURING,
  _LOGISTIC,
  LAST_DACMODE
};

enum CLOCKSTATES {
  OFF,
  ON = 4095
};

enum LOGICMODES {
  AND,
  OR,
  XOR,
  XNOR,
  NAND,
  NOR
};

enum MENUPAGES {
  PARAMETERS,
  CV_SOURCES,
  TEMPO
};

uint64_t ext_frequency[CHANNEL_TRIGGER_LAST];

class Clock_channel : public settings::SettingsBase<Clock_channel, CHANNEL_SETTING_LAST> {
public:

  uint8_t get_mode() const {
    return values_[CHANNEL_SETTING_MODE];
  }

  uint8_t get_mode4() const {
    return values_[CHANNEL_SETTING_MODE4];
  }

  uint8_t get_clock_source() const {
    return values_[CHANNEL_SETTING_CLOCK];
  }
  
  int8_t get_mult() const {
    return values_[CHANNEL_SETTING_MULT];
  }

  uint16_t get_pulsewidth() const {
    return values_[CHANNEL_SETTING_PULSEWIDTH];
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
    return values_[CHANNEL_SETTING_EUCLID_K];
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

  uint8_t logic_tracking() const {
    return values_[CHANNEL_SETTING_LOGIC_TRACK_WHAT];
  }

  uint8_t dac_range() const {
    return values_[CHANNEL_SETTING_DAC_RANGE];
  }

  uint8_t dac_mode() const {
    return values_[CHANNEL_SETTING_DAC_MODE];
  }

  uint8_t binary_tracking() const {
    return values_[CHANNEL_SETTING_DAC_TRACK_WHAT];
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

  int get_pattern() const {
    return values_[CHANNEL_SETTING_CLOCK_PATTERN];
  }

  uint8_t get_mult_cv_source() const {
    return values_[CHANNEL_SETTING_MULT_CV_SOURCE];
  }

  uint8_t get_pulsewidth_cv_source() const {
    return values_[CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE];
  }

  uint8_t get_clock_cv_source() const {
    return values_[CHANNEL_SETTING_CLOCK_CV_SOURCE];
  }

  uint8_t get_tap1_cv_source() const {
    return values_[CHANNEL_SETTING_LFSR_TAP1_CV_SOURCE];
  }

  uint8_t get_tap2_cv_source() const {
    return values_[CHANNEL_SETTING_LFSR_TAP2_CV_SOURCE];
  }

  uint8_t get_rand_n_cv_source() const {
    return values_[CHANNEL_SETTING_RAND_N_CV_SOURCE];
  }

  uint8_t get_euclid_n_cv_source() const {
    return values_[CHANNEL_SETTING_EUCLID_N_CV_SOURCE];
  }

  uint8_t get_euclid_k_cv_source() const {
    return values_[CHANNEL_SETTING_EUCLID_K_CV_SOURCE];
  }

  uint8_t get_euclid_offset_cv_source() const {
    return values_[CHANNEL_SETTING_EUCLID_OFFSET_CV_SOURCE];
  }

  uint8_t get_logic_type_cv_source() const {
    return values_[CHANNEL_SETTING_LOGIC_TYPE_CV_SOURCE];
  }

  uint8_t get_logic_op1_cv_source() const {
    return values_[CHANNEL_SETTING_LOGIC_OP1_CV_SOURCE];
  }

  uint8_t get_logic_op2_cv_source() const {
    return values_[CHANNEL_SETTING_LOGIC_OP2_CV_SOURCE];
  }

  uint8_t get_turing_prob_cv_source() const {
    return values_[CHANNEL_SETTING_TURING_PROB_CV_SOURCE];
  }

  uint8_t get_turing_length_cv_source() const {
    return values_[CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE];
  }

  uint8_t get_logistic_map_r_cv_source() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE];
  }

  uint8_t get_mask_cv_source() const {
    return values_[CHANNEL_SETTING_MASK_CV_SOURCE];
  }

  uint8_t get_seq_cv_source() const {
    return values_[CHANNEL_SETTING_SEQ_CV_SOURCE];
  }
  
  void set_pattern(int pattern) {
    if (pattern != get_pattern()) {
      const TU::Pattern &pattern_def = TU::Patterns::GetPattern(pattern);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << pattern_def.num_slots)))
        mask |= 0x1;
      apply_value(CHANNEL_SETTING_CLOCK_MASK, mask);
      apply_value(CHANNEL_SETTING_CLOCK_PATTERN, pattern);
    }
  }

  int get_mask() const {
    return values_[CHANNEL_SETTING_CLOCK_MASK];
  }
  
  // wrappers for PatternEdit
  void pattern_changed() {
    force_update_ = true;
  }

  void update_pattern_mask(uint16_t mask) {
    apply_value(CHANNEL_SETTING_CLOCK_MASK, mask); 
  }

  uint16_t get_rotated_mask() const {
    return last_mask_;
  }

  uint16_t get_clock_cnt() const {
    return clk_cnt_;
  }

  void clear_CV_mapping() {

    apply_value(CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_MULT_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_CLOCK_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LFSR_TAP1_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LFSR_TAP2_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_RAND_N_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_EUCLID_N_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_EUCLID_K_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_EUCLID_OFFSET_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LOGIC_TYPE_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LOGIC_OP1_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LOGIC_OP2_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_TURING_PROB_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_SEQ_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_MASK_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_DAC_RANGE_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_DAC_MODE_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE,0);
    apply_value(CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE,0);
  }

  void page() {

     switch (menu_page_) {
          case PARAMETERS:
            menu_page_ = CV_SOURCES;
            break;
          case CV_SOURCES:
            clear_CV_mapping();
            break;
          case TEMPO:
            menu_page_ = CV_SOURCES;
            break;
          default: 
            break;      
     }
  }

  uint8_t get_page() const {
    return menu_page_;  
  }

  void set_page(uint8_t _page) {
    menu_page_ = _page;  
  }
  
  void Init(ChannelTriggerSource trigger_source) {
    
    InitDefaults();
    apply_value(CHANNEL_SETTING_CLOCK, trigger_source);

    force_update_ = true;
    gpio_state_ = OFF;
    ticks_ = 0;
    subticks_ = 0;
    tickjitter_ = 100;
    clk_cnt_ = 0;
    clk_src_ = 0;
    logic_ = false;
    menu_page_ = PARAMETERS;
 
    prev_multiplier_ = 0;
    prev_pulsewidth_ = get_pulsewidth();
    
    ext_frequency_in_ticks_ = get_pulsewidth() << 15; // init to something...
    channel_frequency_in_ticks_ = get_pulsewidth() << 15;
    pulse_width_in_ticks_ = get_pulsewidth() << 10;
     
    _ZERO = TU::calibration_data.dac.calibrated_Zero[0x0][0x0];
    
    turing_machine_.Init();
    logistic_map_.Init();
    uint32_t _seed = TU::ADC::value<ADC_CHANNEL_1>() + TU::ADC::value<ADC_CHANNEL_2>() + TU::ADC::value<ADC_CHANNEL_3>() + TU::ADC::value<ADC_CHANNEL_4>();
    randomSeed(_seed);
    logistic_map_.set_seed(_seed);
    clock_display_.Init();
    update_enabled_settings(0);
  }
 
  void force_update() {
    force_update_ = true;
  }

  inline void Update(uint32_t triggers, CLOCK_CHANNEL clock_channel) {

     subticks_++; 
     
     uint8_t _clock_source, _multiplier, _mode;
     bool _none, _triggered, _tock, _sync;
     uint16_t _output = gpio_state_;

     // channel parameters:
     _clock_source = get_clock_source();
     _multiplier = get_mult();
     _mode = (clock_channel != CLOCK_CHANNEL_4) ? get_mode() : get_mode4();
     // clocked ?
     _none = CHANNEL_TRIGGER_NONE == _clock_source;
     _triggered = !_none && (triggers & DIGITAL_INPUT_MASK(_clock_source - CHANNEL_TRIGGER_TR1));
     _tock = false;
     _sync = false;
     // new tick frequency:
     if (_triggered || clk_src_ != _clock_source) {   
        ext_frequency_in_ticks_ = ext_frequency[_clock_source]; 
        _tock = true;
        div_cnt_--;
     }
     // store clock source:
     clk_src_ = _clock_source;
    
     // new multiplier ?
     if (prev_multiplier_ != _multiplier)
       _tock |= true;  
     prev_multiplier_ = _multiplier; 

     // recalculate channel frequency and jitter-threshold:
     if (_tock) {
        channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(ext_frequency_in_ticks_, multipliers_[_multiplier]) << 3; 
        tickjitter_ = multiply_u32xu32_rshift32(channel_frequency_in_ticks_, TICK_JITTER);
     }
     // limit to > 0
     if (!channel_frequency_in_ticks_)  
        channel_frequency_in_ticks_ = 1u;  
    /*             
     *  brute force ugly sync hack:
     *  this, presumably, is needlessly complicated. 
     *  but seems to work ok-ish, w/o too much jitter and missing clocks... 
     */
     uint32_t _subticks = subticks_;
     
     if (_multiplier < 8 && _triggered && div_cnt_ <= 0) { // division, so we track
        _sync = true;
        div_cnt_ = 8 - _multiplier; // /1 = 7 ; /2 = 6, /3 = 5 etc
        subticks_ = channel_frequency_in_ticks_; // force sync
     }
     else if (_multiplier < 8 && _triggered) {
        // division, mute output:
        TU::OUTPUTS::setState(clock_channel, OFF);
     }
     else if (_multiplier > 7 && _triggered)  {
        _sync = true;
        subticks_ = channel_frequency_in_ticks_; // force sync, if clocked
     }
     else if (_multiplier > 7)
        _sync = true;   
     // end of ugly hack
     
     // time to output ? 
     if (subticks_ >= channel_frequency_in_ticks_ && _sync) { 

         // if so, reset ticks: 
         subticks_ = 0x0;
         // count, only if ... 
         if (_subticks < tickjitter_) // reject? .. 
            return;
            
         clk_cnt_++;  
         _output = gpio_state_ = process_clock_channel(_mode); // = either ON, OFF, or anything (DAC)
         TU::OUTPUTS::setState(clock_channel, _output);
     }
  
     // on/off...?
     if (gpio_state_ && _mode != DAC) { 

        uint8_t _pulsewidth = get_pulsewidth();
        // recalculate pulsewidth ? 
        if (prev_pulsewidth_ != _pulsewidth || ! subticks_) {
            int32_t _fraction = signed_multiply_32x16b(TICKS_TO_MS, static_cast<int32_t>(_pulsewidth)); // = * 0.6667f
            _fraction = signed_saturate_rshift(_fraction, 16, 0);
            pulse_width_in_ticks_  = (_pulsewidth << 4) + _fraction;
        }
        prev_pulsewidth_ = _pulsewidth;
        
        // limit pulsewidth
        if (pulse_width_in_ticks_ >= channel_frequency_in_ticks_) 
          pulse_width_in_ticks_ = (channel_frequency_in_ticks_ >> 1) | 1u;
          
        // turn off output? 
        if (subticks_ >= pulse_width_in_ticks_) 
          _output = gpio_state_ = OFF;
        else // keep on 
          _output = ON; 
     }

     // DAC channel needs extra treatment / zero offset: 
     if (clock_channel == CLOCK_CHANNEL_4 && _mode != DAC && gpio_state_ == OFF)
       _output += _ZERO;
       
     // update (physical) outputs:
     TU::OUTPUTS::set(clock_channel, _output);
  } // end update

  inline uint16_t process_clock_channel(uint8_t mode) {
 
      uint16_t _out = ON;
      logic_ = false;
  
      switch (mode) {
  
          case MULT:
            break;
          case LOGIC:
          // logic happens elsewhere.
            logic_ = true;
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
            
               int16_t _n = rand_n(); // threshold  
               int16_t _rand_new = random(RND_MAX);
               
               _out = _rand_new > _n ? ON : OFF; // DAC needs special care ... 
            }
            break;
          case EUCLID: {

              uint8_t _n, _k, _offset;
              
              _n = euclid_n();
              _k = euclid_k();
              _offset = euclid_offset();
                
              _out = ((clk_cnt_ + _offset) * _k) % _n;
              _out = (_out < _k) ? ON : OFF;
            }
            break;
          case SEQ: {

              uint16_t _mask = get_mask();
              const TU::Pattern &_pattern = TU::Patterns::GetPattern(get_pattern());
              
              if (clk_cnt_ >= _pattern.num_slots)
                clk_cnt_ = 0; // reset counter
              // pulse_width = _pattern.slots[clk_cnt_];
              _out = (_mask >> clk_cnt_) & 1u;
              _out = _out ? ON : OFF;
            }   
            break; 
          case DAC: {

               int16_t _range = dac_range(); // 1-255

               switch (dac_mode()) {

                  case _BINARY: {
  
                     uint8_t _binary = 0;
                     // MSB .. LSB
                     if (binary_tracking()) {
                       _binary |= (TU::OUTPUTS::state(CLOCK_CHANNEL_1) & 1u) << 4;
                       _binary |= (TU::OUTPUTS::state(CLOCK_CHANNEL_2) & 1u) << 3;
                       _binary |= (TU::OUTPUTS::state(CLOCK_CHANNEL_3) & 1u) << 2;
                       _binary |= (TU::OUTPUTS::state(CLOCK_CHANNEL_5) & 1u) << 1;
                       _binary |= (TU::OUTPUTS::state(CLOCK_CHANNEL_6) & 1u);
                     }
                     else {
                       _binary |= (TU::OUTPUTS::value(CLOCK_CHANNEL_1) & 1u) << 4;
                       _binary |= (TU::OUTPUTS::value(CLOCK_CHANNEL_2) & 1u) << 3;
                       _binary |= (TU::OUTPUTS::value(CLOCK_CHANNEL_3) & 1u) << 2;
                       _binary |= (TU::OUTPUTS::value(CLOCK_CHANNEL_5) & 1u) << 1;
                       _binary |= (TU::OUTPUTS::value(CLOCK_CHANNEL_6) & 1u);
                     }
                     ++_binary; // 32 max
                     
                     int16_t _dac_code = (static_cast<int16_t>(_binary) << 7) - 0x800; // +/- 2048
                     _dac_code = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _dac_code);
                     _dac_code = signed_saturate_rshift(_dac_code, 16, 0);
                     
                     _out = _ZERO - _dac_code;
                   }
                   break;
                  case _RANDOM: {
  
                     uint16_t _history[TU::OUTPUTS::kHistoryDepth];
                     int16_t _rand_history;
                     int16_t _rand_new;
  
                     TU::OUTPUTS::getHistory<CLOCK_CHANNEL_4>(_history);
                     
                     _rand_history = calc_average(_history, get_history_depth());     
                     _rand_history = signed_multiply_32x16b((static_cast<int32_t>(get_history_weight()) * 65535U) >> 8, _rand_history);
                     _rand_history = signed_saturate_rshift(_rand_history, 16, 0);
                     
                     _rand_new = random(0xFFF) - 0x800; // +/- 2048
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
                  case _LOGISTIC: {
                    
                     logistic_map_.set_r(get_logistic_map_r()); 
                     
                     int16_t _logistic_map_x = (static_cast<int16_t>(logistic_map_.Clock()) & 0xFFF) - 0x800; // +/- 2048
                     _logistic_map_x = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _logistic_map_x);
                     _logistic_map_x = signed_saturate_rshift(_logistic_map_x, 16, 0);
                     _out = _ZERO - _logistic_map_x;
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

  inline void logic(CLOCK_CHANNEL clock_channel) {

     if (!logic_)
       return;
     logic_ = false;  
        
     uint16_t _out = OFF;
     uint8_t _op1, _op2;
  
     _op1 = logic_op1();
     _op2 = logic_op2();

     // this doesn't care if CHANNEL_4 is in DAC mode (= mostly always true); but so what.
     if (!logic_tracking()) {
       _op1 = TU::OUTPUTS::state(_op1) & 1u;
       _op2 = TU::OUTPUTS::state(_op2) & 1u;
     }
     else {
       _op1 = TU::OUTPUTS::value(_op1) & 1u;
       _op2 = TU::OUTPUTS::value(_op2) & 1u;
     }

     switch (logic_type()) {
  
        case AND:  
            _out = _op1 & _op2;
            break;
        case OR:   
            _out = _op1 | _op2;
            break;
        case XOR:  
            _out = _op1 ^ _op2;
            break;
        case XNOR:  
            _out = ~(_op1 ^ _op2);
            break;
        case NAND: 
            _out = ~(_op1 & _op2);
            break;
        case NOR:  
            _out = ~(_op1 | _op2);
            break;
        default: 
            break;    
    } // end op switch

    // write to output
    gpio_state_ = _out = (_out & 1u) ? ON : OFF;
    if (!_out && clock_channel == CLOCK_CHANNEL_4)
       _out += _ZERO;
    TU::OUTPUTS::setState(clock_channel, _out); 
    TU::OUTPUTS::set(clock_channel, _out);  
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
    uint8_t mode = (channel_id != CLOCK_CHANNEL_4) ? get_mode() : get_mode4();

    if (channel_id != CLOCK_CHANNEL_4)
      *settings++ = CHANNEL_SETTING_MODE;
    else   
      *settings++ = CHANNEL_SETTING_MODE4;
          

    if (menu_page_ == CV_SOURCES) {

      if (mode != DAC)
        *settings++ = CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE;
      *settings++ = CHANNEL_SETTING_MULT_CV_SOURCE;
     
      switch (mode) {

          case LFSR:
            *settings++ = CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_TURING_PROB_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_LFSR_TAP1_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_LFSR_TAP2_CV_SOURCE;
            break;
          case EUCLID:
            *settings++ = CHANNEL_SETTING_EUCLID_N_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_EUCLID_K_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_EUCLID_OFFSET_CV_SOURCE;
            break; 
          case RANDOM:
            *settings++ = CHANNEL_SETTING_RAND_N_CV_SOURCE;
            break;
          case LOGIC:
            *settings++ = CHANNEL_SETTING_LOGIC_TYPE_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_LOGIC_OP1_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_LOGIC_OP2_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_DUMMY;
            break; 
          case SEQ:
            *settings++ = CHANNEL_SETTING_SEQ_CV_SOURCE;
            *settings++ = CHANNEL_SETTING_MASK_CV_SOURCE;
            break;
          case DAC: 
            *settings++ = CHANNEL_SETTING_DAC_MODE_CV_SOURCE; 
            *settings++ = CHANNEL_SETTING_DAC_RANGE_CV_SOURCE;
            switch (dac_mode())  {
              case _BINARY:
                *settings++ = CHANNEL_SETTING_DUMMY;
                break;
              case _RANDOM:
                *settings++ = CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE;
                *settings++ = CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE;
                break;
              case _TURING:
                *settings++ = CHANNEL_SETTING_TURING_PROB_CV_SOURCE;
                break;
              case _LOGISTIC:
                *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE;
                break;
              default:
                break;                
            }
          default:
            break;
      }
      *settings++ = CHANNEL_SETTING_CLOCK_CV_SOURCE;
      if (mode == MULT || mode == SEQ || mode == EUCLID) // make # items the same...
        *settings++ = CHANNEL_SETTING_DUMMY;
       
    }

    else if (menu_page_ == PARAMETERS) {

        if (mode != DAC)
          *settings++ = CHANNEL_SETTING_PULSEWIDTH;
        *settings++ = CHANNEL_SETTING_MULT;
    
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
           *settings++ = CHANNEL_SETTING_LOGIC_TRACK_WHAT;
           break; 
          case SEQ:
           *settings++ = CHANNEL_SETTING_CLOCK_PATTERN;
           *settings++ = CHANNEL_SETTING_CLOCK_MASK;
           break;
          case DAC: 
            *settings++ = CHANNEL_SETTING_DAC_MODE; 
            *settings++ = CHANNEL_SETTING_DAC_RANGE;
            switch (dac_mode())  {
    
              case _BINARY:
                *settings++ = CHANNEL_SETTING_DAC_TRACK_WHAT;
                break;
              case _RANDOM:
                *settings++ = CHANNEL_SETTING_HISTORY_WEIGHT;
                *settings++ = CHANNEL_SETTING_HISTORY_DEPTH;
                break;
              case _TURING:
                *settings++ = CHANNEL_SETTING_TURING_PROB;
                break;
              case _LOGISTIC:
                *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R;
                break;
              default:
                break;                
            }
           break;  
          default:
           break;
        } // end mode switch
    
        *settings++ = CHANNEL_SETTING_CLOCK;
        
        if (mode == MULT || mode == EUCLID || mode == SEQ)
          *settings++ = CHANNEL_SETTING_RESET;
    }
    num_enabled_settings_ = settings - enabled_settings_;  
  }
  

  bool update_pattern(bool force, int32_t mask_rotate) {
    const int pattern = get_pattern();
    uint16_t mask = get_mask();
    if (mask_rotate)
      mask = TU::PatternEditor<Clock_channel>::RotateMask(mask, TU::Patterns::GetPattern(pattern).num_slots, mask_rotate);

    if (force || (last_pattern_ != pattern || last_mask_ != mask)) {

      last_pattern_ = pattern;
      last_mask_ = mask;
      return true;
    } else {
      return false;
    }
  }

  void RenderScreensaver(weegfx::coord_t start_x, CLOCK_CHANNEL clock_channel) const;
  
private:
  bool force_update_;
  uint16_t _ZERO;
  uint8_t clk_src_;
  uint32_t ticks_;
  uint32_t subticks_;
  uint32_t tickjitter_;
  uint32_t clk_cnt_;
  int16_t div_cnt_;
  uint32_t ext_frequency_in_ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;
  uint16_t gpio_state_;
  uint8_t prev_multiplier_;
  uint8_t prev_pulsewidth_;
  uint8_t logic_;
  uint16_t last_pattern_;
  uint16_t last_mask_;
  uint8_t menu_page_;
 
  util::TuringShiftRegister turing_machine_;
  util::LogisticMap logistic_map_;
  int num_enabled_settings_;
  ChannelSetting enabled_settings_[CHANNEL_SETTING_LAST];
  TU::DigitalInputDisplay clock_display_;

};

const char* const channel_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "none"
};

const char* const reset_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "off"
};

const char* const multipliers[] = {
  "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
};

const char* const cv_sources[5] = {
  "None", "CV1", "CV2", "CV3", "CV4"
};
//const char* const clock_delays[] = {
//  "off", "60us", "120us", "180us", "240us", "300us", "360us", "420us", "480us"
//};

SETTINGS_DECLARE(Clock_channel, CHANNEL_SETTING_LAST) {
 
  { 0, 0, MODES - 2, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { 0, 0, MODES - 1, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR1, 0, CHANNEL_TRIGGER_LAST - 1, "clock src", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_TR2 + 1, 0, CHANNEL_TRIGGER_LAST - 1, "reset src", reset_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 7, 0, 14, "mult/div", multipliers, settings::STORAGE_TYPE_U8 },
  { 25, 1, 255, "pulsewidth", NULL, settings::STORAGE_TYPE_U8 },
  //{ 0, 0, 1, "invert", TU::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  //{ 0, 0, 8, "clock delay", clock_delays, settings::STORAGE_TYPE_U4 },
  //
  { 0, 0, 31, "LFSR tap1",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "LFSR tap2",NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, RND_MAX, "rand > n", NULL, settings::STORAGE_TYPE_U8 },
  { 3, 3, 31, "euclid: N", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 1, 31, "euclid: K", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "euclid: OFFSET", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 5, "logic type", TU::Strings::operators, settings::STORAGE_TYPE_U8 },
  { 0, 0, NUM_CHANNELS - 1, "op_1", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 1, 0, NUM_CHANNELS - 1, "op_2", TU::Strings::channel_id, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "track -->", TU::Strings::logic_tracking, settings::STORAGE_TYPE_U4 },
  { 128, 1, 255, "DAC: range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, DAC_MODES-1, "DAC: mode", TU::Strings::dac_modes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "track -->", TU::Strings::binary_tracking, settings::STORAGE_TYPE_U4 },
  { 0, 0, 255, "rnd hist.", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { 0, 0, TU::OUTPUTS::kHistoryDepth - 1, "hist. depth", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { 16, 1, 32, "LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "LFSR p(x)", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "logistic r", NULL, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { TU::Patterns::PATTERN_USER_0, 0, TU::Patterns::PATTERN_USER_LAST, "sequence #", TU::pattern_names_short, settings::STORAGE_TYPE_U8 },
  // cv sources
  { 0, 0, 4, "mult/div    <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "pulsewidth  <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "clock src   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR tap1   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR tap2   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "rand > n    <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "euclid: N   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "euclid: K   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "euclid: OFF <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "logic type  <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "op_1        <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "op_2        <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR p(x)   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR length <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LGST(R)     <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "sequence #  <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "mask        <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "DAC: range  <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "DAC: mode   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "rnd hist.   <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "hist. depth <<", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 0, "--------------------", NULL, settings::STORAGE_TYPE_U4 }
};


class ClocksState {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(CHANNEL_SETTING_MODE, CHANNEL_SETTING_LAST - 1);
    pattern_editor.Init();
  }

  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  int selected_channel;
  menu::ScreenCursor<menu::kScreenLines> cursor;
  TU::PatternEditor<Clock_channel> pattern_editor;
};

ClocksState clocks_state;
Clock_channel clock_channel[NUM_CHANNELS];

void CLOCKS_init() {

  ext_frequency[CHANNEL_TRIGGER_TR1]  = 0xFFFFFFFF;
  ext_frequency[CHANNEL_TRIGGER_TR2]  = 0xFFFFFFFF;
  ext_frequency[CHANNEL_TRIGGER_NONE] = 0xFFFFFFFF;

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
      clocks_state.pattern_editor.Close();
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

  ticks_src1++; // src #1 ticks
  ticks_src2++; // src #2 ticks
  
  uint32_t triggers = TU::DigitalInputs::clocked();  

  // clocked? reset ; better use ARM_DWT_CYCCNT ?
  if (triggers == 1)  {
    ext_frequency[CHANNEL_TRIGGER_TR1] = ticks_src1;
    ticks_src1 = 0x0;
  }
  if (triggers == 2) {
    ext_frequency[CHANNEL_TRIGGER_TR2] = ticks_src2;
    ticks_src2 = 0x0;
  }

  // update channels:
  clock_channel[0].Update(triggers, CLOCK_CHANNEL_1);
  clock_channel[1].Update(triggers, CLOCK_CHANNEL_2);
  clock_channel[2].Update(triggers, CLOCK_CHANNEL_3);
  clock_channel[4].Update(triggers, CLOCK_CHANNEL_5);
  clock_channel[5].Update(triggers, CLOCK_CHANNEL_6);
  clock_channel[3].Update(triggers, CLOCK_CHANNEL_4); // = DAC channel; update last, because of the binary Sequencer thing.

  // apply logic ?
  clock_channel[0].logic(CLOCK_CHANNEL_1);
  clock_channel[1].logic(CLOCK_CHANNEL_2);
  clock_channel[2].logic(CLOCK_CHANNEL_3);
  clock_channel[3].logic(CLOCK_CHANNEL_4);
  clock_channel[4].logic(CLOCK_CHANNEL_5);
  clock_channel[5].logic(CLOCK_CHANNEL_6);
}

void CLOCKS_handleButtonEvent(const UI::Event &event) {

  if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case TU::CONTROL_BUTTON_UP:
          // TEMPO
        break;
      case TU::CONTROL_BUTTON_DOWN:
        CLOCKS_downButtonLong();
        break;
       case TU::CONTROL_BUTTON_L:
        CLOCKS_leftButtonLong();
        break;  
      default:
        break;
     }
  }

  if (clocks_state.pattern_editor.active()) {
    clocks_state.pattern_editor.HandleButtonEvent(event);
    return;
  }
  
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case TU::CONTROL_BUTTON_UP:
        CLOCKS_upButton();
        break;
      case TU::CONTROL_BUTTON_DOWN:
        CLOCKS_downButton();
        break;
      case TU::CONTROL_BUTTON_L:
        CLOCKS_leftButton();
        break;
      case TU::CONTROL_BUTTON_R:
        CLOCKS_rightButton();
        break;
    }
  } 
}

void CLOCKS_handleEncoderEvent(const UI::Event &event) {

  if (clocks_state.pattern_editor.active()) {
    clocks_state.pattern_editor.HandleEncoderEvent(event);
    return;
  }
 
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
          
           if (CHANNEL_SETTING_CLOCK_MASK != setting) {
            if (selected.change_value(setting, event.value))
             selected.force_update();

            switch (setting) {
              case CHANNEL_SETTING_MODE:
              case CHANNEL_SETTING_MODE4:  
              case CHANNEL_SETTING_DAC_MODE:
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

void CLOCKS_upButton() {
  
  
  Clock_channel &selected = clock_channel[clocks_state.selected_channel];
  if (selected.get_page() == CV_SOURCES) {
    selected.set_page(PARAMETERS);
    selected.update_enabled_settings(clocks_state.selected_channel);
  }
  else {
    int selected_channel = clocks_state.selected_channel + 1;
    CONSTRAIN(selected_channel, 0, NUM_CHANNELS-1);
    clocks_state.selected_channel = selected_channel;
  }
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void CLOCKS_downButton() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];

  if (selected.get_page() == CV_SOURCES) {
    selected.set_page(PARAMETERS);
    selected.update_enabled_settings(clocks_state.selected_channel);
  }
  else {  
    int selected_channel = clocks_state.selected_channel - 1;
    CONSTRAIN(selected_channel, 0, NUM_CHANNELS-1);
    clocks_state.selected_channel = selected_channel;
  }
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void CLOCKS_rightButton() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];
  switch (selected.enabled_setting_at(clocks_state.cursor_pos())) {

    case CHANNEL_SETTING_CLOCK_MASK: {
      int pattern = selected.get_pattern();
      if (TU::Patterns::PATTERN_NONE != pattern) {
        clocks_state.pattern_editor.Edit(&selected, pattern);
      }
    }
      break;
    default:
      clocks_state.cursor.toggle_editing();
      break;
  }

}

void CLOCKS_leftButton() {
 
  // ?
}

void CLOCKS_leftButtonLong() {

  // sync
}

void CLOCKS_downButtonLong() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];
  clock_channel[clocks_state.selected_channel].page();
  selected.update_enabled_settings(clocks_state.selected_channel);
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1); 
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
      
      case CHANNEL_SETTING_CLOCK_MASK:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_mask(), TU::Patterns::GetPattern(channel.get_pattern()).num_slots);
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_DUMMY:  
        list_item.DrawNoValue<false>(value, attr);
        break;
      default:
        list_item.DrawDefault(value, attr);
    }
  }

  if (clocks_state.pattern_editor.active())
    clocks_state.pattern_editor.Draw();   
}

void Clock_channel::RenderScreensaver(weegfx::coord_t start_x, CLOCK_CHANNEL clock_channel) const {

  uint16_t _square, _frame, _mode = get_mode();

  _square = TU::OUTPUTS::state(clock_channel);
  _frame  = TU::OUTPUTS::value(clock_channel);

  // DAC needs special treatment:
  if (clock_channel == CLOCK_CHANNEL_4 && _mode < DAC) {
    _square = _square == ON ? 0x1 : 0x0;
    _frame  = _frame  == ON ? 0x1 : 0x0;
  }
  // draw little square thingies ..     
  if (_square && _frame) {
    graphics.drawRect(start_x, 36, 10, 10);
    graphics.drawFrame(start_x-2, 34, 14, 14);
  }
  else if (_square)
    graphics.drawRect(start_x, 36, 10, 10);
  else
   graphics.drawRect(start_x+3, 39, 4, 4);
}

void CLOCKS_screensaver() {

  clock_channel[0].RenderScreensaver(4,  CLOCK_CHANNEL_1);
  clock_channel[1].RenderScreensaver(26, CLOCK_CHANNEL_2);
  clock_channel[2].RenderScreensaver(48, CLOCK_CHANNEL_3);
  clock_channel[3].RenderScreensaver(70, CLOCK_CHANNEL_4);
  clock_channel[4].RenderScreensaver(92, CLOCK_CHANNEL_5);
  clock_channel[5].RenderScreensaver(114,CLOCK_CHANNEL_6);
}
