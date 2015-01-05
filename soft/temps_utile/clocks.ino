/*
* 
*    clocks
*
*
*/

const int8_t MODES = 6; 

enum CLOCKMODES {
 
   LFSR,
   RANDOM,
   DIV,
   EUCLID,
   LOGIC,
   DAC 
  
};

const int8_t CHANNELS = 6; // # channels
uint8_t  CLOCKS_STATE;     // output state: XX654321
uint32_t TRIG_COUNTER;     // count clocks
uint32_t LAST_TRIG = 0;    // clock timestamp
uint32_t PW;               // ext. clock interval
uint8_t CLK_SRC = 0;       // clock source: ext/int
const float BPM_CONST = 29970000.0f; // 8th ?
uint16_t BPM = 100;        // int. clock
const uint8_t  BPM_MIN = 8; 
const uint16_t BPM_MAX = 320;
uint32_t BPM_MICROSEC=BPM_CONST/BPM;
uint32_t CORE_TIMER;
uint8_t init_mode = 2; // initial mode: 2 => mult/div
uint8_t CLOCKS_OFF_CNT;

const uint16_t _ZERO = 1800; // DAC 0.0V
const uint8_t DAC_CHANNEL = 3;
const uint8_t PULSE_WIDTH = 0;

extern uint8_t MENU_REDRAW;

struct params {
  
    uint8_t    mode;               // LFRS, RANDOM, MULT/DIV, EUCLID, LOGIC, DAC* 
    uint8_t    channel_modes;      // modes per channel: {5, 5, 5, 5, 5, 5+DAC }; 
    uint8_t    mode_param_numbers; // 3, 3, 2, 3, 3, 2   
    uint16_t   param[6][4];        // param values or submode: param[<mode>][<param>];
    uint16_t   param_min[4];       // limits min
    uint16_t   param_max[4];       // limits max
    uint8_t    cvmod[5];           // assigned param [none, param1, param2, param3, param4]
    uint32_t   lfsr;               // data
    uint8_t    _pw;                // pulse_width
};

const uint8_t  _CHANNEL_MODES[]  = {5, 5, 5, 6, 5, 5}; // modes per channel; ch 4 = 6, ie. 5 + DAC
const uint8_t  _CHANNEL_PARAMS[] = {3, 2, 2, 3, 3, 1}; // [len, tap1, tap2]; [rand(N), direction]; [div, direction]; [N, K, offset]; [type, op1, *op2]; [type, scale]
const uint16_t _CHANNEL_PARAMS_MIN[MODES][4] = {{5,4,0,0}, {5,1,0,0}, {5,0,0,0}, {5,2,1,0}, {5,0,1,1}, {0,0,0,0}}; // pw, p0, p1, p3
const uint16_t _CHANNEL_PARAMS_MAX[MODES][4] = {{150,31,31,31}, {150,31,1,0}, {150,31,1,0}, {150,31,31,15}, {150,4,6,6}, {1,31,0,0}}; // pw, p0, p1, p3

params *channel1, *channel2, *channel3, *channel4, *channel5, *channel6;

params allChannels[6] {
 
  *channel1, *channel2, *channel3, *channel4, *channel5, *channel6
  
};  

/*  -----------------  internal clock -------------------------------  */

void coretimer() {
  
 if (micros() - CORE_TIMER > BPM_MICROSEC) {  // BPM, 32th
  
        CORE_TIMER = micros();  
        if (CLK_SRC) _bpm++; 
  } 
}

/* ------------------------------------------------------------------   */

void make_channel(struct params* _p) { 

      _p = (params*)malloc(sizeof(params)); 
}

void init_channel(struct params* _p, uint8_t _channel) {
 
      _p->mode = init_mode; 
      _p->channel_modes = _CHANNEL_MODES[_channel];
      _p->mode_param_numbers = _CHANNEL_PARAMS[init_mode];
      const uint16_t *_min_ptr = _CHANNEL_PARAMS_MIN[init_mode];
      const uint16_t *_max_ptr = _CHANNEL_PARAMS_MAX[init_mode];
      memcpy(_p->param_min, _min_ptr, sizeof(_p->param_min));
      memcpy(_p->param_max, _max_ptr, sizeof(_p->param_max));
      _p->lfsr = random(0xFFFFFFFF);
      
      for (int i = 0; i < MODES; i++) {
        
         _p->param[i][0] = 50; // pw
         _p->param[i][1] = _CHANNEL_PARAMS_MIN[i][1];
         _p->param[i][2] = _CHANNEL_PARAMS_MIN[i][2];  
         _p->param[i][3] = _CHANNEL_PARAMS_MIN[i][3];
      }
      _p->cvmod[0] = 0;
      _p->cvmod[1] = 0;
      _p->cvmod[2] = 0;
      _p->cvmod[3] = 0;
      _p->cvmod[4] = 0;
      _p->param[DAC][1] = 15;  // init DAC mult. param
      _p->_pw = 50;            // pw [effective] 
}  

/* ------------------------------------------------------------------   */

void init_clocks(){
 
      for (int i  = 0; i < CHANNELS; i++) {
   
         make_channel(&allChannels[i]);
         init_channel(&allChannels[i], i);
     } 
}  

/* ------------------------------------------------------------------   */

uint8_t do_clock(struct params* _p, uint8_t _ch)   {
  
 switch (_p->mode) {
  
     case 0: return _lfsr(_p);
     case 1: return _rand(_p);
     case 2: return _plainclock(_p);
     case 3: return _euclid(_p);
     case 4: return (CLOCKS_STATE>>_ch)&1u; // logic: return prev.
     case 5: return _dac(_p);
 
 } 
}  

/* ------------------------------------------------------------------   */

void all_clocks() {
   
  /* output them first     */
  if (allChannels[DAC_CHANNEL].mode == DAC) outputDAC(&allChannels[DAC_CHANNEL]); 
  else if (CLOCKS_STATE & 0x8)   analogWrite(A14, 4000); 
   
  digitalWriteFast(CLK1, CLOCKS_STATE & 0x1);
  digitalWriteFast(CLK2, CLOCKS_STATE & 0x2);
  digitalWriteFast(CLK3, CLOCKS_STATE & 0x4);
  //  digitalWriteFast(CLK4, CLOCKS_STATE & 0x8); // see above
  digitalWriteFast(CLK5, CLOCKS_STATE & 0x10);
  digitalWriteFast(CLK6, CLOCKS_STATE & 0x20);
  
  /* keep track of things: */
  PW = LAST_TRIG;
  LAST_TRIG = millis();
  PW = LAST_TRIG - PW;
  TRIG_COUNTER++;
  display_clock = CLOCKS_STATE; // = true
  MENU_REDRAW = 1;
  /* reset ?              */
  if(!digitalReadFast(TR2)) sync(); 
  /* update clocks        */
  for (int i  = 0; i < CHANNELS; i++) {
        
        uint8_t tmp = do_clock(&allChannels[i], i); 
        if (tmp)  CLOCKS_STATE |=  (1 << i);    // set clock
        else      CLOCKS_STATE &= ~(1 << i);    // clear clock
        update_pw(&allChannels[i]);             // update pw
  }       
   /* apply logic           */
   for (int i  = 0; i < CHANNELS; i++) {
     
        if (allChannels[i].mode == LOGIC)  {
            uint8_t tmp = _logic(&allChannels[i]);
            if (tmp)  CLOCKS_STATE |=  (1 << i); // set clock 
            else      CLOCKS_STATE &= ~(1 << i); // clear clock
        }
   }
}  

/*  --------------------------- the clock modes --------------------------    */

// 1 : lfsr 

uint8_t _lfsr(struct params* _p) {
  
  uint8_t _mode, _tap1, _tap2, _len, _out;
  int16_t _cv1, _cv2, _cv3;
  uint32_t _data;
  
  _cv1 = _p->cvmod[2]; // len CV
  _cv2 = _p->cvmod[3]; // tap1 CV
  _cv3 = _p->cvmod[4]; // tap2 CV
  
  _mode = LFSR;
    
  if (_cv1) { // len
    
        _cv1 = (HALFSCALE - CV[_cv1]) >> 4;
        _len = limits(_p->param[_mode][1] + _cv1, 1, _mode);
        
  }
  else _len  = _p->param[_mode][1];
  if (_cv2) { // tap1
    
        _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
        _tap1 = limits(_p->param[_mode][2] + _cv2, 2, _mode);
        
  }
  else _tap1 = _p->param[_mode][2];
  if (_cv3) { // tap2
    
        _cv3 = (HALFSCALE - CV[_cv3]) >> 5;
        _tap2 = limits(_p->param[_mode][3] + _cv3, 3, _mode);
        
  }
  else _tap2 = _p->param[_mode][3];
  _data = _p->lfsr;
  _out = _data & 1u; 
  
  _tap1 = (_data >> _tap1) & 1u; // bit at tap1
  _tap2 = (_data >> _tap2) & 1u; // bit at tap2 
  _p->lfsr = (_data >> 1) | ((_out ^ _tap1 ^ _tap2) << (_len - 1)); // update lfsr
  if (_data == 0x0 || _data == 0xFFFFFFFF) _p->lfsr = random(0xFFFFFFFF); // let's not get stuck (entirely)
  return _out;
  
}  

// 2 : random

uint8_t _rand(struct params* _p) {
  
  uint8_t _mode, _n, _dir, _out; 
  int16_t _cv1, _cv2;
  
  _cv1 = _p->cvmod[2]; // RND-N CV
  _cv2 = _p->cvmod[3]; // DIR CV
  
  _mode = RANDOM;
  
  if (_cv1) {
        _cv1 = (HALFSCALE - CV[_cv1]) >> 5;
        _n = limits(_p->param[_mode][1] + _cv1, 1, _mode);
  }
  else _n   = _p->param[_mode][1];
  if (_cv2)  { // direction : 
        _cv2 = CV[_cv2];
        _dir = _p->param[_mode][2];
        if (_cv2 < THRESHOLD) _dir =  ~_dir & 1u;
        //_dir = limits(_p->param[_mode][2] + _cv2, 2, _mode);
  }  
  else _dir = _p->param[_mode][2];
  _out = random(_n+1);
  _out &= 1u;             // 0 or 1
  if (_dir) _out = ~_out; //  if inverted: 'yes' ( >= 1)  
  return _out&1u;
  
}

// 3 : clock div 

uint8_t _plainclock(struct params* _p) {
  
  uint16_t _mode, _n, _out;
  int16_t _cv1, _cv2, _div, _dir;
  _mode = DIV;
  
  _cv1 = _p->cvmod[2]; // div CV
  _cv2 = _p->cvmod[3]; // dir CV
 
  /* cv mod? */ 
  if (_cv1)  { // division: 
        _cv1 = (HALFSCALE - CV[_cv1]) >> 5;
        _div = limits(_p->param[_mode][1] + _cv1, 1, _mode);
  }
  else _div = _p->param[_mode][1];
  
  if (_cv2)  { // direction : 
        _cv2 = CV[_cv2];
        _dir = _p->param[_mode][2];
        if (_cv2 < THRESHOLD) _dir =  ~_dir & 1u;
        //_dir = limits(_p->param[_mode][2] + _cv2, 2, _mode);
  }  
  else _dir = _p->param[_mode][2]; 
  
  /* clk counter: */
  _n   = _p->param[_mode][3];

  if (!_n) _out = 1;
  else _out = 0; 
  
  _p->param[_mode][3]++;
  if (_n >= _div) { _p->param[_mode][3] = 0;}
  
  if (_dir) _out = ~_out;  // invert?  
  return _out&1u;
}

// 4: euclid

uint8_t _euclid(struct params* _p) {
  
  uint8_t _mode, _n, _k, _offset, _out;
  
  int16_t _cv1, _cv2, _cv3;
  
  _cv1 = _p->cvmod[2]; // n CV
  _cv2 = _p->cvmod[3]; // k CV
  _cv3 = _p->cvmod[4]; // offset CV
  
  _mode = EUCLID;
  
  if (_cv1)  { // N
        _cv1 = (HALFSCALE - CV[_cv1]) >> 5;
        _n = limits(_p->param[_mode][1] + _cv1, 1, _mode);
  }
  else _n = _p->param[_mode][1];
  if (_cv2) { // K
        _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
        _k = limits(_p->param[_mode][2] + _cv2, 2, _mode);
  }
  else _k = _p->param[_mode][2];
  if (_cv3) { // offset
        _cv3 = (HALFSCALE - CV[_cv3]) >> 5;
        _offset = _k = limits(_p->param[_mode][3] + _cv3, 3, _mode);
  }
  else _offset = _p->param[_mode][3];

  if (_k >= _n ) _k = _n - 1;
  _out = ((TRIG_COUNTER + _offset) * _k) % _n;
  if (_out < _k) _out = 1;
  else   _out = 0;
  return _out;
}

// 5: logic

uint8_t _logic(struct params* _p) {
  
  uint8_t _mode, _type, _op1, _op2, _out;
  
  int16_t _cv1, _cv2, _cv3;
  
  _cv1 = _p->cvmod[2]; // type CV
  _cv2 = _p->cvmod[3]; // op1 CV
  _cv3 = _p->cvmod[4]; // op2 CV
  
  _mode = LOGIC;
  if (_cv1) {
        _cv1 = (HALFSCALE - CV[_cv1]) >> 6;
        _type = limits(_p->param[_mode][1] + _cv1, 1, _mode);
  }
  else _type  = _p->param[_mode][1];
  if (_cv2) { 
        _cv2 = (HALFSCALE - CV[_cv2]) >> 6;
        _op1 = limits(_p->param[_mode][2] + _cv2, 2, _mode)-1;
  }
  else _op1   = _p->param[_mode][2]-1;
  if (_cv3) {
        _cv3 = (HALFSCALE - CV[_cv3]) >> 6;
        _op2 = limits(_p->param[_mode][3]+ _cv3, 3, _mode)-1;
  }
  else _op2   = _p->param[_mode][3]-1;
  
  _op1 = (CLOCKS_STATE >> _op1) & 1u;
  _op2 = (CLOCKS_STATE >> _op2) & 1u;

  switch (_type) {
    
         case 0: {  // AND
         _out = _op1 & _op2;
         break;
         }
         case 1: {  // OR
         _out = _op1 | _op2;
         break;
         }
         case 2: {  // XOR
         _out = _op1 ^ _op2;
         break;
         }
         case 3: {  // NAND
         _out = ~(_op1 & _op2);
         break;
         }
         case 4: {  // NOR
         _out = ~(_op1 | _op2);
         break;
         }
         default: {
         _out = 1;
         break;
         }       
  }  

  return _out&1u; 
}

// 6. DAC 

uint8_t _dac(struct params* _p) {

  return 1; 
}

/* ------------------------------------------------------------------   */

void outputDAC(struct params* _p) {
  
   int8_t _type, _scale, _mode; 
   int16_t _cv1, _cv2;
  
    _cv1 = _p->cvmod[1]; // type CV
    _cv2 = _p->cvmod[2]; // scale CV
    _type = _p->param[DAC][0];
    _mode = DAC;
    
    if (_cv1) {
        _cv1 = CV[_cv1];
        if (_cv1 < THRESHOLD) _type =  ~_type & 1u;
    } 
    if (_cv2) {
        _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
        _scale = 1 + limits(_p->param[_mode][1]+ _cv2, 1, _mode); 
    }
    else _scale = 1 + allChannels[DAC_CHANNEL].param[_mode][1];
  
    if (_type) analogWrite(A14, random(132*_scale) +  _ZERO);   // RND
    
    else       analogWrite(A14, _binary(CLOCKS_STATE, _scale)); // BIN 
}  

uint16_t _binary(uint8_t state, uint8_t _scale) {
   
   uint16_t tmp, _state = state;
  
   tmp  = (_state & 1u)<<10;
   tmp += ((_state>>1) & 1u)<<9;
   tmp += ((_state>>2) & 1u)<<8;
   tmp += ((_state>>4) & 1u)<<7;
   tmp += ((_state>>5) & 1u)<<6;
   
   tmp = (1+_scale)*(tmp/32.0f) + _ZERO;
   return tmp;   
}

/* ------------------------------------------------------------------   */

void update_pw(struct params* _p) {
  
         int16_t _cv, _pw, _mode;
         _cv = _p->cvmod[1]; // pw CV
         _mode = _p->mode;
         _pw = _p->param[_mode][PULSE_WIDTH]; // manual pw
         if (_cv) { // limit pulse_width
               _cv = (HALFSCALE - CV[_cv]) >> 3;
               _cv += _pw;  
               _p->_pw = limits(_cv, PULSE_WIDTH, DIV); 
         }
         else _p->_pw = _pw;
}

/* ------------------------------------------------------------------   */

uint8_t limits(int16_t _param_val, uint8_t _param, uint8_t _mode) {
  
      int8_t _x;
      uint16_t _min = _CHANNEL_PARAMS_MIN[_mode][_param];
      uint16_t _max = _CHANNEL_PARAMS_MAX[_mode][_param];
      if (_param_val < _min) _x = _min;
      else if (_param_val > _max) _x = _max;
      else _x = _param_val;
      return _x;
} 

/* ------------------------------------------------------------------   */

void clocksoff() {
  
    uint8_t _tmp = CLOCKS_OFF_CNT;
    uint8_t _mode = allChannels[_tmp].mode;
    uint8_t _pw = allChannels[_tmp].param[_mode][PULSE_WIDTH]; 

    if (millis() - LAST_TRIG > _pw) {
      
        /* turn clock off */
        display_clock &= ~(1<<_tmp);
    
        digitalWriteFast(CLK1, display_clock & 0x1);  
        digitalWriteFast(CLK2, display_clock & 0x2);
        digitalWriteFast(CLK3, display_clock & 0x4);
      //digitalWriteFast(CLK4, display_clock & 0x8);
        digitalWriteFast(CLK5, display_clock & 0x10);
        digitalWriteFast(CLK6, display_clock & 0x20);
        
        MENU_REDRAW = 1;
        /* DAC needs special treatment */
        if (_tmp == DAC_CHANNEL) { 
              if (allChannels[DAC_CHANNEL].mode < DAC) analogWrite(A14, _ZERO);     
              else { 
                  display_clock |=  (1 << _tmp);  
                  MENU_REDRAW = 0; 
              }      
         }
    }
    _tmp++;  
    _tmp = _tmp > 5 ? 0 : _tmp; 
    CLOCKS_OFF_CNT = _tmp;   
} 
 
 
