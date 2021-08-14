// Copyright (c) 2015, 2016 Max Stadler, Patrick Dowling
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
#include "util/util_arp.h"
#include "util/util_bursts.h"
#include "util/util_phase.h"
#include "util/util_trigger_delay.h"
#include "TU_input_map.h"
#include "TU_input_maps.h"
#include <algorithm>

namespace menu = TU::menu;

static const uint8_t RND_MAX = 31;          // max random (n)
static const uint8_t MULT_MAX = 26;         // max multiplier
static const uint8_t MULT_BY_ONE = 13;      // default multiplication
static const uint8_t PULSEW_MAX = 255;      // max pulse width [ms]
static const uint8_t PHASEOFFSET_MAX = 99;  // max phase offset
static const uint8_t BPM_MIN = 1;           // changes need changes in TU_BPM.h
static const int16_t BPM_MAX = 320;         // ditto
static const uint8_t LFSR_MAX = 31;         // max LFSR length
static const uint8_t LFSR_MIN = 4;          // min "
static const uint8_t EUCLID_N_MAX = 32;
static const uint16_t TOGGLE_THRESHOLD = 500; // ADC threshold for 0/1 parameters (500 = ~1.2V)

static const uint32_t SCALE_PULSEWIDTH = 58982; // 0.9 for signed_multiply_32x16b
static const uint32_t TICKS_TO_MS = 43691; // 0.6667f : fraction, if TU_CORE_TIMER_RATE = 60 us : 65536U * ((1000 / TU_CORE_TIMER_RATE) - 16)
static const uint32_t TICK_JITTER = 0xFFFFFFF;  // 1/16 : threshold/double triggers reject -> ext_frequency_in_ticks_
static const uint32_t TICK_SCALE  = 0xC0000000; // 0.75 for signed_multiply_32x32
static const uint32_t COPYTIMEOUT = 200000;

extern const uint32_t BPM_microseconds_4th[];

static uint32_t ticks_src1 = 0xFFFFFFF; // main clock frequency (top)
static uint32_t ticks_src2 = 0xFFFFFFF; // sec. clock frequency (bottom)
static uint32_t ticks_internal = 0; // sec. clock frequency (bottom)
static int32_t global_div_count_TR1 = 0; // pre-clock-division
static bool RESET_GLOBAL_TR2 = true;

// copy sequence, global
uint16_t copy_length = TU::Patterns::kMax;
uint16_t copy_mask = 0xFFFF;
uint32_t copy_timeout = COPYTIMEOUT;

static const uint64_t multipliers_[] = {

  0xFFFFFFFF, // x1
  0x80000000, // x2
  0x55555555, // x3
  0x40000000, // x4
  0x33333333, // x5
  0x2AAAAAAB, // x6
  0x24924925, // x7
  0x20000000, // x8
  0x15555555, // x12
  0x10000000, // x16
  0xAAAAAAA,  // x24
  0x8000000,  // x32
  0x5555555,  // x48
  0x4000000,  // x64

}; // = 2^32 / multiplier

static const uint64_t pw_scale_[] = {

  0xFFFFFFFF, // /64
  0xC0000000, // /48
  0x80000000, // /32
  0x60000000, // /24
  0x40000000, // /16
  0x30000000, // /12
  0x20000000, // /8
  0x1C000000, // /7
  0x18000000, // /6
  0x14000000, // /5
  0x10000000, // /4
  0xC000000,  // /3
  0x8000000,  // /2
  0x4000000,  // x1

}; // = 2^32 * divisor / 64

static const uint8_t divisors_[] = {

  64,
  48,
  32,
  24,
  16,
  12,
  8,
  7,
  6,
  5,
  4,
  3,
  2,
  1
};

static const uint8_t multipliers_values_[] = {

  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  12,
  16,
  24,
  32,
  48,
  64
};

enum ChannelSetting {
  // shared
  CHANNEL_SETTING_MODE,
  CHANNEL_SETTING_CLOCK,
  CHANNEL_SETTING_TRIGGER_DELAY,
  CHANNEL_SETTING_RESET,
  CHANNEL_SETTING_MULT,
  CHANNEL_SETTING_PULSEWIDTH,
  CHANNEL_SETTING_SWING,
  CHANNEL_SETTING_INTERNAL_CLK,
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
  CHANNEL_SETTING_DAC_OFFSET,
  CHANNEL_SETTING_DAC_MODE,
  CHANNEL_SETTING_DAC_TRACK_WHAT,
  CHANNEL_SETTING_HISTORY_WEIGHT,
  CHANNEL_SETTING_HISTORY_DEPTH,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
  CHANNEL_SETTING_LOGISTIC_MAP_R,
  CHANNEL_SETTING_ARP_RANGE,
  CHANNEL_SETTING_ARP_DIRECTION,
  CHANNEL_SETTING_MASK1,
  CHANNEL_SETTING_MASK2,
  CHANNEL_SETTING_MASK3,
  CHANNEL_SETTING_MASK4,
  CHANNEL_SETTING_MASK_CV,
  CHANNEL_SETTING_SEQUENCE,
  CHANNEL_SETTING_SEQUENCE_LEN1,
  CHANNEL_SETTING_SEQUENCE_LEN2,
  CHANNEL_SETTING_SEQUENCE_LEN3,
  CHANNEL_SETTING_SEQUENCE_LEN4,
  CHANNEL_SETTING_SEQUENCE_PLAYMODE,
  CHANNEL_SETTING_CV_SEQUENCE_LENGTH,
  CHANNEL_SETTING_CV_SEQUENCE_PLAYMODE,
  CHANNEL_SETTING_BURST_DENSITY,
  CHANNEL_SETTING_BURST_INITIAL,
  CHANNEL_SETTING_BURST_DAMP,
  CHANNEL_SETTING_BURST_TRIG_SOURCE_INT,
  // cv sources
  CHANNEL_SETTING_MULT_CV_SOURCE,
  CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE,
  CHANNEL_SETTING_SWING_CV_SOURCE,
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
  CHANNEL_SETTING_SEQ_MASK_CV_SOURCE,
  CHANNEL_SETTING_DAC_RANGE_CV_SOURCE,
  CHANNEL_SETTING_DAC_MODE_CV_SOURCE,
  CHANNEL_SETTING_DAC_OFFSET_CV_SOURCE,
  CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE,
  CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE,
  CHANNEL_SETTING_ARP_RANGE_CV_SOURCE,
  CHANNEL_SETTING_ARP_DIRECTION_CV_SOURCE,
  CHANNEL_SETTING_BURST_DENSITY_CV_SOURCE,
  CHANNEL_SETTING_BURST_INITIAL_CV_SOURCE,
  CHANNEL_SETTING_BURST_DAMP_CV_SOURCE,
  CHANNEL_SETTING_SEPARATOR,
  CHANNEL_SETTING_DUMMY_EMPTY,
  CHANNEL_SETTING_SCREENSAVER,
  CHANNEL_SETTING_LAST
};

enum ChannelTriggerSource {
  CHANNEL_TRIGGER_TR1,
  CHANNEL_TRIGGER_TR2,
  CHANNEL_TRIGGER_NONE,
  CHANNEL_TRIGGER_INTERNAL,
  CHANNEL_TRIGGER_LAST,
  CHANNEL_TRIGGER_FREEZE_HIGH = CHANNEL_TRIGGER_INTERNAL,
  CHANNEL_TRIGGER_FREEZE_LOW = CHANNEL_TRIGGER_LAST
};

enum ChannelCV_Mapping {
  CHANNEL_CV_MAPPING_CV1,
  CHANNEL_CV_MAPPING_CV2,
  CHANNEL_CV_MAPPING_CV3,
  CHANNEL_CV_MAPPING_CV4,
  CHANNEL_CV_MAPPING_LAST
};

enum class CLOCKMODE : int {
  MULT,
  LFSR,
  RANDOM,
  EUCLID,
  LOGIC,
  SEQ,
  BURST,
  DAC,
  LAST
};

enum class DACMODE : int {
  BINARY,
  RANDOM,
  TURING,
  LOGISTIC,
  SEQ_CV,
  LAST
};

enum CV_SEQ_MODES {
  _FWD,
  _REV,
  _PND1,
  _PND2,
  _RND,
  _ARP,
  CV_SEQ_MODE_LAST
};

enum EDIT_MODES {
  TRIGGERS,
  PITCH
};

enum CLOCKSTATES {
  OFF,
  ON = 4095
};

enum DISPLAYSTATES {
  _OFF,
  _ONBEAT,
  _ACTIVE
};

enum LOGICMODES {
  AND,
  OR,
  XOR,
  XNOR,
  NAND,
  NOR,
  LOGICMODE_LAST
};

enum PLAY_MODES {
  PM_NONE,
  PM_SEQ1,
  PM_SEQ2,
  PM_SEQ3,
  PM_TR1,
  PM_TR2,
  PM_TR3,
  PM_SH1,
  PM_SH2,
  PM_SH3,
  PM_SH4,
  PM_LAST
};

enum MENUPAGES {
  PARAMETERS,
  CV_SOURCES,
  TEMPO
};

uint64_t ext_frequency[CHANNEL_TRIGGER_LAST];
uint64_t int_frequency;
uint64_t pending_int_frequency;

class Clock_channel : public settings::SettingsBase<Clock_channel, CHANNEL_SETTING_LAST> {
public:

  CLOCKMODE get_mode() const {
    return static_cast<CLOCKMODE>(values_[CHANNEL_SETTING_MODE]);
  }

  uint8_t get_clock_source() const {
    return values_[CHANNEL_SETTING_CLOCK];
  }

  uint8_t get_burst_source() const {
    return values_[CHANNEL_SETTING_BURST_TRIG_SOURCE_INT];
  }

  void set_clock_source(uint8_t _src) {
    apply_value(CHANNEL_SETTING_CLOCK, _src);
  }

  int8_t get_multiplier() const {
    return values_[CHANNEL_SETTING_MULT];
  }

  int8_t get_effective_multiplier() {
    return prev_multiplier_;
  }

  uint16_t get_pulsewidth() const {
    return values_[CHANNEL_SETTING_PULSEWIDTH];
  }

  uint16_t get_phase() const {
    return values_[CHANNEL_SETTING_SWING];
  }

  uint16_t get_internal_timer() const {
    return values_[CHANNEL_SETTING_INTERNAL_CLK];
  }

  uint8_t get_reset_source() const {
    return values_[CHANNEL_SETTING_RESET];
  }

  void set_reset_source(uint8_t src) {
    apply_value(CHANNEL_SETTING_RESET, src);
  }

  uint8_t get_tap1() const {
    return values_[CHANNEL_SETTING_LFSR_TAP1];
  }

  void set_tap1(uint8_t _t) {
    apply_value(CHANNEL_SETTING_LFSR_TAP1, _t);
  }

  uint8_t get_tap2() const {
    return values_[CHANNEL_SETTING_LFSR_TAP2];
  }

  void set_tap2(uint8_t _t) {
    apply_value(CHANNEL_SETTING_LFSR_TAP2, _t);
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

  void set_euclid_k(uint8_t k) {
    apply_value(CHANNEL_SETTING_EUCLID_K, k);
  }

  uint8_t euclid_offset() const {
    return values_[CHANNEL_SETTING_EUCLID_OFFSET];
  }

  void set_euclid_offset(uint8_t o) {
    apply_value(CHANNEL_SETTING_EUCLID_OFFSET, o);
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

  int8_t dac_offset() const {
    return values_[CHANNEL_SETTING_DAC_OFFSET];
  }

  DACMODE dac_mode() const {
    return static_cast<DACMODE>(values_[CHANNEL_SETTING_DAC_MODE]);
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

  int get_sequence() const {
    return values_[CHANNEL_SETTING_SEQUENCE];
  }

  int get_playmode() const {
    return values_[CHANNEL_SETTING_SEQUENCE_PLAYMODE];
  }

  int get_cv_playmode() const {
    return values_[CHANNEL_SETTING_CV_SEQUENCE_PLAYMODE];
  }

  int burst_density() const {
    return values_[CHANNEL_SETTING_BURST_DENSITY];
  }

  int burst_initial() const {
    return values_[CHANNEL_SETTING_BURST_INITIAL];
  }

  int burst_damp() const {
    return values_[CHANNEL_SETTING_BURST_DAMP];
  }

  int get_display_sequence() const {
    return display_sequence_;
  }

  void set_display_sequence(uint8_t seq) {
    display_sequence_ = seq;
  }

  int get_display_mask() const {
    return display_mask_;
  }

  int get_cv_display_mask() const {
    return cv_display_mask_;
  }

  int get_display_clock() const {
    return display_state_;
  }

  void set_sequence(uint8_t seq) {
    apply_value(CHANNEL_SETTING_SEQUENCE, seq);
  }

  uint8_t get_sequence_length(uint8_t _seq) const {

    switch (_seq) {

      case 1:
    return values_[CHANNEL_SETTING_SEQUENCE_LEN2];
    break;
      case 2:
    return values_[CHANNEL_SETTING_SEQUENCE_LEN3];
    break;
      case 3:
    return values_[CHANNEL_SETTING_SEQUENCE_LEN4];
    break;
      default:
    return values_[CHANNEL_SETTING_SEQUENCE_LEN1];
    break;
    }
  }

  uint8_t get_cv_sequence_length() const {
     return values_[CHANNEL_SETTING_CV_SEQUENCE_LENGTH];
  }

  uint8_t get_mult_cv_source() const {
    return values_[CHANNEL_SETTING_MULT_CV_SOURCE];
  }

  uint8_t get_pulsewidth_cv_source() const {
    return values_[CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE];
  }

  uint8_t get_phase_cv_source() const {
    return values_[CHANNEL_SETTING_SWING_CV_SOURCE];
  }

  uint8_t get_clock_source_cv_source() const {
    return values_[CHANNEL_SETTING_CLOCK_CV_SOURCE];
  }

  uint16_t get_trigger_delay() const {
    return values_[CHANNEL_SETTING_TRIGGER_DELAY];
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

  uint8_t get_arp_mask_cv_source() const {
      return values_[CHANNEL_SETTING_SEQ_MASK_CV_SOURCE];
  }

  uint8_t get_sequence_cv_source() const {
    return values_[CHANNEL_SETTING_SEQ_CV_SOURCE];
  }

  uint8_t get_DAC_mode_cv_source() const {
    return values_[CHANNEL_SETTING_DAC_MODE_CV_SOURCE];
  }

  uint8_t get_DAC_range_cv_source() const {
    return values_[CHANNEL_SETTING_DAC_RANGE_CV_SOURCE];
  }

  uint8_t get_DAC_offset_cv_source() const {
    return values_[CHANNEL_SETTING_DAC_OFFSET_CV_SOURCE];
  }

  uint8_t get_rand_history_w_cv_source() const {
    return values_[CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE];
  }

  uint8_t get_rand_history_d_cv_source() const {
    return values_[CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE];
  }

  uint8_t get_arp_range() const {
    return values_[CHANNEL_SETTING_ARP_RANGE];
  }

  uint8_t get_arp_range_cv_source() const {
    return values_[CHANNEL_SETTING_ARP_RANGE_CV_SOURCE];
  }

  uint8_t get_arp_direction() const {
    return values_[CHANNEL_SETTING_ARP_DIRECTION];
  }

  uint8_t get_arp_direction_cv_source() const {
    return values_[CHANNEL_SETTING_ARP_DIRECTION_CV_SOURCE];
  }

  uint8_t density_cv_source() const {
    return values_[CHANNEL_SETTING_BURST_DENSITY_CV_SOURCE];
  }

  uint8_t burst_initial_cv_source() const {
    return values_[CHANNEL_SETTING_BURST_INITIAL_CV_SOURCE];
  }

  uint8_t burst_damp_cv_source() const {
    return values_[CHANNEL_SETTING_BURST_DAMP_CV_SOURCE];
  }

  void update_pattern_mask(uint16_t mask, uint8_t sequence) {

    switch(sequence) {

      case 1:
        apply_value(CHANNEL_SETTING_MASK2, mask);
        break;
      case 2:
        apply_value(CHANNEL_SETTING_MASK3, mask);
        break;
      case 3:
        apply_value(CHANNEL_SETTING_MASK4, mask);
        break;
      default:
        apply_value(CHANNEL_SETTING_MASK1, mask);
        break;
    }
  }

  int get_mask(uint8_t _this_sequence) const {

    switch(_this_sequence) {

      case 1:
        return values_[CHANNEL_SETTING_MASK2];
        break;
      case 2:
        return values_[CHANNEL_SETTING_MASK3];
        break;
      case 3:
        return values_[CHANNEL_SETTING_MASK4];
        break;
      default:
        return values_[CHANNEL_SETTING_MASK1];
        break;
    }
  }

  void set_sequence_length(uint8_t len, uint8_t seq) {

    switch(seq) {
      case 0:
        apply_value(CHANNEL_SETTING_SEQUENCE_LEN1, len);
        break;
      case 1:
        apply_value(CHANNEL_SETTING_SEQUENCE_LEN2, len);
        break;
      case 2:
        apply_value(CHANNEL_SETTING_SEQUENCE_LEN3, len);
        break;
      case 3:
        apply_value(CHANNEL_SETTING_SEQUENCE_LEN4, len);
        break;
      default:
        break;
    }
  }

  int get_cv_mask() const {
    return values_[CHANNEL_SETTING_MASK_CV];
  }

  void set_cv_sequence_length(uint8_t len) {

    apply_value(CHANNEL_SETTING_CV_SEQUENCE_LENGTH, len);
    if (get_cv_playmode() == _ARP)
      arpeggiator_.UpdateArpeggiator(0x0, get_cv_mask(), get_cv_sequence_length());
  }

  void update_cv_pattern_mask(uint16_t mask) {
    apply_value(CHANNEL_SETTING_MASK_CV, mask);
  }

  int get_pitch_at_step(uint8_t step) {
    // only 1st sequence for now
    TU::Pattern *read_pattern_ = &TU::user_patterns[0x0];
    return read_pattern_->notes[step];
  }

  void set_pitch_at_step(uint8_t step, int16_t pitch) {
    // only 1st sequence for now:
    TU::Pattern *write_pattern_ = &TU::user_patterns[0x0];
    write_pattern_->notes[step] = pitch;

    if (get_cv_playmode() == _ARP) // to do
      cv_pattern_changed(get_cv_mask(), true);
  }

  int16_t dac_correction() const {
    return dac_overflow_;
  }

  uint16_t get_clock_cnt() const {
    return clk_cnt_;
  }

  int16_t get_div_cnt() const {
    return div_cnt_;
  }

  uint16_t get_current_sequence() const {
    return sequence_last_;
  }

  void update_internal_timer(uint16_t _tempo) {
    apply_value(CHANNEL_SETTING_INTERNAL_CLK, _tempo);
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

  void pattern_changed(uint16_t mask, bool force) {
    force_update_ = force;
    if (force)
      display_mask_ = mask;
  }

  void init_pattern(uint16_t mask) {
    force_update_ = true;
    display_mask_ = mask;
    arpeggiator_.UpdateArpeggiator(0x0, get_cv_mask(), get_cv_sequence_length());
  }

  uint8_t get_gpio_state() const {
    return gpio_state_;
  }

  void cv_pattern_changed(uint16_t mask, bool force) {

    int16_t mask_rotate_ = get_arp_mask_cv_source();

    if (!mask_rotate_) {
      //
      cv_display_mask_ = mask;
      arpeggiator_.UpdateArpeggiator(0x0, get_cv_mask(), get_cv_sequence_length());
    }
    else {
      // rotate already
      mask_rotate_ = (TU::ADC::value(static_cast<ADC_CHANNEL>(mask_rotate_ - 1)) + 127) >> 8;

      if (force || (prev_mask_rotate_ != mask_rotate_) || (prev_cv_playmode_ != get_cv_playmode()) ) {
        cv_display_mask_ = TU::PatternEditor<Clock_channel>::RotateMask(get_cv_mask(), get_cv_sequence_length(), mask_rotate_);
        arpeggiator_.UpdateArpeggiator(0x0, cv_display_mask_, get_cv_sequence_length());
      }
      prev_mask_rotate_ = mask_rotate_;
      prev_cv_playmode_ = get_cv_playmode();
    }
  }

  void reset_sequence() {
    sequence_reset_ = true;
  }

  void copy_seq(uint8_t len, uint16_t mask) {
    copy_length = len;
    copy_mask = mask;
    copy_timeout = 0;
  }

  uint8_t paste_seq(uint8_t seq) {

    if (copy_timeout < COPYTIMEOUT) {

      // copy length:
      set_sequence_length(copy_length, seq);
      // copy mask:
      update_pattern_mask(copy_mask, seq);
      // give more time for more pasting...
      copy_timeout = 0;
      return copy_length;
    }
    else
      return 0;
  }

  void update_inputmap(int num_slots, uint8_t range) {
    input_map_.Configure(TU::InputMaps::GetInputMap(num_slots), range);
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
    apply_value(CHANNEL_SETTING_ARP_RANGE_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_ARP_DIRECTION_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_SEQ_MASK_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_BURST_DENSITY_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_BURST_INITIAL_CV_SOURCE, 0);
    apply_value(CHANNEL_SETTING_BURST_DAMP_CV_SOURCE, 0);
  }

  void sync() {
    skip_reset_ = (div_cnt_ > 0x1) ? true : false;
    pending_sync_ = true;
  }

  void resync(uint32_t clk_cnt, uint32_t div_cnt ) {
    // don't resync channels running off TR2
    if (clk_src_ != CHANNEL_TRIGGER_TR2) {
      clk_cnt_ = clk_cnt;
      div_cnt_ = div_cnt;
      sync_ = false;
    }
  }

  bool slave() {
    return sync_;
  }

  void reset_channel_frequency() {
    channel_frequency_in_ticks_ = 0xFFFFFFFF;
  }

  uint8_t get_page() const {
    return menu_page_;
  }

  void set_page(uint8_t _page) {
    menu_page_ = _page;
  }

  uint16_t get_zero(uint8_t channel) const {

    uint16_t _off = 0;
    if (channel == CLOCK_CHANNEL_4)
      _off += _ZERO;

    return _off;
  }

  void update_burst_parameters(uint32_t frequency_in_ticks_) {

    int32_t _density, _initial, _damp;

    _density = burst_density();

    if (density_cv_source()) {
      _density += (TU::ADC::value(static_cast<ADC_CHANNEL>(density_cv_source() - 1)) + 32) >> 6;
      CONSTRAIN(_density, 0, 31);
    }

    _initial = burst_initial();

    if (burst_initial_cv_source()) {
      _initial += (TU::ADC::value(static_cast<ADC_CHANNEL>(burst_initial_cv_source() - 1)) + 32) >> 6;
      CONSTRAIN(_initial, 0, 40);
    }

    _damp = burst_damp();

    if (burst_damp_cv_source()) {
      _damp += (TU::ADC::value(static_cast<ADC_CHANNEL>(burst_damp_cv_source() - 1)) + 32) >> 6;
      CONSTRAIN(_damp, 0, 60);
    }

    bursts_.set_frequency(frequency_in_ticks_);
    bursts_.set_density(_density);
    bursts_.set_initial(_initial);
    bursts_.set_damping(_damp);
  }

  void Init(CLOCK_CHANNEL channel, ChannelTriggerSource trigger_source) {

    InitDefaults();
    trigger_delay_.Init();
    apply_value(CHANNEL_SETTING_CLOCK, trigger_source);

    channel_ = channel;
    force_update_ = true;
    sync_ = false;
    skip_reset_ = false;
    phase_ = false;
    prev_phase_ = 0;
    gpio_state_ = OFF;
    display_state_ = _OFF;
    ticks_ = 0;
    subticks_ = 0;
    phase_ticks_ = 0;
    tickjitter_ = 10000;
    clk_cnt_ = 0;
    mult_cnt_ = 0;
    div_cnt_ = 0;
    clk_src_ = get_clock_source();
    seq_direction_ = true;
    pending_reset_ = 0;
    logic_ = false;
    prev_mask_rotate_ = 0xFF;
    prev_cv_playmode_ = get_cv_playmode();
    menu_page_ = PARAMETERS;
    dac_overflow_ = 0xFFF;

    pending_multiplier_ = prev_multiplier_ = get_multiplier();
    pending_sync_ = false;
    prev_pulsewidth_ = get_pulsewidth();
    bpm_last_ = 0;

    pending_new_burst_ = 0x0;
    wait_burst_ = 0x0;
    burst_complete_ = 0x0;

    ext_frequency_in_ticks_ = 0xFFFFFFFF;
    channel_frequency_in_ticks_ = 0xFFFFFFFF;
    pulse_width_in_ticks_ = get_pulsewidth() << 10;

    _ZERO = TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero];

    display_sequence_ = get_sequence();
    display_mask_ = get_mask(display_sequence_);
    cv_display_mask_ = get_cv_mask();
    sequence_last_ = display_sequence_;
    sequence_last_length_ = 0x0;
    sequence_advance_ = false;
    sequence_advance_state_ = false;
    last_trigger_state_ = false;

    turing_machine_.Init();
    input_map_.Init();
    arpeggiator_.Init(TU::OUTPUTS::get_v_oct());
    turing_machine_.Clock();
    logistic_map_.Init();
    bursts_.Init();

    uint32_t _seed = TU::ADC::value<ADC_CHANNEL_1>() + TU::ADC::value<ADC_CHANNEL_2>() + TU::ADC::value<ADC_CHANNEL_3>() + TU::ADC::value<ADC_CHANNEL_4>();
    randomSeed(_seed);
    logistic_map_.set_seed(_seed);
    turing_machine_.set_shift_register(_seed);
    clock_display_.Init();
    Phase_.Init();
    update_enabled_settings();
  }

  void force_update() {
    force_update_ = true;
  }

  /* main channel update below: */

  inline void Update(uint32_t triggers, uint8_t mute) {

    int16_t _clock_source, _reset_source, _phase;
    int8_t _multiplier;
    bool _none, _triggered, _tock, _sync;
    uint16_t _output = gpio_state_;
    uint32_t prev_channel_frequency_in_ticks_ = 0x0;
    
    CLOCKMODE _mode = get_mode();

    // core channel parameters --
    // 1. clock source:
    _clock_source = get_clock_source();

    if (get_clock_source_cv_source()){
      int16_t _toggle = TU::ADC::value(static_cast<ADC_CHANNEL>(get_clock_source_cv_source() - 1));

      if (_toggle > TOGGLE_THRESHOLD && _clock_source <= CHANNEL_TRIGGER_TR2)
        _clock_source = (~_clock_source) & 1u;
    }
    // 2. multiplication:
    _multiplier = get_multiplier();

    if ((_mode != CLOCKMODE::BURST) && (_multiplier > MULT_BY_ONE) && (subticks_ > (channel_frequency_in_ticks_ << 2)))
      reset_channel_frequency();

    if (get_mult_cv_source()) {
      _multiplier += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_mult_cv_source() - 1)) + 63) >> 7;
      CONSTRAIN(_multiplier, 0, MULT_MAX);
    }
 
    // clocked ?
    _none = CHANNEL_TRIGGER_NONE == _clock_source;
    _triggered = !_none && (triggers & (0x1 << _clock_source));
    _tock = false;
    _sync = false;

    // 4. swing?
    if (_mode != CLOCKMODE::BURST && !pending_sync_) {

      _phase = get_phase();

      if (prev_phase_ != _phase)
        sync_ = true;
      if (!_phase && prev_phase_) Phase_.clear_phase_offset();
      prev_phase_ = _phase;

      if (get_phase_cv_source()) {
        _phase += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_phase_cv_source() - 1)) + 7) >> 4;
        CONSTRAIN(_phase, 0, PHASEOFFSET_MAX);
      }
    }
    else {
      // pending sync: set _phase to zero and skip just to make sure nothing funny happens
      if (prev_phase_)
        skip_reset_ = true;
      _phase = 0x0;
    }

    // 5. increase latency?
    trigger_delay_.Update();

    if (_triggered)
        trigger_delay_.Push(TU::trigger_delay_ticks[get_trigger_delay()]);
    _triggered = trigger_delay_.triggered();

    // new tick frequency, external or internal:
    if (!_none && _clock_source <= CHANNEL_TRIGGER_INTERNAL) {

      if (_triggered || clk_src_ != _clock_source) {
        ext_frequency_in_ticks_ = ext_frequency[_clock_source];
        _tock = true;
        div_cnt_--;
      }
    }

    // store clock source:
    clk_src_ = _clock_source;

    // new multiplier ?
    if (prev_multiplier_ != _multiplier) {
      pending_multiplier_ = _multiplier; // we need to wait for a new trigger to execute this
      sync_ = true;
    }

    if (_triggered && pending_multiplier_ != prev_multiplier_) {
      _tock |= true;
      prev_multiplier_ = pending_multiplier_ = _multiplier;
    }

    // if so, recalculate channel frequency and corresponding jitter-thresholds:
    if (_tock) {

      // when multiplying, skip too closely spaced triggers:
      if (_multiplier > MULT_BY_ONE) {
        prev_channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(channel_frequency_in_ticks_, TICK_SCALE);
        // new frequency:
        channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(ext_frequency_in_ticks_, multipliers_[_multiplier - MULT_BY_ONE]);
      }
      else {
        prev_channel_frequency_in_ticks_ = 0x0;
        // new frequency (used for pulsewidth):
        channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(ext_frequency_in_ticks_, pw_scale_[_multiplier]) << 6;
      }

      tickjitter_ = multiply_u32xu32_rshift32(channel_frequency_in_ticks_, TICK_JITTER);
    }
  
    // limit frequency to > 0
    if (!channel_frequency_in_ticks_)
      channel_frequency_in_ticks_ = 1u;

    // reset?
    _reset_source = get_reset_source();
    if (_reset_source < CHANNEL_TRIGGER_NONE && pending_reset_ <= 0) {

      uint8_t reset_state_ = !_reset_source ? digitalReadFast(TR1) : digitalReadFast(TR2);
      if (pending_reset_ < 0 && reset_state_) { // reset gate went low again, we can reset
        pending_reset_ = 0;
      }
      else if (!pending_reset_ && !reset_state_) { // reset gate went high
        pending_reset_ = 1;
      }
    }

    if (_triggered && pending_reset_ > 0) { // wait until the next trigger to do it
      // skip_reset_ probably unnecessary here
      // skip_reset_ = (div_cnt_ > 0) ? true : false; // "still und leise im Hintergrund" ?
      div_cnt_ = 0x0;
    }
    
    // special treatment for bursts, if interrupted by new trigger (see below)
    if (pending_new_burst_ && (burst_complete_++ > pulse_width_in_ticks_)) {
      _output = gpio_state_ = false; TU::OUTPUTS::set(channel_, _output);
      burst_complete_ = 0x0;
      pending_new_burst_ = false;
    }
    
    // phase adjust? 
    if (Phase_.update()) return;
    // increment channel ticks ..
    subticks_++;

    // burst mode ?
    if (_mode == CLOCKMODE::BURST) {
      
      bool _new_burst = false;
      uint8_t _trigger_state = 0x1;
      uint64_t ping_f = 0;
      
      switch (_clock_source) {

        case CHANNEL_TRIGGER_TR1:
        {
          _trigger_state = digitalReadFast(TR2);
          ping_f = ext_frequency[TR2];
        }
        break;
        case CHANNEL_TRIGGER_TR2:
        {
          _trigger_state = digitalReadFast(TR1);
          ping_f = ext_frequency[TR1];
        }
        break;
        case CHANNEL_TRIGGER_NONE:
          _trigger_state = last_trigger_state_;
        break;
        case CHANNEL_TRIGGER_INTERNAL:
        {
          _trigger_state = get_burst_source() == TR1 ? digitalReadFast(TR1) : digitalReadFast(TR2);
          ping_f = ext_frequency[get_burst_source()];
        }
        break;
        default:
        break;   
      }
      
      // if triggered, new burst ... 
      if (_trigger_state < last_trigger_state_) {
        _new_burst = bursts_.new_burst();
      }
      // store trigger state:
      last_trigger_state_ = _trigger_state;

      // update parameters
      update_burst_parameters(channel_frequency_in_ticks_);
      
      if (_new_burst || bursts_.process() || wait_burst_) {
        
        // reset / start counting:
        if (_new_burst) { 

          // it might be nicer to have an ongoing burst complete, but doesn't quite work with phase delay as is;
          // ... we let the ongoing pulse finish, at least. see above
          if (gpio_state_) { 
            pending_new_burst_ = true;
            burst_complete_ = subticks_;
          }
          bursts_.reset();
        }
        
        // set phase delay:
        _phase = get_phase();
        if (get_phase_cv_source()) {
          _phase += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_phase_cv_source() - 1)) + 7) >> 4;
          CONSTRAIN(_phase, 0, PHASEOFFSET_MAX);
        }
        
        if (Phase_.set_phase(uint64_t(ping_f >> 1), _phase, _new_burst)) {
          wait_burst_ = true; // ... stay in sync
          return;
        }
        // and clear once things fall through --> pw
        Phase_.clear_phase_offset();

        // update output
        _output = gpio_state_ = true;
        display_state_ = _ACTIVE;
        // reset
        subticks_ = 0x0;
        TU::OUTPUTS::setState(channel_, _output);
        wait_burst_ = false;
        bursts_.increment();
      }
      // stop?
      if (!bursts_.count() && (subticks_ > (bursts_.get_frequency() >> 1))) {
        gpio_state_ = false; display_state_ = _OFF;
      }
    }
    
    // all other modes:
    else  {

      // in sequencer mode, do we advance sequences by TR2?
      if (_mode == CLOCKMODE::SEQ && get_playmode() > 3) {
  
        uint8_t _advance_trig = digitalReadFast(TR2);
        // ?
        if (_advance_trig < sequence_advance_state_)
          sequence_advance_ = true;
  
        sequence_advance_state_ = _advance_trig;
      }
    
      /*
       *  brute force ugly sync hack:
       *  this, presumably, is needlessly complicated.
       *  but seems to work ok-ish, w/o too much jitter and missing clocks...
       */
  
      uint32_t _subticks = subticks_;
  
      // sync ? (manual)
      div_cnt_ = pending_sync_ ? 0x0 : div_cnt_;
  
      if (_multiplier <= MULT_BY_ONE && _triggered && div_cnt_ <= 0) {
        // division, so we track
        _sync = true;
        div_cnt_ = divisors_[_multiplier];
        subticks_ = channel_frequency_in_ticks_; // force sync
      }
      else if (_multiplier <= MULT_BY_ONE && _triggered) {
        // division, mute output:
        TU::OUTPUTS::setState(channel_, OFF);
        display_state_ = _OFF; // for display
      }
      else if (_multiplier > MULT_BY_ONE && _triggered)  {
        // multiplication, force sync, if clocked:
        _sync = true;
        subticks_ = channel_frequency_in_ticks_;
        mult_cnt_ = 0x0;
      }
      else if (_multiplier > MULT_BY_ONE && mult_cnt_ <= (multipliers_values_[_multiplier - MULT_BY_ONE]) && mute != clk_src_)
        _sync = true;
  
      // end of ugly hack
      
      // time to output ?
    
      if ((subticks_ >= channel_frequency_in_ticks_ && _sync) || Phase_.now()) {
  
        if (Phase_.set_phase(channel_frequency_in_ticks_, _phase, _triggered))
          return;
        // reset ticks:
        subticks_ = 0x0;
  
        //reject, if clock is too jittery or skip quasi-double triggers when ext. frequency increases:
        if (!get_trigger_delay()) {
          if ((_subticks < tickjitter_) || (_subticks < prev_channel_frequency_in_ticks_ && !pending_reset_))
            return;
        }
  
        // mute output ?
        if (_reset_source > CHANNEL_TRIGGER_NONE) {
  
          if (_reset_source == CHANNEL_TRIGGER_FREEZE_HIGH && !digitalReadFast(TR2))
            return;
          else if (_reset_source == CHANNEL_TRIGGER_FREEZE_LOW && digitalReadFast(TR2))
            return;
        }
 
        // only then count clocks:
        clk_cnt_++; mult_cnt_++;
  
        // reset counter ? (SEQ/Euclidian)
        // resync/clear pending sync
        if (_triggered && (pending_reset_ > 0 || pending_sync_)) {
          if (pending_reset_) {
            pending_reset_ = -1;
          }
          if (pending_sync_) {
            pending_sync_ = false;
          }
          clk_cnt_ = 0x0;
        }
  
        // clear for reset:
        // finally, process trigger + output
        if (!skip_reset_) {
          _output = gpio_state_ = process_clock_channel(_mode); // = either ON, OFF, or anything (DAC)
          display_state_ = _ACTIVE;
          if (_triggered) {
            TU::OUTPUTS::setState(channel_, _output);
          }
        }
        skip_reset_ = false;
      }
    }

    /*
     *  below: pulsewidth stuff
     */

    if (gpio_state_ && _mode != CLOCKMODE::DAC) {

      // pulsewidth setting --
      int16_t _pulsewidth = get_pulsewidth();

      if (_pulsewidth || _multiplier > MULT_BY_ONE || _clock_source == CHANNEL_TRIGGER_INTERNAL || _phase) {

        bool _gates = false;

        // do we echo && multiply? if so, do half-duty cycle:
        if (!_pulsewidth && _phase)
          _pulsewidth = 1;
        else if (!_pulsewidth)
          _pulsewidth = PULSEW_MAX;

        if (_pulsewidth == PULSEW_MAX)
          _gates = true;
        // CV?
        if (get_pulsewidth_cv_source()) {

          _pulsewidth += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_pulsewidth_cv_source() - 1)) + 8) >> 4;
          if (!_gates)
            CONSTRAIN(_pulsewidth, 1, PULSEW_MAX);
          else // CV for 50% duty cycle:
            CONSTRAIN(_pulsewidth, 1, (PULSEW_MAX<<1) - 55);  // incl margin, max < 2x mult. see below
        }
        // recalculate (in ticks), if new pulsewidth setting:
        if (prev_pulsewidth_ != _pulsewidth || !subticks_) {
          if (!_gates) {
            int32_t _fraction = signed_multiply_32x16b(TICKS_TO_MS, static_cast<int32_t>(_pulsewidth)); // = * 0.6667f
            _fraction = signed_saturate_rshift(_fraction, 16, 0);
            pulse_width_in_ticks_  = (_pulsewidth << 4) + _fraction;
          }
          else { // put out gates/half duty cycle:
            pulse_width_in_ticks_ = channel_frequency_in_ticks_ >> 1;

            if (_pulsewidth != PULSEW_MAX) { // CV?
              pulse_width_in_ticks_ = signed_multiply_32x16b(static_cast<int32_t>(_pulsewidth) << 8, pulse_width_in_ticks_); //
              pulse_width_in_ticks_ = signed_saturate_rshift(pulse_width_in_ticks_, 16, 0);
            }
          }
        }
        prev_pulsewidth_ = _pulsewidth;

        // limit pulsewidth, if approaching half duty cycle:
        if (!_gates && pulse_width_in_ticks_ >= (channel_frequency_in_ticks_ >> 1))
          pulse_width_in_ticks_ = (channel_frequency_in_ticks_ >> 1) | 1u;
        // limit pulsewidth / phase
        int32_t _phase_offset = Phase_.phase_offset();
        if (_phase_offset && (_phase_offset + pulse_width_in_ticks_ >= channel_frequency_in_ticks_))
          pulse_width_in_ticks_ = ((channel_frequency_in_ticks_ - _phase_offset) >> 1) | 1u;
        // burst mode needs special treatment, because pw might have to diminish ...   
        if (_mode == CLOCKMODE::BURST) {
          if (pulse_width_in_ticks_ >= bursts_.duty())
            pulse_width_in_ticks_ = bursts_.duty() | 1u;
        }
        // turn off output?
        if (subticks_ >= pulse_width_in_ticks_) {
          _output = gpio_state_ = OFF;
          display_state_ = _ONBEAT;
        }
        else // keep on
          _output = ON;
      }
      else {
        // we simply echo the pulsewidth:
        bool _state = (_clock_source == CHANNEL_TRIGGER_TR1) ? !digitalReadFast(TR1) : !digitalReadFast(TR2);

        if (_state)
          _output = ON;
        else
          _output = gpio_state_ = OFF;
      }
    }
    // DAC channel needs extra treatment / zero offset:
    if (is_dac_channel() && _mode != CLOCKMODE::DAC && gpio_state_ == OFF)
      _output += _ZERO;

    // update (physical) outputs:
    TU::OUTPUTS::set(channel_, _output);
  } // end update

  /* details re: trigger processing happens (mostly) here: */
  inline uint16_t process_clock_channel(CLOCKMODE mode) {

    int16_t _out = ON;
    logic_ = false;

    switch (mode) {
        
      case CLOCKMODE::MULT:
      case CLOCKMODE::BURST:
        break;
      case CLOCKMODE::LOGIC:
        // logic happens elsewhere.
        logic_ = true;
        break;
      case CLOCKMODE::LFSR: {
        // LFSR, sort of. mash-up of o_C and old TU firmware:
        int16_t _length, _probability, _myfirstbit;

        _length = get_turing_length();
        _probability = get_turing_probability();

        if (get_turing_length_cv_source()) {
          _length += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_turing_length_cv_source() - 1)) + 32) >> 6;
          CONSTRAIN(_length, LFSR_MIN, LFSR_MAX);
        }

        if (get_turing_prob_cv_source()) {
          _probability += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_turing_prob_cv_source() - 1)) + 8) >> 4;
          CONSTRAIN(_probability, 1, 255);
        }

        turing_machine_.set_length(_length);
        turing_machine_.set_probability(_probability);

        uint32_t _shift_register = turing_machine_.Clock();

        _myfirstbit =  _shift_register & 1u; // --> this is our output
        _out = _myfirstbit ? ON : OFF; // DAC needs special care ...

        // now update LFSR (even more):
        int8_t  _tap1 = get_tap1();
        int8_t  _tap2 = get_tap2();

        if (get_tap1_cv_source())
          _tap1 += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_tap1_cv_source() - 1)) + 64) >> 7;

        if (get_tap2_cv_source())
          _tap2 += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_tap2_cv_source() - 1)) + 64) >> 7;

        CONSTRAIN(_tap1, 1, _length);
        CONSTRAIN(_tap2, 1, _length);

        _tap1 = (_shift_register >> _tap1) & 1u;  // bit at tap1
        _tap2 = (_shift_register >> _tap2) & 1u;  // bit at tap1

        _shift_register = (_shift_register >> 1) | ((_myfirstbit ^ _tap1 ^ _tap2) << (_length - 1));
        turing_machine_.set_shift_register(_shift_register);
      }
        break;
      case CLOCKMODE::RANDOM: {
        // get threshold setting:
        int16_t _n = rand_n();
        if (get_rand_n_cv_source()) {
          _n += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_rand_n_cv_source() - 1)) + 64) >> 7;
          CONSTRAIN(_n, 0, RND_MAX);
        }

        int16_t _rand_new = random(RND_MAX);
        _out = _rand_new > _n ? ON : OFF; // DAC needs special care ...
      }
        break;
      case CLOCKMODE::EUCLID: {
        // simple euclidian algorithm, found on pd hurleur:
        int16_t _n, _k, _offset;
        // the three parameters:
        _n = euclid_n();
        _k = euclid_k();
        _offset = euclid_offset();
        // CV --
        if (get_euclid_n_cv_source()) {
          _n += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_euclid_n_cv_source() - 1)) + 32) >> 6;
          CONSTRAIN(_n, 1, EUCLID_N_MAX);
        }

        if (get_euclid_k_cv_source())
          _k += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_euclid_k_cv_source() - 1)) + 32) >> 6;

        if (get_euclid_offset_cv_source())
          _offset += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_euclid_offset_cv_source() - 1)) + 32) >> 6;

        CONSTRAIN(_k, 1, _n);
        CONSTRAIN(_offset, 0, _n);
        // calculate output:
        clk_cnt_ = clk_cnt_ >= (uint8_t)_n ? 0x0 : clk_cnt_;
        _out = ((clk_cnt_ + _offset) * _k) % _n;
        _out = (_out < _k) ? ON : OFF;
      }
        break;
      case CLOCKMODE::SEQ: {
        // sequencer mode, adapted from o_C scale edit:
        int16_t _seq = get_sequence();

        if (get_sequence_cv_source()) {
          _seq += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_sequence_cv_source() - 1)) + 256) >> 9;
          CONSTRAIN(_seq, 0, TU::Patterns::PATTERN_USER_LAST-1);
        }


        uint8_t _playmode = get_playmode();

        switch (_playmode) {

          case PM_NONE:
          sequence_last_ = _seq;
          break;
          case PM_SEQ1:
          case PM_SEQ2:
          case PM_SEQ3:
          // concatenate sequences:
          {
            if (clk_cnt_ >= get_sequence_length(sequence_last_)) {
                sequence_cnt_++;
                sequence_last_ = _seq + (sequence_cnt_ % (_playmode+1));
                clk_cnt_ = 0;
             }
           }
           break;
           case PM_TR1:
           case PM_TR2:
           case PM_TR3:
           {
           if ( sequence_advance_) {

              if (sequence_reset_) {
                // manual change?
                sequence_reset_ = false;
                sequence_last_ = _seq;
              }

              _playmode -= 3;
              sequence_cnt_++;
              sequence_last_ = _seq + (sequence_cnt_ % (_playmode+1));
              clk_cnt_ = 0;
              sequence_advance_ = false;
            }

            if (sequence_last_ >= TU::Patterns::PATTERN_USER_LAST)
              sequence_last_ -= TU::Patterns::PATTERN_USER_LAST;
          }
          break;
          case PM_SH1:
          case PM_SH2:
          case PM_SH3:
          case PM_SH4:
          {
            int len = get_sequence_length(_seq);
            if (sequence_last_length_ != len)
              update_inputmap(len, 0x0);
             // store values:
             sequence_last_ = _seq;
             sequence_last_length_ = len;
             clk_cnt_ = input_map_.Process(TU::ADC::value(static_cast<ADC_CHANNEL>(_playmode - PM_SH1)));
          }
          break;
          default:
          break;
        }
        // end switch

        _seq = sequence_last_;
        // this is the sequence # (USER1-USER4):
        display_sequence_ = _seq;
        // and corresponding pattern mask:
        uint16_t _mask = get_mask(_seq);
        // rotate mask ?
        if (get_mask_cv_source())
          _mask = update_sequence((TU::ADC::value(static_cast<ADC_CHANNEL>(get_mask_cv_source() - 1)) + 128) >> 8, _seq, _mask);
        display_mask_ = _mask;
        // reset counter ?
        if (clk_cnt_ >= get_sequence_length(_seq))
          clk_cnt_ = 0;
        // output slot at current position:
        _out = (_mask >> clk_cnt_) & 1u;
        _out = _out ? ON : OFF;
      }
        break;
      case CLOCKMODE::DAC: {
        // only available in channel 4:
        int16_t _range = dac_range(); // 1-255
        int mode = values_[CHANNEL_SETTING_DAC_MODE];

        if (get_DAC_mode_cv_source()) {
          mode += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_DAC_mode_cv_source() - 1)) + 256) >> 9;
          CONSTRAIN(mode, 0, static_cast<int>(DACMODE::LAST) - 1);
        }

        if (get_DAC_range_cv_source()) {
          _range += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_DAC_range_cv_source() - 1)) + 8) >> 4;
          CONSTRAIN(_range, 1, 255);
        }

        switch (static_cast<DACMODE>(mode)) {
          case DACMODE::BINARY: {

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
          case DACMODE::RANDOM: {

            uint16_t _history[TU::OUTPUTS::kHistoryDepth];
            int16_t _rand_history, _rand_new, _depth, _weight;

            _depth  = get_history_depth();
            _weight = get_history_weight();

            if (get_rand_history_d_cv_source()) {
              _depth += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_rand_history_d_cv_source() - 1)) + 256) >> 9;
              CONSTRAIN(_depth, 0, (int8_t) TU::OUTPUTS::kHistoryDepth - 1 );
            }
            if (get_rand_history_w_cv_source()) {
              _weight += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_rand_history_w_cv_source() - 1)) + 8) >> 4;
              CONSTRAIN(_weight, 0, 255);
            }

            TU::OUTPUTS::getHistory<CLOCK_CHANNEL_4>(_history);

            _rand_history = calc_average(_history,_depth);
            _rand_history = signed_multiply_32x16b((static_cast<int32_t>(_weight) * 65535U) >> 8, _rand_history);
            _rand_history = signed_saturate_rshift(_rand_history, 16, 0);

            _rand_new = random(0xFFF) - 0x800; // +/- 2048
            _rand_new = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _rand_new);
            _rand_new = signed_saturate_rshift(_rand_new, 16, 0);
            _out = _ZERO - (_rand_new + _rand_history);
          }
            break;
          case DACMODE::TURING: {

            int16_t _length = get_turing_length();
            int16_t _probability = get_turing_probability();

            if (get_turing_length_cv_source()) {
              _length += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_turing_length_cv_source() - 1)) + 64) >> 7;
              CONSTRAIN(_length, LFSR_MIN, LFSR_MAX);
            }

            if (get_turing_prob_cv_source()) {
              _probability += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_turing_prob_cv_source() - 1)) + 8) >> 4;
              CONSTRAIN(_probability, 1, 255);
            }

            turing_machine_.set_length(_length);
            turing_machine_.set_probability(_probability);

            int32_t _shift_register = (static_cast<int16_t>(turing_machine_.Clock()) & 0xFFF);
            // return useful values for small values of _length:
            if (_length < 12) {
              _shift_register = _shift_register << (12 -_length);
              _shift_register = _shift_register & 0xFFF;
            }
            _shift_register -= 0x800; // +/- 2048
            _shift_register = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _shift_register);
            _shift_register = signed_saturate_rshift(_shift_register, 16, 0);
            _out = _ZERO - _shift_register;
          }
            break;
          case DACMODE::LOGISTIC: {

            logistic_map_.set_seed(123);
            int32_t logistic_map_r = get_logistic_map_r();

            if (get_logistic_map_r_cv_source()) {
              logistic_map_r += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_logistic_map_r_cv_source() - 1)) + 8) >> 4;
              CONSTRAIN(logistic_map_r, 0, 255);
            }

            logistic_map_.set_r(logistic_map_r);

            int16_t _logistic_map_x = (static_cast<int16_t>(logistic_map_.Clock()) & 0xFFF) - 0x800; // +/- 2048
            _logistic_map_x = signed_multiply_32x16b((static_cast<int32_t>(_range) * 65535U) >> 8, _logistic_map_x);
            _logistic_map_x = signed_saturate_rshift(_logistic_map_x, 16, 0);
            _out = _ZERO - _logistic_map_x;
          }
            break;
          case DACMODE::SEQ_CV: {

            uint8_t _length = get_cv_sequence_length();
            int8_t _playmode_cv = get_cv_playmode();
            bool _reset = !clk_cnt_ ? true : false;

            switch(_playmode_cv) {

              case _FWD:
              // forward
              {
                seq_cnt_++;

                if (seq_cnt_ >= _length || _reset)
                  seq_cnt_ = 0x0; // reset

                // there's no off state in terms of clocks, so...
                /*
                cv_display_mask_ = get_cv_mask();
                // slot at current position:
                bool _step_state = (cv_display_mask_ >> clk_cnt_) & 1u;
                */
                // ... reset mask until we come up with a better idea:
                cv_display_mask_ = 0xFFFF;
                //update_cv_pattern_mask(0xFFFF);

                _out = get_pitch_at_step(seq_cnt_);
                clk_cnt_ = seq_cnt_;
              }
              break;
              case _REV:
              {
                seq_cnt_--;

                if (seq_cnt_ < 0x0 || _reset)
                  seq_cnt_ = _length - 0x1; // reset

                // ... reset mask until we come up with a better idea:
                cv_display_mask_ = 0xFFFF;
                //update_cv_pattern_mask(0xFFFF);

                _out = get_pitch_at_step(seq_cnt_);
                clk_cnt_ = seq_cnt_;
              }
              // reverse
              break;
              case _PND1:
              // pendulum1
              {
                // ... reset mask until we come up with a better idea:
                cv_display_mask_ = 0xFFFF;
                //update_cv_pattern_mask(0xFFFF);

                if (seq_direction_) {
                  seq_cnt_++;
                  if (_reset)
                    seq_cnt_ = 0x0;
                  else if (seq_cnt_ >= _length - 1)
                    seq_direction_ = false;
                }
                else {
                  seq_cnt_--;
                  if (_reset)
                    seq_cnt_ = _length - 0x1;
                  else if (seq_cnt_ <= 0)
                    seq_direction_ = true;
                }
                _out = get_pitch_at_step(seq_cnt_);
                clk_cnt_ = seq_cnt_;
              }
              break;
              case _PND2:
              //pendulum2
              {
                // ... reset mask until we come up with a better idea:
                cv_display_mask_ = 0xFFFF;
                //update_cv_pattern_mask(0xFFFF);

                if (seq_direction_) {
                  seq_cnt_++;
                   if (_reset)
                    seq_cnt_ = 0x0;
                   else if (seq_cnt_ > _length - 1) {
                    seq_direction_ = false;
                    seq_cnt_ = _length - 1;
                  }
                }
                else {
                  seq_cnt_--;
                  if (_reset)
                    seq_cnt_ = _length - 0x1;
                  else if (seq_cnt_ < 0) {
                    seq_direction_ = true;
                    seq_cnt_ = 0x0;
                  }
                }
                _out = get_pitch_at_step(seq_cnt_);
                clk_cnt_ = seq_cnt_;
              }
              break;
              case _RND:
              // random
              {
                // ... reset mask until we come up with a better idea:
                cv_display_mask_ = 0xFFFF;
                //update_cv_pattern_mask(0xFFFF);

                int rnd = _reset ? 0x0 : random(_length);
                _out = get_pitch_at_step(rnd);
                clk_cnt_ = rnd;
              }
              break;
              case _ARP:
              // ARP
              {
                if (_reset)
                  arpeggiator_.reset();
                // range:
                int _range = get_arp_range();
                if (get_arp_range_cv_source())
                  _range += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_arp_range_cv_source() - 1)) + 256) >> 9;
                CONSTRAIN(_range, 0, 5);
                arpeggiator_.set_range(_range);

                // direction:
                int _direction = get_arp_direction();
                if (get_arp_direction_cv_source())
                  _direction += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_arp_direction_cv_source() - 1)) + 256) >> 9;
                CONSTRAIN(_direction, 0, 3);
                arpeggiator_.set_direction(_direction);

                // rotate?
                if (get_arp_mask_cv_source() || (cv_display_mask_ != get_cv_mask()))
                  cv_pattern_changed(get_cv_mask(), false);

                _out = arpeggiator_.ClockArpeggiator();
                clk_cnt_ = 0x0;
              }
              break;
              default:
              break;
            }
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

    if (mode == CLOCKMODE::DAC) {
      // offset
      uint16_t v_oct = TU::OUTPUTS::get_v_oct();
      int8_t _offset = dac_offset();

      if (get_DAC_offset_cv_source())
        _offset += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_DAC_offset_cv_source() - 1)) + 256) >> 9;
      _out += _offset * v_oct;

      bool corrected = false;
      while (_out > TU::OUTPUTS::PITCH_LIMIT) {
        _out -= v_oct;
        corrected = true;
      }
      while (_out < TU::calibration_data.dac.calibration_points[0x0][0x0]) {
        _out += v_oct;
        corrected = true;
      }
      if (corrected)
        dac_overflow_ = _out;
      else
        dac_overflow_ = 0xFFF;
    }

    return _out;
  }

  inline void logic(CLOCK_CHANNEL clock_channel) {

    if (!logic_)
      return;
    logic_ = false;

    // else logic ...
    uint16_t _out = OFF;
    int8_t _op1, _op2, _type;

    _type = logic_type();
    _op1  = logic_op1();
    _op2  = logic_op2();

    if (get_logic_type_cv_source()) {
      _type += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_logic_type_cv_source() - 1)) + 256) >> 9;
      CONSTRAIN(_type, 0, LOGICMODE_LAST-1);
    }

    if (get_logic_op1_cv_source())  {
      _op1 += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_logic_op1_cv_source() - 1)) + 127) >> 8;
      CONSTRAIN(_op1, 0, NUM_CHANNELS-1);
    }

    if (get_logic_op2_cv_source()) {
      _op2 += (TU::ADC::value(static_cast<ADC_CHANNEL>(get_logic_op2_cv_source() - 1)) + 127) >> 8;
      CONSTRAIN(_op2, 0, NUM_CHANNELS-1);
    }

    // this doesn't care if CHANNEL_4 is in DAC mode (= mostly always true); but so what.
    if (!logic_tracking()) {
      _op1 = TU::OUTPUTS::state(_op1) & 1u;
      _op2 = TU::OUTPUTS::state(_op2) & 1u;
    }
    else {
      _op1 = TU::OUTPUTS::value(_op1) & 1u;
      _op2 = TU::OUTPUTS::value(_op2) & 1u;
    }

    // and go through the options ...
    switch (_type) {

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
    } // end logic op switch

    // write to output:
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


  ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {

    ChannelSetting *settings = enabled_settings_;
    CLOCKMODE mode = !is_dac_channel() ? get_mode() : get_mode();

    if (menu_page_ != TEMPO)
      *settings++ = CHANNEL_SETTING_MODE;
    else
      *settings++ = CHANNEL_SETTING_INTERNAL_CLK;


    if (menu_page_ == CV_SOURCES) {

      *settings++ = CHANNEL_SETTING_MULT_CV_SOURCE;
      *settings++ = CHANNEL_SETTING_SWING_CV_SOURCE;
      
      if (mode != CLOCKMODE::DAC)
        *settings++ = CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE;

      switch (mode) {

        case CLOCKMODE::LFSR:
          *settings++ = CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_TURING_PROB_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_LFSR_TAP1_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_LFSR_TAP2_CV_SOURCE;
          break;
        case CLOCKMODE::EUCLID:
          *settings++ = CHANNEL_SETTING_EUCLID_N_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_EUCLID_K_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_EUCLID_OFFSET_CV_SOURCE;
          break;
        case CLOCKMODE::RANDOM:
          *settings++ = CHANNEL_SETTING_RAND_N_CV_SOURCE;
          break;
        case CLOCKMODE::LOGIC:
          *settings++ = CHANNEL_SETTING_LOGIC_TYPE_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_LOGIC_OP1_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_LOGIC_OP2_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_SEPARATOR;
          break;
        case CLOCKMODE::SEQ:
          *settings++ = CHANNEL_SETTING_SEQ_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_MASK_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_SEPARATOR; // playmode CV
          break;
        case CLOCKMODE::BURST:
          *settings++ = CHANNEL_SETTING_BURST_DENSITY_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_BURST_INITIAL_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_BURST_DAMP_CV_SOURCE;
          break;
        case CLOCKMODE::DAC:
          *settings++ = CHANNEL_SETTING_DAC_MODE_CV_SOURCE;
          *settings++ = CHANNEL_SETTING_DAC_OFFSET_CV_SOURCE;

          if (dac_mode() != DACMODE::SEQ_CV)
            *settings++ = CHANNEL_SETTING_DAC_RANGE_CV_SOURCE;

          switch (dac_mode()) {
            case DACMODE::BINARY:
              *settings++ = CHANNEL_SETTING_SEPARATOR;
              break;
            case DACMODE::RANDOM:
              *settings++ = CHANNEL_SETTING_DAC_RANGE_CV_SOURCE;
              *settings++ = CHANNEL_SETTING_HISTORY_WEIGHT_CV_SOURCE;
              *settings++ = CHANNEL_SETTING_HISTORY_DEPTH_CV_SOURCE;
              break;
            case DACMODE::TURING:
              *settings++ = CHANNEL_SETTING_TURING_LENGTH_CV_SOURCE;
              *settings++ = CHANNEL_SETTING_TURING_PROB_CV_SOURCE;
              break;
            case DACMODE::LOGISTIC:
              *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE;
              break;
            case DACMODE::SEQ_CV:
              if (get_cv_playmode() == _ARP)
                *settings++ = CHANNEL_SETTING_SEQ_MASK_CV_SOURCE;
              else
                *settings++ = CHANNEL_SETTING_MASK_CV;
              *settings++ = CHANNEL_SETTING_CV_SEQUENCE_PLAYMODE;
              if (get_cv_playmode() == _ARP) {
                *settings++ = CHANNEL_SETTING_ARP_DIRECTION_CV_SOURCE;
                *settings++ =  CHANNEL_SETTING_ARP_RANGE_CV_SOURCE;
              }
              break;
            default:
              break;
          }
        default:
          break;
      }
      *settings++ = CHANNEL_SETTING_CLOCK_CV_SOURCE;
      //if (mode == SEQ || mode == EUCLID)
      *settings++ = CHANNEL_SETTING_TRIGGER_DELAY; // make # items the same / no CV for reset source ...

    }

    else if (menu_page_ == PARAMETERS) {

      *settings++ = CHANNEL_SETTING_MULT;
      *settings++ = CHANNEL_SETTING_SWING;
      
      if (mode != CLOCKMODE::DAC)
        *settings++ = CHANNEL_SETTING_PULSEWIDTH;

      switch (mode) {

        case CLOCKMODE::LFSR:
          *settings++ = CHANNEL_SETTING_TURING_LENGTH;
          *settings++ = CHANNEL_SETTING_TURING_PROB;
          *settings++ = CHANNEL_SETTING_LFSR_TAP1;
          *settings++ = CHANNEL_SETTING_LFSR_TAP2;
          break;
        case CLOCKMODE::EUCLID:
          *settings++ = CHANNEL_SETTING_EUCLID_N;
          *settings++ = CHANNEL_SETTING_EUCLID_K;
          *settings++ = CHANNEL_SETTING_EUCLID_OFFSET;
          break;
        case CLOCKMODE::RANDOM:
          *settings++ = CHANNEL_SETTING_RAND_N;
          break;
        case CLOCKMODE::LOGIC:
          *settings++ = CHANNEL_SETTING_LOGIC_TYPE;
          *settings++ = CHANNEL_SETTING_LOGIC_OP1;
          *settings++ = CHANNEL_SETTING_LOGIC_OP2;
          *settings++ = CHANNEL_SETTING_LOGIC_TRACK_WHAT;
          break;
        case CLOCKMODE::SEQ:
          *settings++ = CHANNEL_SETTING_SEQUENCE;

          switch (get_sequence()) {
            case 0:
              *settings++ = CHANNEL_SETTING_MASK1;
              break;
            case 1:
              *settings++ = CHANNEL_SETTING_MASK2;
              break;
            case 2:
              *settings++ = CHANNEL_SETTING_MASK3;
              break;
            case 3:
              *settings++ = CHANNEL_SETTING_MASK4;
              break;
            default:
              break;
          }
          *settings++ = CHANNEL_SETTING_SEQUENCE_PLAYMODE;
          break;
        case CLOCKMODE::BURST:
          *settings++ = CHANNEL_SETTING_BURST_DENSITY;
          *settings++ = CHANNEL_SETTING_BURST_INITIAL;
          *settings++ = CHANNEL_SETTING_BURST_DAMP;
          break;
        case CLOCKMODE::DAC:
          *settings++ = CHANNEL_SETTING_DAC_MODE;
          *settings++ = CHANNEL_SETTING_DAC_OFFSET;

          if (dac_mode() != DACMODE::SEQ_CV)
            *settings++ = CHANNEL_SETTING_DAC_RANGE;

          switch (dac_mode())  {

            case DACMODE::BINARY:

              *settings++ = CHANNEL_SETTING_DAC_TRACK_WHAT;
              break;
            case DACMODE::RANDOM:
              *settings++ = CHANNEL_SETTING_HISTORY_WEIGHT;
              *settings++ = CHANNEL_SETTING_HISTORY_DEPTH;
              break;
            case DACMODE::TURING:
              *settings++ = CHANNEL_SETTING_TURING_LENGTH;
              *settings++ = CHANNEL_SETTING_TURING_PROB;
              break;
            case DACMODE::LOGISTIC:
              *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R;
              break;
            case DACMODE::SEQ_CV:
              *settings++ = CHANNEL_SETTING_MASK_CV;
              *settings++ = CHANNEL_SETTING_CV_SEQUENCE_PLAYMODE;
              if (get_cv_playmode() == _ARP)  {
                *settings++ =  CHANNEL_SETTING_ARP_DIRECTION;
                *settings++ =  CHANNEL_SETTING_ARP_RANGE;
              }
              break;
            default:
              break;
          }
          break;
        default:
          break;
      } // end mode switch

      *settings++ = CHANNEL_SETTING_CLOCK;
      if (mode != CLOCKMODE::BURST)
        *settings++ = CHANNEL_SETTING_RESET;
      else {
        if (get_clock_source() == CHANNEL_TRIGGER_INTERNAL)
          *settings++ = CHANNEL_SETTING_BURST_TRIG_SOURCE_INT;
        else
          *settings++ = CHANNEL_SETTING_TRIGGER_DELAY;
      }
    }
    else if (menu_page_ == TEMPO) {

      *settings++ = CHANNEL_SETTING_DUMMY_EMPTY;
      *settings++ = CHANNEL_SETTING_SCREENSAVER;
      *settings++ = CHANNEL_SETTING_DUMMY_EMPTY;
    }
    num_enabled_settings_ = settings - enabled_settings_;
  }


  uint16_t update_sequence(int32_t mask_rotate_, uint8_t sequence_, uint16_t mask_) {

    const int sequence_num = sequence_;
    uint16_t mask = mask_;

    if (mask_rotate_)
      mask = TU::PatternEditor<Clock_channel>::RotateMask(mask, get_sequence_length(sequence_num), mask_rotate_);
    return mask;
  }

  void RenderScreensaver(weegfx::coord_t start_x) const;

  inline bool is_dac_channel() const {
    return channel_ == CLOCK_CHANNEL_4;
  }

private:
  CLOCK_CHANNEL channel_;

  bool sync_;
  bool phase_;
  uint8_t prev_phase_;
  bool skip_reset_;
  bool force_update_;
  uint16_t _ZERO;
  uint8_t clk_src_;
  int8_t pending_reset_;
  uint32_t ticks_;
  uint32_t subticks_;
  int32_t phase_ticks_;
  uint32_t tickjitter_;
  uint32_t clk_cnt_;
  int16_t div_cnt_;
  int16_t mult_cnt_;
  int16_t seq_cnt_;
  bool seq_direction_;
  uint32_t ext_frequency_in_ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;
  uint16_t gpio_state_;
  uint16_t display_state_;
  uint8_t prev_multiplier_;
  uint8_t prev_pulsewidth_;
  uint8_t pending_multiplier_;
  bool pending_sync_;
  bool pending_new_burst_;
  bool wait_burst_;
  uint32_t burst_complete_;
  uint8_t logic_;
  uint8_t display_sequence_;
  uint16_t display_mask_;
  uint16_t cv_display_mask_;
  int8_t sequence_last_;
  int8_t sequence_last_length_;
  int32_t sequence_cnt_;
  int8_t sequence_advance_;
  int8_t sequence_advance_state_;
  int8_t sequence_reset_;
  int8_t last_trigger_state_;
  uint8_t menu_page_;
  uint16_t bpm_last_;
  int16_t prev_mask_rotate_;
  int8_t prev_cv_playmode_;
  int16_t dac_overflow_;

  util::TuringShiftRegister turing_machine_;
  util::Arpeggiator arpeggiator_;
  util::LogisticMap logistic_map_;
  util::Bursts bursts_;
  util::Phase Phase_;
  util::TriggerDelay<TU::kMaxTriggerDelayTicks> trigger_delay_;
  int num_enabled_settings_;
  ChannelSetting enabled_settings_[CHANNEL_SETTING_LAST];
  TU::DigitalInputDisplay clock_display_;
  TU::Input_Map input_map_;

};

const char* const channel_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "-", "INT"
};

const char* const reset_trigger_sources[CHANNEL_TRIGGER_LAST+1] = {
  "RST1", "RST2", "-", "=HI2", "=LO2"
};

const char* const multipliers[] = {
  "/64", "/48", "/32", "/24", "/16", "/12", "/8", "/7", "/6", "/5", "/4", "/3", "/2",
  "-",
  "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x12", "x16", "x24", "x32", "x48", "x64",
  "QN24", "QN96"
};

const char* const cv_sources[5] = {
  "--", "CV1", "CV2", "CV3", "CV4"
};

const char* const arp_directions[4] = {
  "up", "down", "u/d", "rnd"
};

SETTINGS_DECLARE(Clock_channel, CHANNEL_SETTING_LAST) {

  // NOTE special-cased during editing/display to enable/disable DAC mode
  { 0, 0, static_cast<int>(CLOCKMODE::LAST) - 1, "mode", TU::Strings::mode, settings::STORAGE_TYPE_U4 },

  { CHANNEL_TRIGGER_TR1,  0, CHANNEL_TRIGGER_LAST - 1, "clock src", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, TU::kNumDelayTimes - 1, "latency", TU::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { CHANNEL_TRIGGER_NONE, 0, CHANNEL_TRIGGER_LAST, "reset/mute", reset_trigger_sources, settings::STORAGE_TYPE_U8 },
  { MULT_BY_ONE, 0, MULT_MAX, "mult/div", multipliers, settings::STORAGE_TYPE_U8 },
  { 25, 0, PULSEW_MAX, "pulsewidth", TU::Strings::pulsewidth_ms, settings::STORAGE_TYPE_U8 },
  { 0, 0, PHASEOFFSET_MAX, "phase %", NULL, settings::STORAGE_TYPE_U8 },
  // note that BPM is a channel setting, although it's a global value.
  // when recalling, we just grab the value for channel 0
  { 100, BPM_MIN, BPM_MAX, "BPM:", NULL, settings::STORAGE_TYPE_U16 },
  //
  { 0, 0, 31, "LFSR tap1",NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR) },
  { 0, 0, 31, "LFSR tap2",NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR) },
  { 0, 0, RND_MAX, "rand > n", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::RANDOM) },
  { 3, 3, EUCLID_N_MAX, "euclid: N", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 1, 1, EUCLID_N_MAX, "euclid: K", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 0, 0, EUCLID_N_MAX-1, "euclid: OFFSET", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 0, 0, LOGICMODE_LAST-1, "logic type", TU::Strings::operators, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 0, 0, NUM_CHANNELS - 1, "op_1", TU::Strings::channel_id, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 1, 0, NUM_CHANNELS - 1, "op_2", TU::Strings::channel_id, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 0, 0, 1, "track -->", TU::Strings::logic_tracking, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 128, 1, 255, "DAC: range", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  #ifdef MOD_OFFSET
  { 0, -1, 5, "DAC: offset", NULL, settings::STORAGE_TYPE_I8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  #else
    #ifdef MODEL_2TT
    {  0, 0, 6, "DAC: offset", NULL, settings::STORAGE_TYPE_I8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
    #else
    { 0, -3, 3, "DAC: offset", NULL, settings::STORAGE_TYPE_I8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
    #endif
  #endif
  { 0, 0, static_cast<int>(DACMODE::LAST) - 1, "DAC: mode", TU::Strings::dac_modes, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 1, "track -->", TU::Strings::binary_tracking, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 255, "rnd hist.", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { 0, 0, TU::OUTPUTS::kHistoryDepth - 1, "hist. depth", NULL, settings::STORAGE_TYPE_U8 }, /// "history"
  { LFSR_MIN << 1, LFSR_MIN, LFSR_MAX, "LFSR length", NULL, settings::STORAGE_TYPE_U8, /*, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR)*/ }, // used by DAC and LFSR modes
  { 128, 0, 255, "LFSR p(x)", NULL, settings::STORAGE_TYPE_U8, /*, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR)*/ }, // used by DAC and LFSR modes
  { 1, 1, 255, "LGST(R)", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 1, 0, 4, "arp.range", NULL, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "arp.direction", arp_directions, settings::STORAGE_TYPE_U4 },

  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 1
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 2
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 3
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 4
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq CV

  { TU::Patterns::PATTERN_USER_0, 0, TU::Patterns::PATTERN_USER_LAST-1, "sequence #", TU::pattern_names_short, settings::STORAGE_TYPE_U8 },
  { TU::Patterns::kMax, TU::Patterns::kMin, TU::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 1
  { TU::Patterns::kMax, TU::Patterns::kMin, TU::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 2
  { TU::Patterns::kMax, TU::Patterns::kMin, TU::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 3
  { TU::Patterns::kMax, TU::Patterns::kMin, TU::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 4
  { 0, 0, PM_LAST - 1, "playmode", TU::Strings::seq_playmodes, settings::STORAGE_TYPE_U8 },
  { TU::Patterns::kMax, TU::Patterns::kMin, TU::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // CV seq
  { 0, 0, 5, "playmode", TU::Strings::cv_seq_playmodes, settings::STORAGE_TYPE_U4 }, // CV playmode
  { 1, 1, 31, "density", NULL, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { 20, 0, 40, "f (initial)", TU::Strings::initial_f, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { 30, 0, 60, "damping", TU::Strings::damping, settings::STORAGE_TYPE_U8, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { CHANNEL_TRIGGER_TR1,  0, CHANNEL_TRIGGER_TR2, "burst src", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  // cv sources
  { 0, 0, 4, "mult/div    ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "pulsewidth  ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "phase %     ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "clock src   ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR tap1   ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR) },
  { 0, 0, 4, "LFSR tap2   ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR) },
  { 0, 0, 4, "rand > n    ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::RANDOM) },
  { 0, 0, 4, "euclid: N   ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 0, 0, 4, "euclid: K   ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 0, 0, 4, "euclid: OFF ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::EUCLID) },
  { 0, 0, 4, "logic type  ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 0, 0, 4, "op_1        ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 0, 0, 4, "op_2        ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LOGIC) },
  { 0, 0, 4, "LFSR p(x)   ->", cv_sources, settings::STORAGE_TYPE_U4 /*, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR)*/ }, // used by DAC and LFSR modes
  { 0, 0, 4, "LFSR length ->", cv_sources, settings::STORAGE_TYPE_U4 /*, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::LFSR)*/ },
  { 0, 0, 4, "LGST(R)     ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 4, "sequence #  ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "mask        ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "mask        ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "DAC: range  ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 4, "DAC: mode   ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 4, "DAC: offset ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::DAC) },
  { 0, 0, 4, "rnd hist.   ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "hist. depth ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "arp.range   ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "arp.direc.  ->", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "density     ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { 0, 0, 4, "f (initial) ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { 0, 0, 4, "damping     ->", cv_sources, settings::STORAGE_TYPE_U4, VALID_IF(CHANNEL_SETTING_MODE, CLOCKMODE::BURST) },
  { 0, 0, 0, "---------------------", NULL, settings::STORAGE_TYPE_NOP }, // SEPARATOR
  { 0, 0, 0, "  ", NULL, settings::STORAGE_TYPE_NOP }, // DUMMY empty
  { 0, 0, 0, "  ", NULL, settings::STORAGE_TYPE_NOP }  // screensaver
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
  menu::ScreenCursor<menu::kScreenLines> cursor_state;
  TU::PatternEditor<Clock_channel> pattern_editor;
};

ClocksState clocks_state;
Clock_channel clock_channel[NUM_CHANNELS];

void CLOCKS_init() {

  TU::Patterns::Init();
  TU::Patterns::Fill();

  ext_frequency[CHANNEL_TRIGGER_TR1]  = 0xFFFFFFFF;
  ext_frequency[CHANNEL_TRIGGER_TR2]  = 0xFFFFFFFF;
  ext_frequency[CHANNEL_TRIGGER_NONE] = 0xFFFFFFFF;
  ext_frequency[CHANNEL_TRIGGER_INTERNAL] = 0xFFFFFFFF;

  int_frequency = pending_int_frequency = 0xFFFFFFFF;

  clocks_state.Init();
  for (int i = 0; i < CLOCK_CHANNEL_LAST; ++i)
    clock_channel[i].Init(static_cast<CLOCK_CHANNEL>(i), static_cast<ChannelTriggerSource>(CHANNEL_TRIGGER_TR1));
  clocks_state.cursor.AdjustEnd(clock_channel[0].num_enabled_settings() - 1);
}

void CLOCKS_reset() {
  TU::global_config.Reset();
  CLOCKS_init();
}

size_t CLOCKS_storageSize() {
  return 
    sizeof(TU::user_patterns) +
    NUM_CHANNELS * Clock_channel::storageSize() + 
    TU::GlobalConfig::storageSize();
}

size_t CLOCKS_save(util::StreamBufferWriter &stream_buffer) {

  stream_buffer.Write(TU::user_patterns);
  std::for_each(
      std::begin(clock_channel),
      std::end(clock_channel),
      [&](const Clock_channel &c) {
        c.Save(stream_buffer);
      });
  TU::global_config.Save(stream_buffer);

  return stream_buffer.overflow() ? 0 : stream_buffer.written();
}

size_t CLOCKS_restore(util::StreamBufferReader &stream_buffer) {

  stream_buffer.Read(TU::user_patterns);
  for (auto &channel : clock_channel) {
    channel.Restore(stream_buffer);
    channel.update_enabled_settings();
    // update display sequence + mask:
    channel.set_display_sequence(channel.get_sequence());
    channel.init_pattern(channel.get_mask(channel.get_sequence()));
    channel.reset_channel_frequency();
  }
  clocks_state.cursor.AdjustEnd(clock_channel[0].num_enabled_settings() - 1);

  // TODO: maybe this should be a global value in the storage block to save memory, not a big deal, though
  ext_frequency[CHANNEL_TRIGGER_INTERNAL] = BPM_microseconds_4th[clock_channel[0].get_internal_timer() - BPM_MIN];
  // update channel 4:
  clock_channel[CLOCK_CHANNEL_4].cv_pattern_changed(clock_channel[CLOCK_CHANNEL_4].get_cv_mask(), true);

  TU::global_config.Restore(stream_buffer);

  return stream_buffer.underflow() ? 0 : stream_buffer.read();
}

void CLOCKS_handleAppEvent(TU::AppEvent event) {

  switch (event) {
    case TU::APP_EVENT_RESUME:
    {
      clocks_state.cursor.set_editing(false);
      clocks_state.pattern_editor.Close();
      for (int i = 0; i < NUM_CHANNELS; ++i) {
        clock_channel[i].sync();
        clock_channel[i].set_page(PARAMETERS);
      }
    }
    break;
    case TU::APP_EVENT_SUSPEND:
    case TU::APP_EVENT_SCREENSAVER_ON:
    case TU::APP_EVENT_SCREENSAVER_OFF:
    {
      Clock_channel &selected = clock_channel[clocks_state.selected_channel];
      if (selected.get_page() > PARAMETERS) {
        selected.set_page(PARAMETERS);
        selected.update_enabled_settings();
        clocks_state.cursor.Init(CHANNEL_SETTING_MODE, 0);
        clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
      }
    }
    break;
    default:
    break;
  }
}

void CLOCKS_loop() {
}

void CLOCKS_isr() {

  ticks_src1++; // src #1 ticks
  ticks_src2++; // src #2 ticks
  ticks_internal++; // internal clock ticks

  uint32_t triggers = TU::DigitalInputs::clocked();

  // TR1:
  if (triggers & (1 << CHANNEL_TRIGGER_TR1))  {

    global_div_count_TR1--;
    uint8_t global_divisor_ = TU::DigitalInputs::global_div_TR1();

    // sync/reset global divisor? defaults to TR2
    if (global_divisor_ && clock_channel[CLOCK_CHANNEL_1].get_reset_source() == CHANNEL_TRIGGER_TR2) {

      bool global_reset = digitalReadFast(TR2);
      // reset ?
      if (global_reset < RESET_GLOBAL_TR2)
          global_div_count_TR1 = 0x0;
      RESET_GLOBAL_TR2 = global_reset;
    }

    if (global_div_count_TR1 <= 0x0) {
        // reset
        global_div_count_TR1 = global_divisor_;
        ext_frequency[CHANNEL_TRIGGER_TR1] = ticks_src1;
        ticks_src1 = 0x0;
    }
    else // clear trigger
      triggers &= ~ (1 << CHANNEL_TRIGGER_TR1);
  }

  // TR2:
  if (triggers & (1 << CHANNEL_TRIGGER_TR2)) {
    ext_frequency[CHANNEL_TRIGGER_TR2] = ticks_src2;
    ticks_src2 = 0x0;
  }

  // update this once in the same thread as the processing to avoid race conditions and sync problems
  if (pending_int_frequency != int_frequency) {
    int_frequency = pending_int_frequency;
    ext_frequency[CHANNEL_TRIGGER_INTERNAL] = BPM_microseconds_4th[int_frequency - BPM_MIN];
    // following is only needed for save/recall of settings
    for (int i = 0; i < 6; i++) {
      clock_channel[i].update_internal_timer(int_frequency);
    }
  }

  if (ticks_internal >= ext_frequency[CHANNEL_TRIGGER_INTERNAL]) {
    triggers |= (1 << CHANNEL_TRIGGER_INTERNAL);
    ticks_internal = 0x0;
  }

  uint8_t mute = 0xFF;
  if (ticks_src1 > (ext_frequency[CHANNEL_TRIGGER_TR1] << 2))
    mute = CHANNEL_TRIGGER_TR1;

  if (ticks_src2 > (ext_frequency[CHANNEL_TRIGGER_TR2] << 2))
    mute = CHANNEL_TRIGGER_TR2;

  if (TU::DigitalInputs::master_clock()) {

    if (triggers & (1 << CHANNEL_TRIGGER_TR1)) {

      // track master clock:
      uint8_t min_mult = MULT_MAX;
      uint8_t master_channel = 0x0;

      for (size_t i = 0; i < NUM_CHANNELS; ++i) {
        uint8_t mult = clock_channel[i].get_effective_multiplier();
        if (mult <= min_mult) {
          min_mult = mult;
          master_channel = i;
        }
      }
      // channel is about to reset, so reset slave channels --
      if (min_mult <= MULT_BY_ONE && clock_channel[master_channel].get_div_cnt() <= 0x1) {

        for (size_t i = 0; i < NUM_CHANNELS; ++i)  {

          if (clock_channel[i].slave()) {

              uint8_t mult = clock_channel[i].get_effective_multiplier();
              uint32_t clock_count = 0; uint32_t div_count = 0;

              for (size_t j = 0; j < NUM_CHANNELS; ++j) {
                if (i != j) {
                    if (clock_channel[j].get_effective_multiplier() == mult)
                    {
                       clock_count = clock_channel[j].get_clock_cnt();
                       div_count = clock_channel[j].get_div_cnt();
                    }
                }
              }
              // resync channel
              clock_channel[i].resync(clock_count, div_count);
          }
        }
      }
    }
  }

  // update channels:
  clock_channel[CLOCK_CHANNEL_1].Update(triggers, mute);
  clock_channel[CLOCK_CHANNEL_2].Update(triggers, mute);
  clock_channel[CLOCK_CHANNEL_3].Update(triggers, mute);
  clock_channel[CLOCK_CHANNEL_5].Update(triggers, mute);
  clock_channel[CLOCK_CHANNEL_6].Update(triggers, mute);
  clock_channel[CLOCK_CHANNEL_4].Update(triggers, mute); // = DAC channel; update last, because of the binary Sequencer thing.

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
        CLOCKS_upButtonLong();
        break;
      case TU::CONTROL_BUTTON_DOWN:
        CLOCKS_downButtonLong();
        break;
      case TU::CONTROL_BUTTON_L:
        if (!(clocks_state.pattern_editor.active()))
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

    int previous_channel = clocks_state.selected_channel;
    Clock_channel &previous= clock_channel[previous_channel];

    if (previous.get_page() == TEMPO) {
      previous.set_page(PARAMETERS);
      clocks_state.cursor.set_editing(false);
      previous.update_enabled_settings();
      clocks_state.cursor.AdjustEnd(previous.num_enabled_settings() - 1);
      return;
    }

    int selected_channel = clocks_state.selected_channel + event.value;
    CONSTRAIN(selected_channel, 0, NUM_CHANNELS-1);
    clocks_state.selected_channel = selected_channel;
    Clock_channel &selected = clock_channel[clocks_state.selected_channel];

    if (previous.get_page() == CV_SOURCES)
      selected.set_page(CV_SOURCES);
    else if (previous.get_page() == PARAMETERS)
      selected.set_page(PARAMETERS);

    selected.update_enabled_settings();

    if (previous.num_enabled_settings() > selected.num_enabled_settings()) {
      int _cursor = clocks_state.cursor.cursor_pos();
      clocks_state.cursor.reInit(CHANNEL_SETTING_MODE, 0);
      clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
      if (_cursor > selected.num_enabled_settings() - 1)
        clocks_state.cursor.Scroll(selected.num_enabled_settings() - 1);
      else
        clocks_state.cursor.Scroll(_cursor);
    }
    else
      clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);

  } else if (TU::CONTROL_ENCODER_R == event.control) {

    Clock_channel &selected = clock_channel[clocks_state.selected_channel];

    if (selected.get_page() == TEMPO) {

      uint16_t int_clock_used_ = 0x0;
      for (int i = 0; i < NUM_CHANNELS; i++)
        int_clock_used_ += selected.get_clock_source() == CHANNEL_TRIGGER_INTERNAL ? 0x10 : 0x00;
      if (!int_clock_used_) {
        selected.set_page(PARAMETERS);
        clocks_state.cursor = clocks_state.cursor_state;
        selected.update_enabled_settings();
        return;
      }
    }

    if (clocks_state.editing()) {

      ChannelSetting setting = selected.enabled_setting_at(clocks_state.cursor_pos());

      if (CHANNEL_SETTING_MASK1 != setting || CHANNEL_SETTING_MASK2 != setting || CHANNEL_SETTING_MASK3 != setting || CHANNEL_SETTING_MASK4 != setting || CHANNEL_SETTING_MASK_CV != setting) {

        bool changed;
        if (CHANNEL_SETTING_MODE != setting) {
          changed = selected.change_value(setting, event.value);
        } else {
          changed = selected.change_value_max(
                        setting,
                        event.value,
                        selected.is_dac_channel() ? static_cast<int>(CLOCKMODE::DAC) : static_cast<int>(CLOCKMODE::BURST));
        }
        if (changed)
          selected.force_update();

        switch (setting) {

          case CHANNEL_SETTING_SEQUENCE:
          {
            if (!selected.get_playmode()) {
              uint8_t seq = selected.get_sequence();
              selected.pattern_changed(selected.get_mask(seq), true);
              selected.set_display_sequence(seq);
            }
            // force update, when TR+1 etc
            selected.reset_sequence();
          }
            break;
          case CHANNEL_SETTING_MODE:
          case CHANNEL_SETTING_DAC_MODE:
            selected.update_enabled_settings();
            clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
            break;
          case CHANNEL_SETTING_CV_SEQUENCE_PLAYMODE: 
          // update ARP if need be
            selected.update_enabled_settings();
            clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
            selected.cv_pattern_changed(selected.get_cv_mask(), false);
            break;
          // special cases:
          case CHANNEL_SETTING_EUCLID_N:
          case CHANNEL_SETTING_EUCLID_K:
          {
            uint8_t _n = selected.euclid_n();
            if (selected.euclid_k() > _n)
              selected.set_euclid_k(_n);
            if (selected.euclid_offset() > _n)
              selected.set_euclid_offset(_n);
          }
            break;
          case CHANNEL_SETTING_EUCLID_OFFSET:
          {
            uint8_t _n = selected.euclid_n();
            if (selected.euclid_offset() > _n)
              selected.set_euclid_offset(_n);
          }
            break;
          case CHANNEL_SETTING_TURING_LENGTH:
          {
            uint8_t _len = selected.get_turing_length();
            if (selected.get_tap1() > _len)
              selected.set_tap1(_len);
            if (selected.get_tap2() > _len)
              selected.set_tap2(_len);
          }
            break;
          case CHANNEL_SETTING_LFSR_TAP1:
          {
            uint8_t _len = selected.get_turing_length();
            if (selected.get_tap1() > _len)
              selected.set_tap1(_len);
          }
            break;
          case CHANNEL_SETTING_LFSR_TAP2:
          {
            uint8_t _len = selected.get_turing_length();
            if (selected.get_tap2() > _len)
              selected.set_tap2(_len);
          }
            break;
          case CHANNEL_SETTING_CLOCK: {
            if (selected.get_mode() == CLOCKMODE::BURST) {
              // show burst src
              selected.update_enabled_settings();
              clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
            }
          }
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

  if (!TU::ui.read_immediate(TU::CONTROL_BUTTON_L)) {

    Clock_channel &selected = clock_channel[clocks_state.selected_channel];

    uint8_t _menu_page = selected.get_page();

    switch (_menu_page) {

      case TEMPO:
      {
        clocks_state.cursor = clocks_state.cursor_state;
        // disallow case where things would end up in editor:
        if (selected.enabled_setting_at(clocks_state.cursor_pos()) == CHANNEL_SETTING_SEQ_MASK_CV_SOURCE)
          clocks_state.cursor.set_editing(false); 
        selected.set_page(PARAMETERS);
      }
        break;
      default:
        clocks_state.cursor_state = clocks_state.cursor;
        selected.set_page(TEMPO);
        clocks_state.cursor.Init(CHANNEL_SETTING_MODE, 0);
        clocks_state.cursor.toggle_editing();
        break;
    }
    selected.update_enabled_settings();
    clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
  }
}

void CLOCKS_downButton() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];

  uint8_t _menu_page = selected.get_page();

  switch (_menu_page) {

    case CV_SOURCES:
      {
      // don't get stuck:
      int pos = selected.enabled_setting_at(clocks_state.cursor_pos());
      if ((pos == CHANNEL_SETTING_MASK_CV_SOURCE) || (pos == CHANNEL_SETTING_SEQ_MASK_CV_SOURCE))
        clocks_state.cursor.set_editing(false);
      selected.set_page(PARAMETERS);
      }
      break;
    case TEMPO:
      selected.set_page(CV_SOURCES);
      clocks_state.cursor = clocks_state.cursor_state;
    default:
      // don't get stuck:
      if (selected.enabled_setting_at(clocks_state.cursor_pos()) == CHANNEL_SETTING_RESET)
        clocks_state.cursor.set_editing(false);
      selected.set_page(CV_SOURCES);
      break;
  }
  selected.update_enabled_settings();
  clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void CLOCKS_rightButton() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];

  if (selected.get_page() == TEMPO) {
    selected.set_page(PARAMETERS);
    clocks_state.cursor = clocks_state.cursor_state;
    selected.update_enabled_settings();
    clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
    return;
  }

  switch (selected.enabled_setting_at(clocks_state.cursor_pos())) {

    case CHANNEL_SETTING_MASK1:
    case CHANNEL_SETTING_MASK2:
    case CHANNEL_SETTING_MASK3:
    case CHANNEL_SETTING_MASK4:
    {
      int pattern = selected.get_sequence();
      if (TU::Patterns::PATTERN_NONE != pattern) {
        clocks_state.pattern_editor.Edit(&selected, pattern, TRIGGERS);
      }
    }
     break;
    case CHANNEL_SETTING_MASK_CV:
    {
    // we just use pattern 1, for the time being:
      int pattern = TU::Patterns::PATTERN_USER_0;
      clocks_state.pattern_editor.Edit(&selected, pattern, PITCH);
    }
     break;
    case CHANNEL_SETTING_SEPARATOR:
    case CHANNEL_SETTING_DUMMY_EMPTY:
      break;
    default:
      clocks_state.cursor.toggle_editing();
      break;
  }

}

void CLOCKS_leftButton() {
  // sync:
  for (int i = 0; i < NUM_CHANNELS; ++i)
    clock_channel[i].sync();
}

void CLOCKS_leftButtonLong() {

  if (TU::ui.read_immediate(TU::CONTROL_BUTTON_UP)) {

    for (int i = 0; i < NUM_CHANNELS; ++i)
      clock_channel[i].InitDefaults();

    Clock_channel &selected = clock_channel[clocks_state.selected_channel];
    selected.set_page(PARAMETERS);
    selected.update_enabled_settings();
    clocks_state.cursor.set_editing(false);
    clocks_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
  }
}

void CLOCKS_upButtonLong() {

  if (!TU::ui.read_immediate(TU::CONTROL_BUTTON_L)) {

    Clock_channel &selected = clock_channel[clocks_state.selected_channel];
    // set all channels to internal ?
    if (selected.get_page() == TEMPO) {
      for (int i = 0; i < NUM_CHANNELS; ++i)
        clock_channel[i].set_clock_source(CHANNEL_TRIGGER_INTERNAL);
      // and clear outputs:
      TU::OUTPUTS::set(CLOCK_CHANNEL_1, OFF);
      TU::OUTPUTS::setState(CLOCK_CHANNEL_1, OFF);
      TU::OUTPUTS::set(CLOCK_CHANNEL_2, OFF);
      TU::OUTPUTS::setState(CLOCK_CHANNEL_2, OFF);
      TU::OUTPUTS::set(CLOCK_CHANNEL_3, OFF);
      TU::OUTPUTS::setState(CLOCK_CHANNEL_3, OFF);
      TU::OUTPUTS::set(CLOCK_CHANNEL_4, selected.get_zero(CLOCK_CHANNEL_4));
      TU::OUTPUTS::setState(CLOCK_CHANNEL_4, OFF);
      TU::OUTPUTS::set(CLOCK_CHANNEL_5, OFF);
      TU::OUTPUTS::setState(CLOCK_CHANNEL_5, OFF);
      TU::OUTPUTS::set(CLOCK_CHANNEL_6, OFF);
      TU::OUTPUTS::setState(CLOCK_CHANNEL_6, OFF);
    }
  }
}

void CLOCKS_downButtonLong() {

  Clock_channel &selected = clock_channel[clocks_state.selected_channel];

  if (selected.get_page() == CV_SOURCES)
    selected.clear_CV_mapping();
  else if (selected.get_page() == TEMPO)   {
    for (int i = 0; i < NUM_CHANNELS; ++i)
      clock_channel[i].set_clock_source(CHANNEL_TRIGGER_TR1);
    // and clear outputs:
    TU::OUTPUTS::set(CLOCK_CHANNEL_1, OFF);
    TU::OUTPUTS::setState(CLOCK_CHANNEL_1, OFF);
    TU::OUTPUTS::set(CLOCK_CHANNEL_2, OFF);
    TU::OUTPUTS::setState(CLOCK_CHANNEL_2, OFF);
    TU::OUTPUTS::set(CLOCK_CHANNEL_3, OFF);
    TU::OUTPUTS::setState(CLOCK_CHANNEL_3, OFF);
    TU::OUTPUTS::set(CLOCK_CHANNEL_4, selected.get_zero(CLOCK_CHANNEL_4));
    TU::OUTPUTS::setState(CLOCK_CHANNEL_4, OFF);
    TU::OUTPUTS::set(CLOCK_CHANNEL_5, OFF);
    TU::OUTPUTS::setState(CLOCK_CHANNEL_5, OFF);
    TU::OUTPUTS::set(CLOCK_CHANNEL_6, OFF);
    TU::OUTPUTS::setState(CLOCK_CHANNEL_6, OFF);
  }
}

void CLOCKS_menu() {

  menu::SixTitleBar::Draw();
  uint16_t int_clock_used_ = 0x0;
  bool global_div_ = TU::DigitalInputs::global_div_TR1();

  for (int i = 0, x = 0; i < NUM_CHANNELS; ++i, x += 21) {

    const Clock_channel &channel = clock_channel[i];
    int_clock_used_ += channel.get_clock_source() == CHANNEL_TRIGGER_INTERNAL ? 0x1 : 0x0;
    menu::SixTitleBar::SetColumn(i);
    graphics.print((char)('1' + i));
    graphics.movePrintPos(2, 0);
    //
    if (channel.get_clock_source() == CHANNEL_TRIGGER_INTERNAL)
      graphics.drawBitmap8(x + 17, 2, 4, TU::bitmap_indicator_4x8);
    //
    if (clocks_state.selected_channel == i && channel.get_mode() != CLOCKMODE::DAC && channel.get_page() != TEMPO && channel.get_display_clock() == _ONBEAT)
      menu::SixTitleBar::DrawGateIndicator(i, 0x10);
    // global division?
    if (global_div_ && !i)
      graphics.drawBitmap8(x, 2, 4, TU::bitmap_div_indicator_4x8);
  }

  const Clock_channel &channel = clock_channel[clocks_state.selected_channel];
  if (channel.get_page() != TEMPO)
    menu::SixTitleBar::Selected(clocks_state.selected_channel);

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(clocks_state.cursor);

  menu::SettingsListItem list_item;

  while (settings_list.available()) {
    const int setting =
    channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = Clock_channel::value_attr(setting);

    switch (setting) {
      case CHANNEL_SETTING_MODE:
        list_item.DrawValueMax(value, attr, channel.is_dac_channel() ? static_cast<int>(CLOCKMODE::DAC) : static_cast<int>(CLOCKMODE::BURST));
      break;

      case CHANNEL_SETTING_MASK1:
      case CHANNEL_SETTING_MASK2:
      case CHANNEL_SETTING_MASK3:
      case CHANNEL_SETTING_MASK4:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_display_mask(), channel.get_sequence_length(channel.get_display_sequence()), channel.get_clock_cnt());
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_MASK_CV:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_cv_display_mask(), channel.get_cv_sequence_length(), channel.get_clock_cnt());
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_SEPARATOR:
      case CHANNEL_SETTING_DUMMY_EMPTY:
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_SCREENSAVER:
        CLOCKS_screensaver();
        break;
      case CHANNEL_SETTING_INTERNAL_CLK:
        pending_int_frequency = value;
        if (int_clock_used_)
          list_item.DrawDefault(value, attr);
        break;
      case CHANNEL_SETTING_EUCLID_K:
      case CHANNEL_SETTING_EUCLID_OFFSET:
        list_item.DrawValueMax(value, attr, channel.euclid_n());
      break;
      case CHANNEL_SETTING_LFSR_TAP1:
      case CHANNEL_SETTING_LFSR_TAP2:
        list_item.DrawValueMax(value, attr, channel.get_turing_length());
      break;
      default:
        list_item.DrawDefault(value, attr);
        break;
    }
  }

  if (clocks_state.pattern_editor.active())
    clocks_state.pattern_editor.Draw();
}

void Clock_channel::RenderScreensaver(weegfx::coord_t start_x) const {

  // DAC needs special treatment:
  if (is_dac_channel() && get_mode() == CLOCKMODE::DAC) { // display DAC values, ish; x/y coordinates slightly off ...

    uint16_t _dac_value = TU::OUTPUTS::value(channel_);

    if (_dac_value < 2047) {
      // output negative
      _dac_value = 16 - (_dac_value >> 7);
      CONSTRAIN(_dac_value, 1, 16);
      graphics.drawFrame(start_x + 5 - (_dac_value >> 1), 41 - (_dac_value >> 1), _dac_value, _dac_value);
    }
    else {
      // positive output
      _dac_value = ((_dac_value - 2047) >> 7);
      CONSTRAIN(_dac_value, 1, 16);
      graphics.drawRect(start_x + 5 - (_dac_value >> 1), 41 - (_dac_value >> 1), _dac_value, _dac_value);
    }
    return;
  }

  // draw little square thingies ..
  if (gpio_state_ && display_state_ == _ACTIVE) {
    graphics.drawRect(start_x, 36, 10, 10);
    graphics.drawFrame(start_x - 2, 34, 14, 14);
  }
  else if (display_state_ == _ONBEAT)
    graphics.drawRect(start_x, 36, 10, 10);
  else
    graphics.drawRect(start_x + 3, 39, 4, 4);
}

void CLOCKS_screensaver() {
  clock_channel[0].RenderScreensaver(4);
  clock_channel[1].RenderScreensaver(26);
  clock_channel[2].RenderScreensaver(48);
  clock_channel[3].RenderScreensaver(70);
  clock_channel[4].RenderScreensaver(92);
  clock_channel[5].RenderScreensaver(114);
}
