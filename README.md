temps_utile-
============

teensy 3.x trigger generator w/ 128x64 oled display.

- build guide: https://github.com/mxmxmx/temps_utile-/wiki/Temps-Utile


![My image](https://farm1.staticflickr.com/628/20400765240_149a3ea220_b.jpg)


i/o:

**- 2 clock inputs (~2.5V threshold; 100k input impedance)**

**- 4 assignable CV inputs (bipolar (+/- 5V); 100k input impedance)**

**- 6 clock outputs (5 digital (10V), 1 DAC (12 bit, +/- 6V))**

**- 6 modes, selectable per channel (params / channel):** 

  - clock multiplication/division (pulse-width, multiplier) (available across modes)

  - LFSR (length, tap1, tap2)

  - random (N (threshold))

  - euclidian (N (length), K (fill), offset)

  - logic (operator (AND, OR, XOR, NAND, NOR), operand_1, operand_2)

  - trigger sequencer (pattern length(4-16), 4 editable patterns/channel)

  - DAC (channel #4 only): random, ‘binary’ sequencer, ‘Turing Machine’, logistic map.

25 mm Depth

![My image](https://farm1.staticflickr.com/654/20400744418_250ae63aeb_b.jpg)

