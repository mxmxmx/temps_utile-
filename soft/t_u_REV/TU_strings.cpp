#include "TU_strings.h"


namespace TU {

  namespace Strings {

  const char * const trigger_input_names[] = { "TR1", "TR2", "TR3", "TR4" };

  const char * const cv_input_names[] = { "CV1", "CV2", "CV3", "CV4" };

  const char * const no_yes[] = { "No", "Yes" };

  const char * const operators[] = { "AND", "OR", "XOR", "XNOR", "NAND", "NOR" };
  
  const char * const channel_id[] = { "ch#1", "ch#2", "ch#3", "ch#4", "ch#5", "ch#6"};

  const char * const mode[] = { "MULT", "LFSR", "RAND", "EUCL", "LOGIC", "SEQ", "DAC" };

  const char * const dac_modes[] = { "BIN", "RAND", "T_M", "LSTC"};
  
  const char * const logic_tracking[] = { "state", "P_W"};

  const char * const binary_tracking[] = { "P_W", "state"};

  };
};
