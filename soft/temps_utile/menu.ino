/*
*
*   the menu
*
*
*
*/

// td: CV_DEST_CHANNEL = - , 1, 2, 3, 4, 5, 6

uint16_t ACTIVE_MENU_ITEM = INIT_MODE; 
int16_t CV_MENU_ITEM;
uint16_t ACTIVE_MODE = INIT_MODE;
int16_t  ACTIVE_CHANNEL = 0;
uint16_t UI_MODE = 0;
int16_t  MODE_SELECTOR = 0;
int16_t  MAIN_MENU_ITEM = 0;
uint16_t MENU_REDRAW = 0;

const int16_t MAIN_ITEMS = 5;
const int16_t MENU_ITEMS = 6;

uint16_t CV_DEST_CHANNEL[MAIN_ITEMS] = {0, 0, 0, 0, 0};
int16_t CV_DEST_PARAM[MAIN_ITEMS];

const uint16_t X_OFFSET = 110; // x offset
const uint16_t Y_OFFSET = 0x0; // y offset

uint16_t CALIB_MENU = 0, CALIB_CLK; // for calibration/testing

enum MENU_ {
  _SCREENSAVER,
  _MAIN,
  _BPM,
  _CV,
  _CALIBRATE
};

enum MODE_ {
  _LFSR,
  _RANDOM,
  _CLOCK_DIV,
  _EUCLID,
  _LOGIC,
  _DAC 
};

enum CALIB_ {
  
  _CHECK_CV,
  _CHECK_DAC,
  _CHECK_CLK,
  _CALIBRATE_LAST
};

/* ----------- menu strings ---------------- */

const char *menu_strings[MENU_ITEMS*CHANNELS] = 
{
"", "", "pulse_w", "length: ", "tap1: ", "tap2: ", 
"", "", "pulse_w", "rand(N)", "inv.", "", 
"", "", "pulse_w", "div.", "inv.", "",
"", "", "pulse_w", "N:", "K (fill):", "offset:",
"", "", "pulse_w", "type: ", "op1: ", "op2: ",
"", "", "mode: ", "multiply:", "polarity:",""
};

const char *display_mode[MODES] = 
{
      "LFSR", 
      "RANDOM", 
      "CLOCK/DIV", 
      "EUCLID", 
      "LOGIC", 
      "DAC"
};

const char *display_channel[CHANNELS] = 
{
      "ch. # 1", 
      "ch. # 2",
      "ch. # 3",
      "ch. # 4",
      "ch. # 5",
      "ch. # 6"
};

const char *display_CV_menu[MAIN_ITEMS] = 
{
      "cv#1  -->",
      "cv#2  -->",
      "cv#3  -->",
      "cv#4  -->", 
      "clk source:",
};

const char *yesno[2] = 
{
       "no", 
       "yes"
};

const char *operators[5] = 
{
       "AND",
       "OR ",
       "XOR",
       "~& ",
       "NOR"
};

const char *DACmodes[4] = 
{
       "BIN",
       "RND",
       "UNI",
       "*BI"
};

const char *clock_mode[4] = 
{
       "EXT",
       "EXT",
       "INT",
       "INT"
};

const char *cv_display[] = 
{
       "-", "p_w", "len", "t_1", "t_2",
       "-", "p_w", "RND", "inv", "",
       "-", "p_w", "div", "inv", "",
       "-", "p_w",  "_N_", "_K_", "off",
       "-", "p_w", "_op", "op1", "op2", 
       "-", "dac", "mlt", "pol",""
};

const char *bpm_sel[] = 
{
       "1/4",
       "1/8",
       "1/16"
};

/* --------------------------------------------------- */

void UI() {
  
  if (MENU_REDRAW) {
        
        u8g.firstPage();  
        do {
     
          draw(); 
    
        } while( u8g.nextPage() );
        MENU_REDRAW = 0;
  }
}

/* -------------------------------------------------- */

void draw(void) { 
  
  
  if (UI_MODE==_SCREENSAVER) {  // screen 'saver'
       
        if (! display_clock) {
            
           for(int i = 0; i < 6; i++ ) { 
              u8g.drawFrame(i*23, 18, 10, 24);
          }
        }
        else   {  // clock?
           
            for(int i = 0; i < 6; i++ ) { 
             
               if (display_clock & (1<<i)) u8g.drawBox(i*23, 18, 10, 24);  // if bit set - > clock
               else u8g.drawFrame(i*23, 18, 10, 24);
                
          }
        }
  } 
  
  else if (UI_MODE==_MAIN) { // menu
    
      uint16_t h, w, x, y, ch, i, items;
     
      h = 11; // = u8g.getFontAscent()-u8g.getFontDescent()+2;
      w = 128;
      x = X_OFFSET;
      y = Y_OFFSET;
      ch = ACTIVE_CHANNEL;
      items = allChannels[ch].mode_param_numbers + 0x3; // offset by 3 lines
       
      // display mode + channel
      if (MODE_SELECTOR == ACTIVE_MODE)  { 
          u8g.setPrintPos(0x2,y);
          u8g.print("\xb7");
      }
      u8g.drawStr(10, y, display_mode[MODE_SELECTOR]); 
      u8g.drawStr(80, y, display_channel[ch]);       
      
      u8g.drawLine(0, 13, 128, 13);
    
      for( i = 2; i < items; i++ ) {                // draw user menu, offset by 2
     
            y = i*h; 
            u8g.setDefaultForegroundColor();
            
            if (i == ACTIVE_MENU_ITEM) {              
                u8g.drawBox(0, y, w, h);          // cursor bar
                u8g.setDefaultBackgroundColor();
            }
            /* print param names */
            u8g.drawStr(10, y, menu_strings[i+CHANNELS*allChannels[ch].mode]);
    
            /* print param values */
            print_param_values(ch, ACTIVE_MODE, i-2, x, y);
            u8g.setDefaultForegroundColor();    
     }              
  }

  else if (UI_MODE==_BPM) {
  
      // display mode + channel
      if (CLK_SRC) {
            u8g.setPrintPos(0, 50); 
            u8g.print(bpm_sel[BPM_SEL]); 
            if (BPM < 100) u8g.setPrintPos(84, 50); 
            else u8g.setPrintPos(78, 50);
            u8g.print(BPM); 
            u8g.setPrintPos(98, 50); 
            u8g.print("(bpm)"); 
      }
      else {  
            uint16_t _ms = PW / _FCPU;
            if (_ms < 10)         u8g.setPrintPos(94, 50);
            else if (_ms < 100)   u8g.setPrintPos(88, 50);
            else if (_ms < 1000)  u8g.setPrintPos(82, 50);
            else if (_ms < 10000) u8g.setPrintPos(76, 50);
            else u8g.setPrintPos(70, 50);
            u8g.print(_ms);
            u8g.setPrintPos(102, 50); 
            u8g.print("(ms)"); 
      }
    
      if (! display_clock) {
            
          for(int i = 0; i < 6; i++ ) { 
              u8g.drawFrame(i*23, 18, 10, 24);
          }
        }
       else   {  // clock?
           
          for(int i = 0; i < 6; i++ ) { 
             
               if (display_clock & (1<<i)) u8g.drawBox(i*23, 18, 10, 24); // if bit set - > clock
               else  u8g.drawFrame(i*23, 18, 10, 24);
                
          }
        }
  }
  else if (UI_MODE==_CV) {
    
      uint16_t h, w, i, x, y, items;
      
      h = 13; // = u8g.getFontAscent()-u8g.getFontDescent()+4
      w = 128;
      x = X_OFFSET; 
      items = MAIN_ITEMS;
          
      for(i = 0; i < items; i++ ) {                // draw user menu
     
            u8g.setDefaultForegroundColor();
            y = i*h;
            
            if (i == CV_MENU_ITEM) {     // cursor bar    
                u8g.drawBox(0, y, w, h-2);            
                u8g.setDefaultBackgroundColor();
            }
            // print src / menu items 
            u8g.drawStr(10, y, display_CV_menu[i]);
            
            // print dest: channel 
            if (i < 4 ) {
                uint16_t _tmp, _c = 0;
                _tmp = CV_DEST_CHANNEL[i];
                 
                u8g.setPrintPos(82,y);
                if (_tmp) { 
                    u8g.print(_tmp); 
                    _c = allChannels[_tmp-1].mode; 
                }
                else u8g.print("-");
                // print dest: param 
                _tmp = CV_DEST_PARAM[i];
                u8g.setPrintPos(x,y);
                u8g.print(cv_display[_tmp+5*_c]);
                u8g.setDefaultForegroundColor(); 
            }
          
            else if (i == 4) {
                // print  clock source
                u8g.setPrintPos(x,y);
                u8g.print(clock_mode[CV_DEST_CHANNEL[4]]);
                u8g.setDefaultForegroundColor(); 
           
            }
              
     }   
  }
  else if (UI_MODE == _CALIBRATE) {
             
               calibrate();
  }  
} 

/* ----------------- deal with special needs ---------------- */

void print_param_values(uint16_t _channel, uint16_t _mode, uint16_t _param_slot, uint16_t _x, uint16_t _y) {
  
  uint16_t paramval = allChannels[_channel].param[_mode][_param_slot]; 
 
  u8g.setPrintPos(_x,_y);
  
  switch (_mode) {
       
         case _LFSR: {
              u8g.print(paramval);
            break;
         } // lfsr
         
         case _RANDOM: { 
               if (_param_slot == 2) u8g.print(yesno[paramval]);
               else u8g.print(paramval);
            break;
         } // random
         
         case _CLOCK_DIV: {
               switch(_param_slot) {
                  case 1: u8g.print(paramval+0x1); break; 
                  case 2: u8g.print(yesno[paramval]); break;
                  default:
                    u8g.print(paramval); break;
              }
            break;
         } // clock div
         
         case _EUCLID: { 
              u8g.print(paramval);
            break;
         } //  euclid
         
         case _LOGIC: { 
              if (_param_slot==1) u8g.print(operators[paramval]);
              u8g.print(paramval);
            break;
         } // logic
         
         case _DAC: { 
              if (_param_slot==1) u8g.print(paramval);
              else {
                // this should be dealt with elsewhere
                if (paramval > 1) { 
                    paramval = 1; 
                    encoder[RIGHT].setPos(1);
                 } 
                 u8g.print(DACmodes[paramval+_param_slot]);
              }
            break;
         } // dac
  }
};

/* ----------------- splash ---------------- */

void hello() {
  
  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setColorIndex(1);
  u8g.firstPage();  
        do {
            
            for(int i = 0; i < 4; i++ ) { 
              u8g.drawBox(i*37, 20, 10, 24);
              
            }
    
        } while( u8g.nextPage() ); 
}  

/* ----------- calibration stuff ----------- */

void calibrate_main() {
  
        UI_MODE = _CALIBRATE;
        MENU_REDRAW = 1;
        
        u8g.setFont(u8g_font_6x12);
        u8g.setColorIndex(1);
        u8g.setFontRefHeightText();
        u8g.setFontPosTop();
        Serial.print("DAC offset: ");
        Serial.println(_ZERO[0]);
        encoder[RIGHT].setPos(_ZERO[0]);
       
        while(UI_MODE == _CALIBRATE) {
         
             UI();
             delay(20);
             
             CV[1] = analogRead(CV4); 
             CV[2] = analogRead(CV3); 
             CV[3] = analogRead(CV1); 
             CV[4] = analogRead(CV2);
             CALIB_CLK = digitalRead(TR2);
             
             MENU_REDRAW = 1;
             
             if (millis() - LAST_BUT > DEBOUNCE && !digitalReadFast(butR)) {
               
                 if (CALIB_MENU < _CALIBRATE_LAST) {
                     CALIB_MENU++;
                     LAST_BUT = millis();
                 }
                 else {        
                     UI_MODE = _SCREENSAVER;
                     save_settings();
                     Serial.println("saved..");
                     LAST_BUT = millis();
                 }
             }
             
        }
}

void calibrate(){
     
  if (CALIB_MENU == _CHECK_CV) {
    
       u8g.drawStr(10, 10, "CV1 -- >");
       u8g.setPrintPos(80,10);
       u8g.print(CV[1]);
       u8g.drawStr(10, 25, "CV2 -- >");
       u8g.setPrintPos(80,25);
       u8g.print(CV[2]);
       u8g.drawStr(10, 40, "CV3 -- >");
       u8g.setPrintPos(80,40);
       u8g.print(CV[3]);
       u8g.drawStr(10, 55, "CV4 -- >");
       u8g.setPrintPos(80,55);
       u8g.print(CV[4]);    
  }
  else if (CALIB_MENU == _CHECK_DAC) {
       u8g.drawStr(10, 10, "DAC -- > 0.0v");
       u8g.drawStr(10, 25, "adjust code:"); 
       u8g.setPrintPos(85,25);
       uint16_t _zero = encoder[RIGHT].pos(); 
       u8g.print(_zero);
       analogWrite(A14, _zero);
       _ZERO[0] = _zero;    
       u8g.drawStr(10, 55, "... use R encoder"); 
  }
  else if (CALIB_MENU == _CHECK_CLK) {
    
       u8g.drawStr(10, 25, "CLK2 -- >");
       if (CALIB_CLK)  u8g.drawStr(80, 25, "OFF");
       else  u8g.drawStr(80, 25, "CLK");
  } 
  else {

      u8g.drawStr(10, 25, "... done");
  }
}
