#include "TU_strings.h"


namespace TU {

  namespace Strings {

  const char * const seq_playmodes[] = {" - ", "SEQ+1", "SEQ+2", "SEQ+3", "TR2+1", "TR2+2", "TR2+3", "SH-1", "SH-2", "SH-3", "SH-4"};  

  const char * const cv_seq_playmodes[] = {"fwd", "rev", "pnd1", "pdn2", "rnd", "ARP"};  
    
  const char * const seq_id[] = { "--> #1", "--> #2", "--> #3", "--> #4", "#1", "#2", "#3", "#4"};

  const char * const trigger_input_names[] = { "TR1", "TR2", "TR3", "TR4" };

  const char * const cv_input_names[] = { "CV1", "CV2", "CV3", "CV4" };

  const char * const no_yes[] = { "no", "yes" };

  const char * const operators[] = { "AND", "OR", "XOR", "XNOR", "NAND", "NOR" };
  
  const char * const channel_id[] = { "ch#1", "ch#2", "ch#3", "ch#4", "ch#5", "ch#6"};

  const char * const mode[] = { "MULT", "LFSR", "RAND", "EUCL", "LOGIC", "SEQ", "BURST", "DAC" };

  const char * const dac_modes[] = { "BIN", "RAND", "T_M", "LGTC", "SEQ"};
  
  const char * const logic_tracking[] = { "state", "P_W"};

  const char * const binary_tracking[] = { "P_W", "state"};

  const char * const encoder_config_strings[] = { "normal", "R reversed", "L reversed", "LR reversed" };

  const char * const trigger_delay_times[kNumDelayTimes] = {
    "off", "120us", "240us", "360us", "480us", "1ms", "2ms", "4ms"
  };

  const char * const pulsewidth_ms[] = {
    "echo",
    "1ms","2ms","3ms","4ms","5ms","6ms","7ms","8ms","9ms","10ms",
    "11ms","12ms","13ms","14ms","15ms","16ms","17ms","18ms","19ms","20ms",
    "21ms","22ms","23ms","24ms","25ms","26ms","27ms","28ms","29ms","30ms",
    "31ms","32ms","33ms","34ms","35ms","36ms","37ms","38ms","39ms","40ms",
    "41ms","42ms","43ms","44ms","45ms","46ms","47ms","48ms","49ms","50ms",
    "51ms","52ms","53ms","54ms","55ms","56ms","57ms","58ms","59ms","60ms",
    "61ms","62ms","63ms","64ms","65ms","66ms","67ms","68ms","69ms","70ms",
    "71ms","72ms","73ms","74ms","75ms","76ms","77ms","78ms","79ms","80ms",
    "81ms","82ms","83ms","84ms","85ms","86ms","87ms","88ms","89ms","90ms",
    "91ms","92ms","93ms","94ms","95ms","96ms","97ms","98ms","99ms","100ms",
    "101ms","102ms","103ms","104ms","105ms","106ms","107ms","108ms","109ms","110ms",
    "111ms","112ms","113ms","114ms","115ms","116ms","117ms","118ms","119ms","120ms",
    "121ms","122ms","123ms","124ms","125ms","126ms","127ms","128ms","129ms","130ms",
    "131ms","132ms","133ms","134ms","135ms","136ms","137ms","138ms","139ms","140ms",
    "141ms","142ms","143ms","144ms","145ms","146ms","147ms","148ms","149ms","150ms",
    "151ms","152ms","153ms","154ms","155ms","156ms","157ms","158ms","159ms","160ms",
    "161ms","162ms","163ms","164ms","165ms","166ms","167ms","168ms","169ms","170ms",
    "171ms","172ms","173ms","174ms","175ms","176ms","177ms","178ms","179ms","180ms",
    "181ms","182ms","183ms","184ms","185ms","186ms","187ms","188ms","189ms","190ms",
    "191ms","192ms","193ms","194ms","195ms","196ms","197ms","198ms","199ms","200ms",
    "201ms","202ms","203ms","204ms","205ms","206ms","207ms","208ms","209ms","210ms",
    "211ms","212ms","213ms","214ms","215ms","216ms","217ms","218ms","219ms","220ms",
    "221ms","222ms","223ms","224ms","225ms","226ms","227ms","228ms","229ms","230ms",
    "231ms","232ms","233ms","234ms","235ms","236ms","237ms","238ms","239ms","240ms",
    "241ms","242ms","243ms","244ms","245ms","246ms","247ms","248ms","249ms","250ms",
    "251ms","252ms","253ms","254ms",
    "50%"
    };
  };

  // \sa TU_config.h -> kMaxTriggerDelayTicks
  const uint8_t trigger_delay_ticks[kNumDelayTimes] = {
    0, 2, 4, 6, 8, 16, 33, 66
  };
};
