#include "TU_strings.h"


namespace TU {

  namespace Strings {

  const char * const trigger_input_names[4] = { "TR1", "TR2", "TR3", "TR4" };

  const char * const cv_input_names[4] = { "CV1", "CV2", "CV3", "CV4" };

  const char * const no_yes[] = { "No", "Yes" };

  const char * const operators[5] = { "AND", "OR", "XOR", "NAND", "NOR" };
  
  const char * const channel_id[6] = { "ch#1", "ch#2", "ch#3", "ch#4", "ch#5", "ch#6"};

  const char * const mode[6] = { "LFSR", "RAND", "EUCL", "LOGIC", "SEQ", "DAC" };

  const char * const dac_modes[4] = { "BIN", "RAND", "TM", "LSTC"};

  };
};
