 
 
 void _loop() {
   
   
      if (coretimer()) next_clocks();
      
      /* update UI ?*/
      UI();
  
      
      /* update encoders ?*/
      if (millis() - LAST_BUT > DEBOUNCE) update_ENC();
  
      //if (coretimer()) next_clocks();
      
      /* update CV ?*/
      if (_adc) {
        _adc = 0;      
        CV[1] = analogRead(CV4);
        CV[2] = analogRead(CV3);
        CV[3] = analogRead(CV1);
        CV[4] = analogRead(CV2);
      }
  
      //if (coretimer()) next_clocks();
      
      /* buttons ?*/
      if (_UI) {
        buttons();
        _UI = false;
      }
  
      //if (coretimer()) next_clocks();
      
      /* timeout ?*/
      if (UI_MODE && (millis() - LAST_UI > TIMEOUT)) {
        save_settings();
        time_out(); // timeout UI
      }
  
      //if (coretimer()) next_clocks();
      /* clocks off ?*/
      if (display_clock) clocksoff();
  
      //if (coretimer()) next_clocks();
      
      if (button_states[BOTTOM])   process_buttons(BOTTOM);

      //if (coretimer()) if (coretimer()) next_clocks();

      if (button_states[LEFT_SW])  process_buttons(LEFT_SW);

      //if (coretimer()) next_clocks();

      if (button_states[RIGHT_SW]) process_buttons(RIGHT_SW);
  
      //if (coretimer()) next_clocks();
      
      /* do something ?*/
      if (button_states[TOP]) process_buttons(TOP);
  
      //if (coretimer()) next_clocks();

}
