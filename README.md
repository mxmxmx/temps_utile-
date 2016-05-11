temps_utile-
============

teensy 3.x trigger generator w/ 128x64 oled display.

- build guide: https://github.com/mxmxmx/temps_utile-/wiki/Temps-Utile


![My image](https://farm1.staticflickr.com/628/20400765240_149a3ea220_b.jpg)


i/o:

**- 2 clock inputs (~2.5V threshold; 100k input impedance)**

**- 4 assignable CV inputs (bipolar (+/- 5V); 100k input impedance)**

**- 6 clock outputs (5 digital (10V), 1 DAC (12 bit, +/- 6V))**

**- 5 modes, selectable per channel (params / channel):** 

  - clock multiplication/division (pulse-width, multiplier, inverted)

  - LFSR (pulse-width, length, tap1, tap2)

  - random (pulse-width, N, inverted)

  - euclidian (pulse-width, N (length), K (fill), offset)

  - logic (pulse-width, mode (AND, OR, XOR, NAND, NOR), op1, op2)

  - DAC (channel #4 only): random, binary seq.

25 mm Depth

![My image](https://farm1.staticflickr.com/654/20400744418_250ae63aeb_b.jpg)

