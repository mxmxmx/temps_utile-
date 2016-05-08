
#include <spififo.h>
#include "TU_outputs.h"
#include "TU_gpio.h"

namespace TU {

/*static*/
void OUTPUTS::Init(CalibrationData *calibration_data) {

  pinMode(CLK1, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK2, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK3, TU_GPIO_OUTPUTS_PINMODE);
  // CLK4 == DAC
  pinMode(CLK5, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK6, TU_GPIO_OUTPUTS_PINMODE);

  calibration_data_ = calibration_data;
  history_tail_ = 0;
  memset(history_, 0, sizeof(uint16_t) * kHistoryDepth * NUM_DACS);

}

/*static*/
OUTPUTS::CalibrationData *OUTPUTS::calibration_data_ = nullptr;
/*static*/
uint32_t OUTPUTS::values_[CLOCK_CHANNEL_LAST];
/*static*/
uint16_t OUTPUTS::history_[NUM_DACS][OUTPUTS::kHistoryDepth];
/*static*/ 
volatile size_t OUTPUTS::history_tail_;

}; // namespace TU

const uint16_t _MAX_VALUE = 4095;

void set_Output1(uint8_t data) {
  digitalWriteFast(CLK1, data);
}

void set_Output2(uint8_t data) {
  digitalWriteFast(CLK2, data);
}

void set_Output3(uint8_t data) {
  digitalWriteFast(CLK3, data);
}


void set_Output4(uint16_t data) {

  uint16_t _data = data > _MAX_VALUE ? _MAX_VALUE : data;
  #if defined(__MK20DX256__)
    SIM_SCGC2 |= SIM_SCGC2_DAC0;
    DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // 3.3V VDDA = DACREF_2
    *(int16_t *)&(DAC0_DAT0L) = _data;
  #endif
}

void set_Output5(uint8_t data) {
  digitalWriteFast(CLK5, data);
}

void set_Output6(uint8_t data) {
  digitalWriteFast(CLK6, data);
}

