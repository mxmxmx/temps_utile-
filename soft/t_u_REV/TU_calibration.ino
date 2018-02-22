/*
*
* calibration menu:
*
* enter by pressing left encoder button during start up; use encoder switches to navigate.
*
*/

#include "TU_calibration.h"
#include "TU_options.h"

using TU::OUTPUTS;

#ifdef MOD_OFFSET
  static constexpr uint16_t _DAC_OFFSET = 30; // DAC offset, initial approx., ish --> 0v
#else
  #ifdef MODEL_2TT
  static constexpr uint16_t _DAC_OFFSET = 30; // DAC offset, initial approx., ish --> 0v
  #else
  static constexpr uint16_t _DAC_OFFSET = 2200; // DAC offset, initial approx., ish --> 0v
  #endif
#endif

#ifdef MODEL_2TT
static constexpr uint16_t _ADC_OFFSET = (uint16_t)((float)pow(2,TU::ADC::kAdcResolution)*1.0f); // ADC offset
#else
static constexpr uint16_t _ADC_OFFSET = (uint16_t)((float)pow(2,TU::ADC::kAdcResolution)*0.5f); // ADC offset
#endif

namespace TU {
CalibrationStorage calibration_storage;
CalibrationData calibration_data;
};

static constexpr unsigned kCalibrationAdcSmoothing = 4;


const TU::CalibrationData kCalibrationDefaults = {
  // DAC
  { {
    #ifdef MODEL_2TT
    {572, 1432, 2288, 3146, 4095}   // 
    #else
    {514, 1375, 2236, 3097, 3960}   //  1.0V/oct ==> 1 octave ~ 430
    #endif
    },
  },
  // ADC
  { { _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET },
    0,  // pitch_cv_scale
    0   // pitch_cv_offset : unused
  },
  // display_offset
  SH1106_128x64_Driver::kDefaultOffset,
  TU_CALIBRATION_DEFAULT_FLAGS,
  0, 0 // reserved
};

void calibration_reset() {
  memcpy(&TU::calibration_data, &kCalibrationDefaults, sizeof(TU::calibration_data));
}

void calibration_load() {
  SERIAL_PRINTLN("CalibrationStorage: PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                 TU::CalibrationStorage::PAGESIZE, TU::CalibrationStorage::PAGES, TU::CalibrationStorage::LENGTH);

  calibration_reset();
  if (!TU::calibration_storage.Load(TU::calibration_data)) {
#ifdef CALIBRATION_LOAD_LEGACY
    if (EEPROM.read(0x2) > 0) {
      SERIAL_PRINTLN("Calibration not loaded, non-zero data found, trying to import...");
      calibration_read_old();
    } else {
      SERIAL_PRINTLN("No calibration data found, using defaults");
    }
#else
    SERIAL_PRINTLN("No calibration data found, using defaults");
#endif
  } else {
    SERIAL_PRINTLN("Calibration data loaded...");
  }
}

void calibration_save() {
  SERIAL_PRINTLN("Saving calibration data...");
  TU::calibration_storage.Save(TU::calibration_data);
}

enum CALIBRATION_STEP {  
  HELLO,
  CENTER_DISPLAY,
  DAC_4VM, DAC_2VM, DAC_ZERO, DAC_2V, DAC_4V,
  CV_OFFSET_0, CV_OFFSET_1, CV_OFFSET_2, CV_OFFSET_3,
  CALIBRATION_EXIT,
  CALIBRATION_STEP_LAST,
  CALIBRATION_STEP_FINAL
};  

enum CALIBRATION_TYPE {
  CALIBRATE_NONE,
  CALIBRATE_DAC_OUTPUT,
  CALIBRATE_ADC_OFFSET,
  CALIBRATE_DISPLAY
};

struct CalibrationStep {
  CALIBRATION_STEP step;
  const char *title;
  const char *message;
  const char *help; // optional
  const char *footer;

  CALIBRATION_TYPE calibration_type;
  int index;

  const char * const *value_str; // if non-null, use these instead of encoder value
  int min, max;
};


struct CalibrationState {
  CALIBRATION_STEP step;
  const CalibrationStep *current_step;
  int encoder_value;

  SmoothedValue<uint32_t, kCalibrationAdcSmoothing> adc_sum;
};

TU::DigitalInputDisplay digital_input_displays[2];

// 128/6=21                  |                     |
const char *start_footer   = "              [START]";
const char *end_footer     = "[PREV]         [EXIT]";
const char *default_footer = "[PREV]         [NEXT]";
const char *default_help_r = "[R] => Adjust";
const char *select_help    = "[R] => Select";

const CalibrationStep calibration_steps[CALIBRATION_STEP_LAST] = {
  { HELLO, "T_U calibration", "use defaults? ", select_help, start_footer, CALIBRATE_NONE, 0, TU::Strings::no_yes, 0, 1 },
  { CENTER_DISPLAY, "center display", "pixel offset ", default_help_r, default_footer, CALIBRATE_DISPLAY, 0, nullptr, 0, 2 },
  #ifdef MOD_OFFSET
  { DAC_4VM, "DAC -2.0 volts", "--> -2.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 0, nullptr, 0, OUTPUTS::MAX_VALUE },
  { DAC_2VM, "DAC  0.0 volts", "-->  0.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 1, nullptr, 0, OUTPUTS::MAX_VALUE },
  { DAC_ZERO,"DAC +2.0 volts", "--> +2.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 2, nullptr, 0, OUTPUTS::MAX_VALUE },
  { DAC_2V,  "DAC +4.0 volts", "--> +4.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 3, nullptr, 0, OUTPUTS::MAX_VALUE },
  { DAC_4V,  "DAC +6.0 volts", "--> +6.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 4, nullptr, 0, OUTPUTS::MAX_VALUE },
  #else
    #ifdef MODEL_2TT
    { DAC_4VM, "DAC  0.0 volts", "-->  0.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 0, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_2VM, "DAC +2.0 volts", "--> +2.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 1, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_ZERO,"DAC +4.0 volts", "--> +4.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 2, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_2V,  "DAC +6.0 volts", "--> +6.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 3, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_4V,  "DAC +8.0 volts", "--> +8.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 4, nullptr, 0, OUTPUTS::MAX_VALUE },
    #else
    { DAC_4VM, "DAC -4.0 volts", "--> -4.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 0, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_2VM, "DAC -2.0 volts", "--> -2.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 1, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_ZERO,"DAC  0.0 volts", "-->  0.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 2, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_2V,  "DAC +2.0 volts", "--> +2.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 3, nullptr, 0, OUTPUTS::MAX_VALUE },
    { DAC_4V,  "DAC +4.0 volts", "--> +4.0V ", default_help_r, default_footer, CALIBRATE_DAC_OUTPUT, 4, nullptr, 0, OUTPUTS::MAX_VALUE },
    #endif
  #endif
  { CV_OFFSET_0, "ADC CV1", "--> 0V", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_1, nullptr, 0, 4095 },
  { CV_OFFSET_1, "ADC CV2", "--> 0V", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_2, nullptr, 0, 4095 },
  { CV_OFFSET_2, "ADC CV3", "--> 0V", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_3, nullptr, 0, 4095 },
  { CV_OFFSET_3, "ADC CV4", "--> 0V", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_4, nullptr, 0, 4095 },
  { CALIBRATION_EXIT, "Calibration complete", "Save values? ", select_help, end_footer, CALIBRATE_NONE, 0, TU::Strings::no_yes, 0, 1 }
};

/*     loop calibration menu until done       */
void TU::Ui::Calibrate() {

  // Calibration data should be loaded (or defaults) by now
  SERIAL_PRINTLN("Starting calibration...");

  CalibrationState calibration_state = {
    HELLO,
    &calibration_steps[HELLO],
    1,
  };
  calibration_state.adc_sum.set(_ADC_OFFSET);

  for (auto &did : digital_input_displays)
    did.Init();

  TickCount tick_count;
  tick_count.Init();

  encoder_enable_acceleration(CONTROL_ENCODER_R, true);

  bool calibration_complete = false;
  while (!calibration_complete) {

    uint32_t ticks = tick_count.Update();
    digital_input_displays[0].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_1>());
    digital_input_displays[1].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_2>());

    while (event_queue_.available()) {
      const UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;

      switch (event.control) {
        case CONTROL_BUTTON_L:
          if (calibration_state.step > CENTER_DISPLAY)
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step - 1);
          break;
        case CONTROL_BUTTON_R:
          // Special case these values to read, before moving to next step
          if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
            switch (calibration_state.current_step->step) {
              default: break;
            }
          }
          if (calibration_state.step < CALIBRATION_EXIT)
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step + 1);
          else
            calibration_complete = true;
          break;
        case CONTROL_ENCODER_L:
          if (calibration_state.step > HELLO) {
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step + event.value);
            CONSTRAIN(calibration_state.step, CENTER_DISPLAY, CALIBRATION_EXIT);
          }
          break;
        case CONTROL_ENCODER_R:
          calibration_state.encoder_value += event.value;
          break;
        case CONTROL_BUTTON_DOWN:
        case CONTROL_BUTTON_UP:
        configure_encoders(calibration_data.next_encoder_config());
        break;
        default:
          break;
      }
    }

    const CalibrationStep *next_step = &calibration_steps[calibration_state.step];
    if (next_step != calibration_state.current_step) {
      SERIAL_PRINTLN("Step: %s (%d)", next_step->title, next_step->step);
      // Special cases on exit current step
      switch (calibration_state.current_step->step) {
        case HELLO:
          if (calibration_state.encoder_value) {
            SERIAL_PRINTLN("Resetting to defaults...");
            calibration_reset();
          }
          break;
        
        default: break;
      }

      // Setup next step
      switch (next_step->calibration_type) {
      case CALIBRATE_DAC_OUTPUT:
        calibration_state.encoder_value = TU::calibration_data.dac.calibration_points[0x0][next_step->index];
        break;
      case CALIBRATE_ADC_OFFSET:
        calibration_state.encoder_value = TU::calibration_data.adc.offset[next_step->index];
        break;
      case CALIBRATE_DISPLAY:
        calibration_state.encoder_value = TU::calibration_data.display_offset;
        break;

      case CALIBRATE_NONE:
      default:
        if (CALIBRATION_EXIT != next_step->step)
          calibration_state.encoder_value = 0;
        else
          calibration_state.encoder_value = 1;
      }
      calibration_state.current_step = next_step;
    }

    calibration_update(calibration_state);
    calibration_draw(calibration_state);
  }

  if (calibration_state.encoder_value) {
    SERIAL_PRINTLN("Calibration complete");
    calibration_save();
  } else {
    SERIAL_PRINTLN("Calibration complete, not saving values...");
  }
}

void calibration_draw(const CalibrationState &state) {
  GRAPHICS_BEGIN_FRAME(true);
  const CalibrationStep *step = state.current_step;

  menu::DefaultTitleBar::Draw();
  graphics.print(step->title);

  weegfx::coord_t y = menu::CalcLineY(0);

  static constexpr weegfx::coord_t kValueX = menu::kDisplayWidth - 30;

  graphics.setPrintPos(menu::kIndentDx, y + 2);
  switch (step->calibration_type) {
    
    case CALIBRATE_DAC_OUTPUT:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.print((int)state.encoder_value, 5);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      break;

    case CALIBRATE_ADC_OFFSET:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.print((int)TU::ADC::value(static_cast<ADC_CHANNEL>(step->index)), 5);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      break;

    case CALIBRATE_DISPLAY:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.pretty_print((int)state.encoder_value, 2);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      graphics.drawFrame(0, 0, 128, 64);
      break;
      
    case CALIBRATE_NONE:
    default:
      graphics.setPrintPos(menu::kIndentDx, y + 2);
      graphics.print(step->message);
      if (step->value_str)
        graphics.print(step->value_str[state.encoder_value]);
      break;
  }

  y += menu::kMenuLineH;
  graphics.setPrintPos(menu::kIndentDx, y + 2);
  if (step->help)
    graphics.print(step->help);

  weegfx::coord_t x = menu::kDisplayWidth - 22;
  y = 2;
  for (int input = TU::DIGITAL_INPUT_1; input < TU::DIGITAL_INPUT_LAST; ++input) {
    uint8_t state = (digital_input_displays[input].getState() + 3) >> 2;
    if (state)
      graphics.drawBitmap8(x, y, 4, TU::bitmap_gate_indicators_8 + (state << 2));
    x += 5;
  }

  graphics.drawStr(1, menu::kDisplayHeight - menu::kFontHeight - 3, step->footer);

  static constexpr uint16_t step_width = (menu::kDisplayWidth << 8 ) / (CALIBRATION_STEP_LAST - 1);
  graphics.drawRect(0, menu::kDisplayHeight - 2, (state.step * step_width) >> 8, 2);

  GRAPHICS_END_FRAME();
}

/* DAC output etc */ 

void calibration_update(CalibrationState &state) {

  CONSTRAIN(state.encoder_value, state.current_step->min, state.current_step->max);
  const CalibrationStep *step = state.current_step;

  switch (step->calibration_type) {
    case CALIBRATE_NONE:
      OUTPUTS::zero_all();
      break;
    case CALIBRATE_DAC_OUTPUT:
      TU::calibration_data.dac.calibration_points[0x0][step->index] = state.encoder_value;
      OUTPUTS::set(CLOCK_CHANNEL_4, state.encoder_value);
      break;
    case CALIBRATE_ADC_OFFSET:
      TU::calibration_data.adc.offset[step->index] = state.encoder_value;
      OUTPUTS::zero_all();
      break;
    case CALIBRATE_DISPLAY:
      TU::calibration_data.display_offset = state.encoder_value;
      display::AdjustOffset(TU::calibration_data.display_offset);
      break;
  }
}

/* misc */ 

uint32_t adc_average() {
  delay(TU_CORE_TIMER_RATE + 1);

  return
    TU::ADC::smoothed_raw_value(ADC_CHANNEL_1) + TU::ADC::smoothed_raw_value(ADC_CHANNEL_2) +
    TU::ADC::smoothed_raw_value(ADC_CHANNEL_3) + TU::ADC::smoothed_raw_value(ADC_CHANNEL_4);
}

