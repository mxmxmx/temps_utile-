/*
*
*    clocks
*
*
*/

const int8_t MODES = 6;

enum CLOCKMODES 
{
  LFSR,
  RANDOM,
  DIV,
  EUCLID,
  LOGIC,
  DAC

};

enum _BPM_SEL 
{  
  _4TH, 
  _8TH,
  _16TH
  
};

extern const uint32_t BPM_microseconds_4th[];
extern const uint32_t BPM_microseconds_8th[];
extern const uint32_t BPM_microseconds_16th[];

const int8_t CHANNELS = 6; // # channels
uint8_t  CLOCKS_STATE;     // output state: XX654321

uint32_t CLOCK_CNT;        // count clocks
uint32_t CLOCKS_OFF_CNT;   //

volatile uint32_t TIME_STAMP = 0;   // systick
volatile uint32_t PREV_TIME_STAMP = 0;
uint32_t PW = 0;      // ext. clock interval
uint32_t PREV_PW = 0; // ext. clock interval

uint32_t LAST_TRIG = 0;        // clocks_off timestamp (ms)

volatile uint16_t CLK_SRC = false; // clock source: ext/int
volatile uint16_t _OK = true;      // ext. clock ok ?
const uint16_t CLK_LIMIT_MS = 25; // max speed (ms)
const uint32_t CLK_LIMIT = CLK_LIMIT_MS*_FCPU; // max speed (cycles/1000)
uint16_t BPM = 100;            // int. clock
uint16_t BPM_SEL = 0;          // 1/4, 1/8, 1/16
const uint8_t  BPM_MIN = 8;
const uint16_t BPM_MAX[] = { 320, 320, 200};
uint32_t BPM_MICROSEC = BPM_microseconds_4th[BPM - BPM_MIN]; ; // BPM_CONST/BPM;
uint32_t CORE_TIMER;
uint8_t INIT_MODE = 2; // initial mode: 2 => mult/div

uint16_t DAC_OUT = 0;
const uint16_t _ON = 4000;
const uint16_t _ZERO[2] = {1800, 0}; // DAC 0.0V
const uint16_t DAC_CHANNEL = 3;

const uint16_t PULSE_WIDTH = 0;


extern uint16_t MENU_REDRAW;

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
const uint8_t  _CHANNEL_PARAMS[] = {3, 2, 2, 3, 3, 2}; // [len, tap1, tap2]; [rand(N), direction]; [div, direction]; [N, K, offset]; [type, op1, *op2]; [scale, polarity]
const uint16_t _CHANNEL_PARAMS_MIN[MODES][4] = {
  // pw, p0, p1, p3
  {5, 4, 0, 0}, // lfsr
  {5, 1, 0, 0}, // rnd
  {5, 0, 0, 0}, // div
  {5, 2, 1, 0}, // euclid
  {5, 0, 1, 1}, // logic
  {0, 0, 0, 0} // dac
};
const uint16_t _CHANNEL_PARAMS_MAX[MODES][4] = {
  // pw, p0, p1, p3
  {150, 31, 31, 31},
  {150, 31, 1, 0},
  {150, 31, 1, 0},
  {150, 31, 31, 15},
  {150, 4, 6, 6},
  {1, 31, 1, 0}
};

params allChannels[6];

/* ------------------------------------------------------------------   */
void bpm_set_microseconds() {
  switch (BPM_SEL) {

      case _4TH:  BPM_MICROSEC = BPM_microseconds_4th[BPM-BPM_MIN];  break;
      case _8TH:  BPM_MICROSEC = BPM_microseconds_8th[BPM-BPM_MIN];  break;
      case _16TH: BPM_MICROSEC = BPM_microseconds_16th[BPM-BPM_MIN]; break;
      default: break;
  }
}

/* ------------------------------------------------------------------   */
void channel_set_mode(struct params* p, uint8_t mode) {
  
  p->mode = mode;
  p->mode_param_numbers = _CHANNEL_PARAMS[mode];
  const uint16_t *_min_ptr = _CHANNEL_PARAMS_MIN[mode];
  const uint16_t *_max_ptr = _CHANNEL_PARAMS_MAX[mode];
  memcpy(p->param_min, _min_ptr, sizeof(p->param_min));
  memcpy(p->param_max, _max_ptr, sizeof(p->param_max));
}

/* ------------------------------------------------------------------   */
void clocks_restore_channel(struct params* p, const struct channel_settings* settings) {
  
  channel_set_mode(p, settings->mode);
  memcpy(p->param[p->mode], settings->param, sizeof(settings->param));
  memcpy(p->cvmod, settings->cvmod, sizeof(p->cvmod));
}

/* ------------------------------------------------------------------   */
void clocks_store_channel(const struct params* p, struct channel_settings* settings) {
  
  settings->mode = p->mode;
  memcpy(settings->param, p->param[p->mode], sizeof(settings->param));
  memcpy(settings->cvmod, p->cvmod, sizeof(p->cvmod));
}

/* ------------------------------------------------------------------   */
void clocks_store(struct settings_data *settings) {

  settings->clk_src = (uint8_t)CLK_SRC;
  settings->bpm = BPM;
  settings->bpm_sel = (uint8_t)BPM_SEL;

  for (int i  = 0; i < 6; i++) {
    clocks_store_channel(&allChannels[i], &settings->channels[i]);
  }
}

/* ------------------------------------------------------------------   */
void clocks_restore(const struct settings_data *settings) {
  
  CLK_SRC = settings->clk_src;
  BPM = settings->bpm;
  BPM_SEL = settings->bpm_sel;
  bpm_set_microseconds();

  for (int i  = 0; i < 6; i++) {
    clocks_restore_channel(&allChannels[i], &settings->channels[i]);
  }
}

/*  -----------------  internal clock -------------------------------  */

void coretimer() {

  if (CLK_SRC && micros() - CORE_TIMER > BPM_MICROSEC) {  // BPM, 32th

    CORE_TIMER = micros();
    output_clocks();
    _bpm++;
  }
}

/* ------------------------------------------------------------------   */

void init_channel(struct params* _p, uint8_t _channel) {

  channel_set_mode(_p, INIT_MODE);
  _p->channel_modes = _CHANNEL_MODES[_channel];
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

void init_clocks() {

  for (int i  = 0; i < CHANNELS; i++) {

    init_channel(&allChannels[i], i);
  }
}

/* ------------------------------------------------------------------   */

uint8_t gen_next_clock(struct params* _p, uint8_t _ch)   {

  switch (_p->mode) {

    case LFSR:   return _lfsr(_p); 
    case RANDOM: return _rand(_p); 
    case DIV:    return _plainclock(_p); 
    case EUCLID: return _euclid(_p); 
    case LOGIC:  return (CLOCKS_STATE >> _ch) & 1u; // logic: return prev. value
    case DAC:    return _dac(_p);
    default:     return 0xFF;
  }
}

/* ------------------------------------------------------------------   */

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

/* ------------------------------------------------------------------   */

void output_clocks() {  // update clock outputs - atm, this is called either by the ISR or coretimer()
 
  digitalWriteFast(CLK1, CLOCKS_STATE & 0x1);
  digitalWriteFast(CLK2, CLOCKS_STATE & 0x2);
  digitalWriteFast(CLK3, CLOCKS_STATE & 0x4);
//digitalWriteFast(CLK4, CLOCKS_STATE & 0x8); // --> DAC
  digitalWriteFast(CLK5, CLOCKS_STATE & 0x10);
  digitalWriteFast(CLK6, CLOCKS_STATE & 0x20);
  analogWrite(A14, DAC_OUT);
}

void next_clocks() {

  CLOCK_CNT++;          // count clocks
  LAST_TRIG = millis(); // -> clocksoff()

  _OK = PW < CLK_LIMIT ? 0 : 1; // ext clock > limit ?
  MENU_REDRAW = 1;     // redraw menu
  //Serial.println(_OK);
  display_clock = CLOCKS_STATE; // = true
  // update clocks:
  if (!digitalReadFast(TR2)) sync();
  
  for (int i  = 0; i < CHANNELS; i++) {

    uint16_t tmp = gen_next_clock(&allChannels[i], i);
    if (tmp)  CLOCKS_STATE |=  (1 << i);    // set clock
    else      CLOCKS_STATE &= ~(1 << i);    // clear clock
    update_pw(&allChannels[i]);             // update pw
  }
  // apply logic:
  for (int i  = 0; i < CHANNELS; i++) {

    if (allChannels[i].mode == LOGIC)  {
      uint16_t tmp = _logic(&allChannels[i]);
      if (tmp)  CLOCKS_STATE |=  (1 << i); // set clock
      else      CLOCKS_STATE &= ~(1 << i); // clear clock
    }
  }
  // next DAC code: 
  if (allChannels[DAC_CHANNEL].mode == DAC) outputDAC(&allChannels[DAC_CHANNEL]);
  else DAC_OUT = (CLOCKS_STATE & 0x8) ? _ON : _ZERO[0];
}

/*  --------------------------- the clock modes --------------------------    */

// 1 : lfsr

uint8_t _lfsr(struct params* _p) {

  uint8_t _mode, _tap1, _tap2, _len, _out;
  int16_t _cv1, _cv2, _cv3;
  uint32_t _data;
  _mode = LFSR;

  _cv1 = _p->cvmod[2];         // len CV
  _cv2 = _p->cvmod[3];         // tap1 CV
  _cv3 = _p->cvmod[4];         // tap2 CV
  _len  = _p->param[_mode][1]; // len param
  _tap1 = _p->param[_mode][2]; // tap1 param
  _tap2 = _p->param[_mode][3]; // tap2 param

  /* cv mod ? */
  if (_cv1) { // len

    _cv1 = (HALFSCALE - CV[_cv1]) >> 4;
    _len = limits(_p, 1, _cv1 + _len);
  }
  if (_cv2) { // tap1

    _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
    _tap1 = limits(_p, 2, _cv2 + _tap1);
  }
  if (_cv3) { // tap2

    _cv3 = (HALFSCALE - CV[_cv3]) >> 5;
    _tap2 = limits(_p, 3, _cv3 + _tap2);
  }

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
  _mode = RANDOM;

  _cv1 = _p->cvmod[2]; // RND-N CV
  _cv2 = _p->cvmod[3]; // DIR CV
  _n   = _p->param[_mode][1]; // RND-N param
  _dir = _p->param[_mode][2]; // DIR param

  /* cv mod ? */
  if (_cv1) {
    _cv1 = (HALFSCALE - CV[_cv1]) >> 5;
    _n = limits(_p, 1, _cv1 + _n);
  }
  if (_cv2)  { // direction :
    _cv2 = CV[_cv2];
    if (_cv2 < THRESHOLD) _dir =  ~_dir & 1u;
  }

  _out = random(_n + 1);
  _out &= 1u;             // 0 or 1
  if (_dir) _out = ~_out; //  if inverted: 'yes' ( >= 1)
  return _out & 1u;
}

// 3 : clock div

uint8_t _plainclock(struct params* _p) {

  uint16_t _mode, _n, _out;
  int16_t _cv1, _cv2, _div, _dir;
  _mode = DIV;

  _cv1 = _p->cvmod[2]; // div CV
  _cv2 = _p->cvmod[3]; // dir CV
  _div = _p->param[_mode][1]; // div param
  _dir = _p->param[_mode][2]; // dir param

  /* cv mod? */
  if (_cv1)  { // division:
    _cv1 = (HALFSCALE - CV[_cv1]) >> 4;
    _div = limits(_p, 1, _cv1 + _div);
  }

  if (_cv2)  { // direction :
    _cv2 = CV[_cv2];
    if (_cv2 < THRESHOLD) _dir =  ~_dir & 1u;
  }

  /* clk counter: */
  _n   = _p->param[_mode][3];

  if (!_n) _out = 1;
  else _out = 0;

  _p->param[_mode][3]++;
  if (_n >= _div)  _p->param[_mode][3] = 0;

  if (_dir) _out = ~_out;  // invert?
  return _out & 1u;
}

// 4: euclid

uint8_t _euclid(struct params* _p) {

  uint8_t _mode, _n, _k, _offset, _out;
  int16_t _cv1, _cv2, _cv3;
  _mode = EUCLID;

  _cv1 = _p->cvmod[2];           // n CV
  _cv2 = _p->cvmod[3];           // k CV
  _cv3 = _p->cvmod[4];           // offset CV
  _n = _p->param[_mode][1];      // n param
  _k = _p->param[_mode][2];      // k param
  _offset = _p->param[_mode][3]; // _offset param

  if (_cv1)  { // N
    _cv1 = (HALFSCALE - CV[_cv1]) >> 5;
    _n = limits(_p, 1, _cv1 + _n);
  }
  if (_cv2) { // K
    _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
    _k = limits(_p, 2, _cv2 + _k);
  }
  if (_cv3) { // offset
    _cv3 = (HALFSCALE - CV[_cv3]) >> 5;
    _offset = limits(_p, 3, _cv3 + _offset);
  }

  if (_k >= _n ) _k = _n - 1;
  _out = ((CLOCK_CNT + _offset) * _k) % _n;
  return (_out < _k) ? 1 : 0;
}

// 5: logic

uint8_t _logic(struct params* _p) {

  uint8_t _mode, _type, _op1, _op2, _out;
  int16_t _cv1, _cv2, _cv3;
  _mode = LOGIC;

  _cv1 = _p->cvmod[2];            // type CV
  _cv2 = _p->cvmod[3];            // op1 CV
  _cv3 = _p->cvmod[4];            // op2 CV
  _type  = _p->param[_mode][1];   // type param
  _op1   = _p->param[_mode][2] - 1; // op1 param
  _op2   = _p->param[_mode][3] - 1; // op2 param

  /* cv mod ?*/
  if (_cv1) {
    _cv1 = (HALFSCALE - CV[_cv1]) >> 6;
    _type = limits(_p, 1, _cv1 + _type);
  }
  if (_cv2) {
    _cv2 = (HALFSCALE - CV[_cv2]) >> 6;
    _op1 = limits(_p, 2, _cv2 + _op1) - 1;
  }
  if (_cv3) {
    _cv3 = (HALFSCALE - CV[_cv3]) >> 6;
    _op2 = limits(_p, 3, _cv3 + _op2) - 1;
  }

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

  return _out & 1u;
}

// 6. DAC

uint8_t _dac(struct params* _p) {

  return 1;
}

/* ------------------------------------------------------------------   */

void outputDAC(struct params* _p) {

  int8_t _type, _scale, _polar, _mode;
  int16_t _cv1, _cv2, _cv3;
  _mode = DAC;

  _cv1   = _p->cvmod[1];        // type CV
  _cv2   = _p->cvmod[2];        // scale CV
  _cv3   = _p->cvmod[3];        // polar.CV
  _type  = _p->param[_mode][0]; // type param
  _scale = _p->param[_mode][1]; // scale param
  _polar = _p->param[_mode][2]; // polar param

  if (_cv1) {
    _cv1 = CV[_cv1];
    if (_cv1 < THRESHOLD) _type =  ~_type & 1u;
  }
  if (_cv2) {
    _cv2 = (HALFSCALE - CV[_cv2]) >> 5;
    _scale = limits(_p, 1, _scale + _cv2);
  }
  if (_cv3) {
    _cv3 = CV[_cv3];
    if (_cv3 < THRESHOLD) _polar =  ~_polar & 1u;
  }

  if (_type) DAC_OUT = random(132 * (_scale + 1)) +  _ZERO[_polar]; // RND
  else       DAC_OUT = _binary(CLOCKS_STATE, _scale + 1, _polar); // BIN
}

uint16_t _binary(uint8_t state, uint8_t _scale, uint8_t _pol) {

  uint16_t tmp, _state = state;

  tmp  = (_state & 1u) << 10;       // ch 1
  tmp += ((_state >> 1) & 1u) << 9; // ch 2
  tmp += ((_state >> 2) & 1u) << 8; // ch 3
  tmp += ((_state >> 4) & 1u) << 7; // ch 5
  tmp += ((_state >> 5) & 1u) << 6; // ch 6
  tmp = _scale * (tmp >> 5);
  
  return tmp*(_pol + 0x01) + _ZERO[_pol]; // uni/bi + offset
}

/* ------------------------------------------------------------------   */

void update_pw(struct params* _p) {

  int16_t _cv, _pw, _mode;
  _mode = _p->mode;
  _cv = _p->cvmod[1];                  // pw CV
  _pw = _p->param[_mode][PULSE_WIDTH]; // manual pw

  if (_cv) {                           // limit pulse_width
    _cv = (HALFSCALE - CV[_cv]) >> 3;
    _p->_pw = limits(_p, PULSE_WIDTH, _cv + _pw);
  }
  else _p->_pw = _pw;
}

/* ------------------------------------------------------------------   */

uint8_t limits(struct params* _p, uint8_t _param, int16_t _CV) {

  int16_t  _param_val = _CV;
  uint16_t _min = _p->param_min[_param];
  uint16_t _max = _p->param_max[_param];

  if (_param_val < _min) _param_val = _min;
  else if (_param_val > _max) _param_val = _max;
  else _param_val = _param_val;
  return _param_val;
}

/* ------------------------------------------------------------------   */

void clocksoff() {

  uint8_t _tmp = CLOCKS_OFF_CNT;
  //uint8_t _mode = allChannels[_tmp].mode;
  //uint8_t _pw = allChannels[_tmp].param[_mode][PULSE_WIDTH];
  uint8_t _pw = allChannels[_tmp]._pw;

  if (millis() - LAST_TRIG > _pw) {

    /* turn clock off */
    display_clock &= ~(1 << _tmp);

    digitalWriteFast(CLK1, display_clock & 0x1);
    digitalWriteFast(CLK2, display_clock & 0x2);
    digitalWriteFast(CLK3, display_clock & 0x4);
    //digitalWriteFast(CLK4, display_clock & 0x8);
    digitalWriteFast(CLK5, display_clock & 0x10);
    digitalWriteFast(CLK6, display_clock & 0x20);

    MENU_REDRAW = 1;
    /* DAC needs special treatment */
    if (_tmp == DAC_CHANNEL) {
      if (allChannels[DAC_CHANNEL].mode < DAC) analogWrite(A14, _ZERO[0]);
      else {
        display_clock |=  (1 << _tmp);
        MENU_REDRAW = 0;
      }
    }
  }
  _tmp = _tmp++ > 0x4 ? 0 : _tmp; // reset counter
  CLOCKS_OFF_CNT = _tmp;

}

/* ------------------------------------------------------------------   */

void _wait()
{
  if (millis() - LAST_TRIG > CLK_LIMIT_MS)  _OK = true;
}




