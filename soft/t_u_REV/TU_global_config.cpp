// Copyright 2018 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
#include <Arduino.h>
#include "TU_global_config.h"
#include "TU_digital_inputs.h"
#include "TU_strings.h"
#include "src/util_misc.h"

static const uint8_t global_divisors[] {
  0,
  24,
  48,
  96
};

const char* const global_divisors_strings[] = {
  "-", "24PPQ", "48PPQ", "96PPQ"
};

SETTINGS_DECLARE(TU::GlobalConfig, TU::GLOBAL_CONFIG_SETTING_LAST) {
  { 0, 0, 3, "TR1 global div", global_divisors_strings, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "TR1 master", TU::Strings::no_yes, settings::STORAGE_TYPE_U4 },
};

namespace TU {

GlobalConfig global_config;

void GlobalConfig::Init()
{
  InitDefaults();
}

void GlobalConfig::Apply()
{
  SERIAL_PRINTLN("Global divisor, TR1: %i", global_divisors[global_div1()]);
  SERIAL_PRINTLN("TR1 master clock, TR1: %i", TR1_master());

  TU::DigitalInputs::set_global_div_TR1(global_divisors[global_div1()]);
  TU::DigitalInputs::set_master_clock(TR1_master());
}

void GlobalConfig::Reset()
{
  InitDefaults();
  Apply();
}

}; // namespace TU
