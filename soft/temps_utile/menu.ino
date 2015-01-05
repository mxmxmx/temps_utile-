/*
*
*   the menu
*
*
*
*/

// td: CV_DEST_CHANNEL = - , 1, 2, 3, 4, 5, 6

uint8_t ACTIVE_MENU_ITEM = init_mode; 
int8_t CV_MENU_ITEM;
uint8_t ACTIVE_MODE = init_mode;
int8_t  ACTIVE_CHANNEL = 0;
uint8_t UI_MODE = 0;
int8_t  MODE_SELECTOR = 0;
int8_t  MAIN_MENU_ITEM = 0;
uint8_t MENU_REDRAW = 0;

const int8_t MAIN_ITEMS = 5;
const int8_t MENU_ITEMS = 6;

uint8_t CV_DEST_CHANNEL[MAIN_ITEMS] = {0, 0, 0, 0, 0};
int16_t CV_DEST_PARAM[MAIN_ITEMS];

const uint8_t X_OFF = 110; // x offset

enum MENU_ {
  _SCREENSAVER,
  _MAIN,
  _BPM,
  _CV
};

enum MODE_ {
  _LFSR,
  _RANDOM,
  _CLOCK_DIV,
  _EUCLID,
  _LOGIC,
  _DAC 
};

/* ----------- menu strings ---------------- */

char *menu_strings[MENU_ITEMS*CHANNELS] = {

"", "", "pulse_w", "length: ", "tap1: ", "tap2: ", 
"", "", "pulse_w", "rand(N)", "inv.", "", 
"", "", "pulse_w", "div.", "inv.", "",
"", "", "pulse_w", "N:", "K (fill):", "offset:",
"", "", "pulse_w", "type: ", "op1: ", "op2: ",
"", "", "mode: ", "multiply:", "",""

};
const String display_mode[MODES] = {
      "LFSR", 
      "RANDOM", 
      "CLOCK/DIV", 
      "EUCLID", 
      "LOGIC", 
      "DAC"
};
const String display_channel[CHANNELS] = {
      "ch. # 1", 
      "ch. # 2",
      "ch. # 3",
      "ch. # 4",
      "ch. # 5",
      "ch. # 6"
};
const String display_CV_menu[MAIN_ITEMS] = {
      "cv#1  -->",
      "cv#2  -->",
      "cv#3  -->",
      "cv#4  -->", 
      "clk source:",
};
char *yesno[2] = {
       "no", 
       "yes"
};
char *operators[5] = {
       "AND",
       "OR",
       "XOR",
       "~&",
       "NOR"
};
char *DACmodes[2] = {
       "BIN",
       "RND"
};
char *clock_mode[4] = {
       "EXT",
       "EXT",
       "INT",
       "INT"
};

char *cv_display[] = {
       "-", "p_w", "len", "t_1", "t_2",
       "-", "p_w", "RND", "inv", "",
       "-", "p_w", "div", "inv", "",
       "-", "p_w",  "_N_", "_K_", "off",
       "-", "p_w", "_op", "op1", "op2", 
       "-", "dac", "mlt", "",""
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
    
      uint8_t i, h, ch;
      u8g_uint_t w, d, items;
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      h = u8g.getFontAscent()-u8g.getFontDescent()+1;
      w = 128;
      ch = ACTIVE_CHANNEL;
      items = allChannels[ch].mode_param_numbers+3; // offset by 3 lines
    
      // display clock ?
      if (display_clock) {                 
              u8g.setPrintPos(0,0);
              u8g.print("\xb7"); // clock
      }
               
      // display mode + channel
      u8g.setPrintPos(10, 0);
      if (MODE_SELECTOR == ACTIVE_MODE)   u8g.print(display_mode[allChannels[ch].mode]+"\xb7"); 
      else u8g.print(display_mode[MODE_SELECTOR]);           
      
      u8g.setPrintPos(80, 0); 
      u8g.print(display_channel[ch]); 
            
    
      for( i = 2; i < items; i++ ) {                // draw user menu
     
            u8g.setDefaultForegroundColor();
            
            if (i == ACTIVE_MENU_ITEM) {              
                u8g.drawBox(0, i*h, w, h);          // cursor bar
                u8g.setDefaultBackgroundColor();
            }
            /* print param names */
            u8g.drawStr(10, i*h, menu_strings[i+CHANNELS*allChannels[ch].mode]);
    
            /* print param values */
            u8g.setPrintPos(X_OFF,i*h);
            u8g.print(makedisplay(ch, ACTIVE_MODE, i-2));
            u8g.setDefaultForegroundColor();    
     }              
  }

  else if (UI_MODE==_BPM) {
  
      uint8_t ch;
      //u8g_uint_t items;
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      //h = u8g.getFontAscent()-u8g.getFontDescent()+1;
      //w = 128; // u8g.getWidth();
      ch = ACTIVE_CHANNEL;
      //items = allChannels[ch].mode_param_numbers+3; // offset by 3 lines
    
      // display mode + channel
      if (CLK_SRC) {
            if (BPM < 100) u8g.setPrintPos(84, 50); 
            else u8g.setPrintPos(78, 50);
            u8g.print(BPM); 
            u8g.setPrintPos(98, 50); 
            u8g.print("(bpm)"); 
      }
      else {
            u8g.setPrintPos(70, 50);
            u8g.print(PW);
            u8g.setPrintPos(98, 50); 
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
    
      uint8_t i, h;
      u8g_uint_t items, w;
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      h = u8g.getFontAscent()-u8g.getFontDescent()+4;
      w = 128; 
      items = MAIN_ITEMS;
            
      for(i = 0; i < items; i++ ) {                // draw user menu
     
            u8g.setDefaultForegroundColor();
            
            if (i == CV_MENU_ITEM) {     // cursor bar    
                u8g.drawBox(0, i*h, w, h-2);            
                u8g.setDefaultBackgroundColor();
            }
            /* print src / menu items */
            u8g.setPrintPos(10,i*h);
            u8g.print(display_CV_menu[i]);
            
            /* print dest: channel */
            if (i < 4 ) {
                uint8_t _tmp, _c;
                _tmp = CV_DEST_CHANNEL[i];
                 
                u8g.setPrintPos(82,i*h);
                if (_tmp) { u8g.print(_tmp); _c = allChannels[_tmp-1].mode; }
                else u8g.print("-");
                /* print dest: param  */
                _tmp = CV_DEST_PARAM[i];
                u8g.setPrintPos(X_OFF,i*h);
                //if (_tmp) 
                u8g.print(cv_display[_tmp+5*_c]);
                //else u8g.print("-");
                u8g.setDefaultForegroundColor(); 
            }
    
            else if (i == 4) {
                 /* print  clock source */
                u8g.setPrintPos(X_OFF,i*h);
                u8g.print(clock_mode[CV_DEST_CHANNEL[4]]);
                u8g.setDefaultForegroundColor(); 
           
            }
               
     }   
  }  
} 

String makedisplay(uint8_t _channel, uint8_t _mode, uint8_t _param_slot) {
  
     uint8_t paramval = allChannels[_channel].param[_mode][_param_slot];  
     String displaystring;
     switch (_mode) {
       
         case _LFSR: {
           displaystring = String(paramval); 
           break;
         } // lfsr
         case _RANDOM: { 
            if (_param_slot == 2) displaystring = String(yesno[paramval]);
            else displaystring = String(paramval); 
            break;
         } // random
         case _CLOCK_DIV: { 
            if (_param_slot == 2) displaystring = String(yesno[paramval]);
            else displaystring = String(paramval); 
            break;
         } // clock div
         case _EUCLID: { 
            displaystring = String(paramval); 
            break;
         } //  euclid
         case _LOGIC: { 
            if (_param_slot==1) displaystring = String(operators[paramval]);
            else displaystring = String(paramval); 
            break;
         } // logic
         case _DAC: { 
            
            if (!_param_slot) {
                  if (paramval > 1) { paramval = 1; encoder[RIGHT].setPos(1);} 
                  displaystring = String(DACmodes[paramval]); 
            }
            else if (_param_slot==1) displaystring = String(paramval);
            break;
         } // dac

     }
     
     return displaystring;
  
}  

/* ----------------- splash ---------------- */

void hello() {
  
  u8g.setFont(u8g_font_6x12);
  u8g.setColorIndex(1);
  u8g.firstPage();  
        do {
            
            for(int i = 0; i < 4; i++ ) { 
              u8g.drawBox(i*37, 20, 10, 24);
              
            }
    
        } while( u8g.nextPage() ); 
}  
