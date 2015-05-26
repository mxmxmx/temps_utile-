/* 
*
*  temps_utile. teensy 3.1 6x CLOCK generator / distributor 
*  run at 120 MHz
*
*/

// TD: 
// - parameter limits
// - test: sync();
// - clear();
// - global CV: pw, channel scan.

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
volatile uint8_t display_clock;
extern uint8_t UI_MODE;
extern uint32_t LAST_UI;

#define TIMEOUT 6000 // screen saver

/*  -------------------------------------------------------------   */

/*       triggers + buttons ISRs    */

uint32_t LAST_BUT = 0;
const uint8_t DEBOUNCE = 200; 

enum _buttons {
  
  TOP,
  BOTTOM
  
};

IntervalTimer UI_timer;
#define UI_RATE   10000
volatile uint8_t _UI; 
void UI_timerCallback() { _UI = true; }  
uint8_t volatile _bpm;
uint8_t volatile _reset;
extern uint8_t b_state[];

void clk_ISR1() {  _bpm = true; } 
//void clk_ISR2() {  _reset = true; }

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

  spi4teensy3::init();
  analogWriteResolution(12);
  
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
  attachInterrupt(TR1, clk_ISR1, FALLING);
  //attachInterrupt(TR2, clk_ISR2, FALLING);
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
    if (_bpm) {   _bpm = 0; all_clocks();} 
    /* update UI ?*/
    UI();
  
    coretimer();
    if (_bpm) {   _bpm = 0;  all_clocks(); } 
    /* update encoders ?*/
    if (millis() - LAST_BUT > DEBOUNCE) update_ENC();
  
    coretimer();
    if (_bpm) {   _bpm = 0; all_clocks(); }  
    /* update CV ?*/
    if (_adc) { _adc = 0; CV[1] = analogRead(CV4); CV[2] = analogRead(CV3); CV[3] = analogRead(CV1); CV[4] = analogRead(CV2);} 
  
    coretimer();  
    if (_bpm) {   _bpm = 0; all_clocks(); }  
    /* buttons ?*/
    if (_UI) { leftButton(); rightButton(); topButton(); lowerButton(); _UI = false; }

    coretimer();
    if (_bpm) {   _bpm = 0; all_clocks(); }  
    /* timeout ?*/
    if (UI_MODE && (millis() - LAST_UI > TIMEOUT)) time_out(); // timeout UI
 
    coretimer();
    if (_bpm) {   _bpm = 0; all_clocks(); }  
    /* clocks off ?*/
    if (display_clock) clocksoff();
  
    coretimer();
    if (_bpm) {   _bpm = 0; all_clocks(); } 
    /* do something ?*/
    if (b_state[BOTTOM]) checkbuttons(BOTTOM);
  
    coretimer();
    if (_bpm) {   _bpm = 0; all_clocks(); } 
    /* do something ?*/
    if (b_state[TOP]) checkbuttons(TOP);
  }
}



