temps_utile-
============

# 6 x clock generator.

![](https://c1.staticflickr.com/1/628/20400765240_149a3ea220_b.jpg)

... a fairly simple breakout board for teensy 3.1/3.2, focused on **clock sequencing** 

(the name may suggest as much ... it was stolen from M. **[Louis Lapicque](https://en.wikipedia.org/wiki/Louis_Lapicque)** (see: idem, 1907: Sur l'excitation par décharge de condensateurs; détermination directe de la durée et de la quantité utiles. _Comptes Rendus Soc. Biol._ (Paris) 62, 701-704).


### hardware basics, in brief:

- **teensy 3.1/3.2** @ 120MHz, w/ 128x64 OLED
- trigger-to-output **latency** < 100us.
- **2 clock inputs** (> 100k input impedance; threshold ~ 2.5V)
- **4 CV inputs** (100k input impedance, -/+ 5V, assignable to (almost) any parameter)
- **6 clock outputs** (5 digital, **1 DAC** (12 bit): 10V (GPIO), -/+ 6V (DAC))
- two encoders w/ switches; 2 tactile buttons.
- 14HP, ~ 25 mm Depth

### firmware: 

- **6 modes, selectable per channel:** 

  - trigger sequencer/sequence editor
  - clock division/multiplication
  - LFSR
  - random w/ threshold
  - euclidian
  - logic (AND, OR, XOR, NAND, NOR, XNOR)
  - DAC (channel #4 only): random, binary, "Turing", logistic, sequencer/arpeggiator


### build guide: [see here](https://github.com/mxmxmx/temps_utile-/wiki/build-it)

