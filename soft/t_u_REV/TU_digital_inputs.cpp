#include <Arduino.h>
#include <algorithm>
#include "TU_digital_inputs.h"
#include "TU_gpio.h"

/*static*/
volatile uint32_t TU::DigitalInputs::clocked_[DIGITAL_INPUT_LAST];

void FASTRUN tr1_ISR() {  
  TU::DigitalInputs::clock<TU::DIGITAL_INPUT_1>();
}  // main clock

void FASTRUN tr2_ISR() {
  TU::DigitalInputs::clock<TU::DIGITAL_INPUT_2>();
}

/*static*/
void TU::DigitalInputs::Init() {

  static const struct {
    uint8_t pin;
    void (*isr_fn)();
  } pins[DIGITAL_INPUT_LAST] =  {
    {TR1, tr1_ISR},
    {TR2, tr2_ISR},
  };

  for (auto pin : pins) {
    pinMode(pin.pin, TU_GPIO_TRx_PINMODE);
    attachInterrupt(pin.pin, pin.isr_fn, FALLING);
  }

  std::fill(clocked_, clocked_ + DIGITAL_INPUT_LAST, 0);
}
