/* 
*
*  temps_utile. teensy 3.1 6x CLOCK generator / distributor 
*  compile with "120 MHz optimized (overclock)"
*
*/

// TD:
// - makedisplay - > *char / drawStr()
// - internal timer / multiply (?)
// - parameter limits (when changing mode)
// - sync();
// - clear();
// - global CV: pulsewidth, channel "scan".

// - test: changes made to spi4teensy / 30Mhz; clock_out moved to ISR; NVIC priority changed; check_buttons();

#include <spi4teensy3_14.h>
#include <u8g_teensy_14.h>
#include <rotaryplus.h>

/* clock outputs */
#define CLK1 29
#define CLK2 28
#define CLK3 10
#define CLK4 A14
#define CLK5 2
#define CLK6 9
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
/* aux buttons */
#define but_top 5
#define but_bot 4

U8GLIB u8g(&u8g_dev_sh1106_128x64_2x_hw_spi, u8g_com_hw_spi_fn);

enum encoders_ {
 LEFT,
 RIGHT 
};

Rotary encoder[2] = {
  {encL1, encL2}, 
  {encR1, encR2}
}; 

/*  menu variables */
volatile uint16_t display_clock;
extern uint16_t UI_MODE;
extern uint32_t LAST_UI;

#define TIMEOUT 6000 // screen saver

/*  -------------------------------------------------------------   */

/*       triggers + buttons ISRs    */

uint32_t LAST_BUT = 0;
const uint16_t DEBOUNCE = 200; 

enum _buttons {
  
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

void UI_timerCallback() 
{ 
  _UI = true; 
}  

void clk_ISR() 
{  
  if (!CLK_SRC) {
       output_clocks();
      _bpm = true; 
  }
} 


/*       ---------------------------------------------------------         */

IntervalTimer adc_timer;
#define ADC_RATE   10000   // CV
volatile uint8_t _adc; 
void adc_timerCallback() { _adc = true; }  
const uint8_t numADC = 4;
const uint16_t HALFSCALE = 512;
uint16_t CV[numADC+1];
const uint16_t THRESHOLD = 400;

/*       ---------------------------------------------------------         */

void setup(){

  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  spi4teensy3::init();
  analogWriteResolution(12);
  
  // w/ external pullups; otherwise use INPUT_PULLUP
  pinMode(butL, INPUT);
  pinMode(butR, INPUT);
  pinMode(but_top, INPUT);
  pinMode(but_bot, INPUT);
 
  pinMode(TR1, INPUT);
  pinMode(TR2, INPUT);
  
  pinMode(CLK1, OUTPUT);
  pinMode(CLK2, OUTPUT);
  pinMode(CLK3, OUTPUT);
  // CLK4 = A14 
  pinMode(CLK5, OUTPUT);
  pinMode(CLK6, OUTPUT);
  /* ext clock ISR */
  attachInterrupt(TR1, clk_ISR, FALLING);
   /* encoder ISR */
  attachInterrupt(encL1, left_encoder_ISR, CHANGE);
  attachInterrupt(encL2, left_encoder_ISR, CHANGE);
  attachInterrupt(encR1, right_encoder_ISR, CHANGE);
  attachInterrupt(encR2, right_encoder_ISR, CHANGE);
  /* splash */
  hello();  
  delay(1250);
  /* timers */
  adc_timer.begin(adc_timerCallback, ADC_RATE);
  UI_timer.begin(UI_timerCallback, UI_RATE);
  randomSeed(analogRead(A15)+analogRead(A16)+analogRead(A19)+analogRead(A20));
  /* calibrate ? */
  if (!digitalRead(butL))  calibrate_main();
  /* init */
  init_clocks();
  init_menu();  
  //CORETIMER.priority(32);
  //CORETIMER.begin(BPM_ISR, BPM_MICROSEC); 
}

/*       ---------------------------------------------------------         */ 

void loop()
{
  
  while(1) {
  
    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks();} 
    /* update UI ?*/
    UI();
  
    coretimer();
    if (_bpm) {   _bpm = 0;  next_clocks(); } 
    /* update encoders ?*/
    if (millis() - LAST_BUT > DEBOUNCE) update_ENC();
  
    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks(); }  
    /* update CV ?*/
    if (_adc) { _adc = 0; CV[1] = analogRead(CV4); CV[2] = analogRead(CV3); CV[3] = analogRead(CV1); CV[4] = analogRead(CV2);} 
  
    coretimer();  
    if (_bpm) {   _bpm = 0; next_clocks(); }  
    /* buttons ?*/
    if (_UI) { leftButton(); rightButton(); topButton(); lowerButton(); _UI = false; }

    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks(); }  
    /* timeout ?*/
    if (UI_MODE && (millis() - LAST_UI > TIMEOUT)) time_out(); // timeout UI
 
    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks(); }  
    /* clocks off ?*/
    if (display_clock) clocksoff();
  
    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks(); } 
    /* do something ?*/
    if (button_states[BOTTOM]) checkbuttons(BOTTOM);
  
    coretimer();
    if (_bpm) {   _bpm = 0; next_clocks(); } 
    /* do something ?*/
    if (button_states[TOP]) checkbuttons(TOP);
  }
}



