/*
*
*  temps_utile. teensy 3.1 6x CLOCK generator / distributor
*  compile with "120 MHz optimized (overclock)"
*
*/

// TD:
// - internal timer / multiply (?)
// - expand sync();  (what should this even do, other than reset counter / DIV mode?)
// - clear(); (?)
// - global CV: pulsewidth, channel "scan".

#include <spi4teensy3_14.h>
#include <u8g_teensy_14.h>
#include <rotaryplus.h>
#include <EEPROM.h>
#include "pagestorage.h"

#define _TEMPS_UTILE_REV // uncomment for pre-rev 1 boards 
#define _SH1106          // uncomment for use with SSD1306 / pixel offset issues

/* clock outputs, buttons */
#ifdef _TEMPS_UTILE_REV
 #define CLK1 5
 #define CLK2 6
 #define CLK3 7
 #define CLK4 A14 // alt = 29
 #define CLK5 8
 #define CLK6 2
 #define but_top 3
 #define but_bot 12
 #define _PULL_UP 0x1
#else
 #define CLK1 29
 #define CLK2 28
 #define CLK3 10
 #define CLK4 A14
 #define CLK5 2
 #define CLK6 9
 #define but_top 5
 #define but_bot 4
 #define _PULL_UP 0x0
#endif 

/* CV inputs */
#define CV1 A5
#define CV2 A4
#define CV3 A6
#define CV4 A3
/* clock inputs */
#define TR1 0 // main
#define TR2 1 // reset
/* encoders */
#define encR1 15
#define encR2 16
#define butR 13
#define encL1 22
#define encL2 21
#define butL 23


#ifdef _SH1106
  U8GLIB u8g(&u8g_dev_sh1106_128x64_2x_hw_spi, u8g_com_hw_spi_fn);
#else
  U8GLIB u8g(&u8g_dev_ssd1306_128x64_2x_hw_spi, u8g_com_hw_spi_fn);
#endif

enum encoders_ 
{
  LEFT,
  RIGHT
};

Rotary encoder[2] = 
{
  {encL1, encL2},
  {encR1, encR2}
};

/*  menu variables */
volatile uint16_t display_clock;
extern uint16_t UI_MODE;
extern uint32_t LAST_UI;

#define TIMEOUT 6000 // screen saver

const uint32_t _FCPU = F_CPU / 1000;

/*  -------------------------------------------------------------   */

/*       triggers + buttons ISRs    */

uint32_t LAST_BUT = 0;
const uint16_t DEBOUNCE = 200;

enum _buttons 
{
  TOP,
  BOTTOM
};

IntervalTimer UI_timer;
#define UI_RATE   10000
uint16_t volatile _UI;
uint16_t volatile _bpm;
uint16_t volatile _reset;
extern uint16_t button_states[];
extern volatile uint16_t CLK_SRC; // internal / external
extern volatile uint16_t _OK;     // ext. clock ok?
// Used by settings
extern uint16_t CV_DEST_CHANNEL[];
extern int16_t CV_DEST_PARAM[];

void UI_timerCallback()
{
  _UI = true;
}


/* ------------------------------------------------------------------   */

volatile uint32_t TIME_STAMP = 0;   // systick
volatile uint32_t PREV_TIME_STAMP = 0;
uint32_t PW = 0;      // ext. clock interval
uint32_t PREV_PW = 0; // ext. clock interval

void FASTRUN clk_ISR() 
{  
  // systick / pre-empt going too fast
  TIME_STAMP = ARM_DWT_CYCCNT;     
  PW = TIME_STAMP - PREV_TIME_STAMP; // / _FCPU; 
  PREV_TIME_STAMP = TIME_STAMP;
  
  if (!CLK_SRC && _OK) {
       output_clocks();
      _bpm = true; 
  }
} 

/*       ---------------------------------------------------------         */

IntervalTimer adc_timer;
#define ADC_RATE   10000   // CV
volatile uint8_t _adc;
void adc_timerCallback() {
  _adc = true;
}
const uint8_t numADC = 4;
const uint16_t HALFSCALE = 512;
uint16_t CV[numADC + 1];
const uint16_t THRESHOLD = 400;


/*       ---------------------------------------------------------         */

// To save space (and increase the number of write cycles) this only stores
// the current mode's parameters; the rest will be set to defaults on restart
struct channel_settings {
  uint8_t mode;
  uint16_t param[4]; // match per-channel part of struct params::param
  uint8_t cvmod[5]; // must match struct params::cvmod
} __attribute__((packed)); // 1 + 8 + 5 = 14

// Saved settings
struct settings_data {
  // If contents of this struct changes, modify this identifier
  static const uint32_t FOURCC = FOURCC<'T', 'U', 1, 1>::value;

  uint16_t cv_dest_channel[5]; // See menu.ino
  int16_t cv_dest_param[5];
  uint8_t clk_src;
  uint16_t bpm;
  uint8_t bpm_sel; // note: defined as uint16_t

  channel_settings channels[6];
};

/* Define a storage implemenation using EEPROM */
struct EEPROMStorage {
  static const size_t LENGTH = 2048;

  static void write(size_t addr, const void *data, size_t length) {
    EEPtr e = addr;
    const uint8_t *src = (const uint8_t*)data;
    while (length--)
      (*e++).update(*src++);
  }

  static void read(size_t addr, void *data, size_t length) {
    EEPtr e = addr;
    uint8_t *dst = (uint8_t*)data;
    while (length--)
      *dst++ = *e++;
  }
};

static settings_data settings;
static PageStorage<EEPROMStorage, 0, 128, struct settings_data> storage;
// sizeof(settings_data) with overhead is just < 128 which gives 16 pages, which
// effectively increases write cycles x16. Since the save is triggered on TIMEOUT
// (6s) so this should be Good Enough (tm)

/* ------------------------------------------------------------------   */
void save_settings() {

  memcpy(settings.cv_dest_channel, CV_DEST_CHANNEL, sizeof(settings.cv_dest_channel));
  memcpy(settings.cv_dest_param, CV_DEST_PARAM, sizeof(settings.cv_dest_param));

  clocks_store(&settings);

  if (storage.save( settings)) {
    //Serial.print("Saved settings to page ");
    //Serial.print(storage.page_index());
    //Serial.println(" ");
  }
}

/* ------------------------------------------------------------------   */
void load_settings() {
  if (storage.load(settings)) {

    Serial.print("Restoring settings from page ");
    Serial.print(storage.page_index());
    Serial.println(" ");

    CLK_SRC = settings.clk_src;
    memcpy(CV_DEST_CHANNEL, settings.cv_dest_channel, sizeof(settings.cv_dest_channel));
    memcpy(CV_DEST_PARAM, settings.cv_dest_param, sizeof(settings.cv_dest_param));
    clocks_restore(&settings);
  }
}

/*       ---------------------------------------------------------         */

void setup() {

  // display ... needs some time
  delay(10);
  // rev0 w/ external pullups; rev1 use INPUT_PULLUP
  if (_PULL_UP) {
    // buttons
    pinMode(butL, INPUT_PULLUP);
    pinMode(butR, INPUT_PULLUP);
    // clock inputs
    pinMode(TR1, INPUT_PULLUP);
    pinMode(TR2, INPUT_PULLUP);
  }
  else {
    // buttons
    pinMode(butL, INPUT);
    pinMode(butR, INPUT);
    // clock inputs
    pinMode(TR1, INPUT);
    pinMode(TR2, INPUT);
  };

  pinMode(but_top, INPUT_PULLUP);
  pinMode(but_bot, INPUT_PULLUP);
  pinMode(CLK1, OUTPUT);
  pinMode(CLK2, OUTPUT);
  pinMode(CLK3, OUTPUT);
  // CLK4 = DAC/A14
  pinMode(CLK5, OUTPUT);
  pinMode(CLK6, OUTPUT);
  // ext clock ISR 
  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  attachInterrupt(TR1, clk_ISR, FALLING);
  // encoder ISRs 
  attachInterrupt(encL1, left_encoder_ISR, CHANGE);
  attachInterrupt(encL2, left_encoder_ISR, CHANGE);
  attachInterrupt(encR1, right_encoder_ISR, CHANGE);
  attachInterrupt(encR2, right_encoder_ISR, CHANGE);
  // ADC
  analogWriteResolution(12);
  // splash ... 
  hello();
  delay(1250);
   // systick
  ARM_DEMCR |= ARM_DEMCR_TRCENA;  
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  // timers
  adc_timer.begin(adc_timerCallback, ADC_RATE);
  UI_timer.begin(UI_timerCallback, UI_RATE);
  randomSeed(analogRead(A15) + analogRead(A16) + analogRead(A19) + analogRead(A20));
  // calibrate ? 
  if (!digitalRead(butL))  calibrate_main();
  // init 
  init_clocks();
  init_menu();
  //CORETIMER.priority(32);
  //CORETIMER.begin(BPM_ISR, BPM_MICROSEC);
  load_settings();
}

/*       --------------------- main loop --------------------------         */

void loop()
{
  while (1)
  {  
     _loop();
  }
}



