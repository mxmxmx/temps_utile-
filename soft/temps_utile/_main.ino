 
 
 void _loop() {
   
     coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* update UI ?*/
      UI();
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* update encoders ?*/
      if (millis() - LAST_BUT > DEBOUNCE) update_ENC();
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* update CV ?*/
      if (_adc) {
        _adc = 0;      
        CV[1] = analogRead(CV4);
        CV[2] = analogRead(CV3);
        CV[3] = analogRead(CV1);
        CV[4] = analogRead(CV2);
      }
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* buttons ?*/
      if (_UI) {
        leftButton();
        rightButton();
        topButton();
        lowerButton();
        _UI = false;
      }
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* timeout ?*/
      if (UI_MODE && (millis() - LAST_UI > TIMEOUT)) {
        save_settings();
        time_out(); // timeout UI
      }
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* clocks off ?*/
      if (display_clock) clocksoff();
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* do something ?*/
      if (button_states[BOTTOM]) checkbuttons(BOTTOM);
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* do something ?*/
      if (button_states[TOP]) checkbuttons(TOP);
  
      coretimer();
      if (_bpm) {
        _bpm = 0;
        next_clocks();
      }
      /* do nothing? */
      if (!_OK) _wait();
}
