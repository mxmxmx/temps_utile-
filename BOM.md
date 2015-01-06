#/* BOM for temps utile */


**ICs/semis:**

- TL074* (SOIC) : 3x  
- NPN transistors (MMBT3904, SOT-23) 2x
- 1N5817 (diode, DO-41): 2x
- BAT54S (Schottky (dual/series) SOT-23): 4x
- LM4040-2.5 (SOT-23) : 1x (optional**)

**misc *through-hole*:**

- 22uF  (electrolytic) : 2x
- 100nf (film/ceramic, RM2.5) : 1x
- 100nF (film/ceramic, RM5)   : 1x
- 7805 5v regulator: 1x (TO 220)
- 79L05 -5v regulator: 1x (TO 92)
- 78L33 3v3 regulator: 1x (TO 92)
- trimpot 100k (inline / 9.5mm): 1x
- jacks: 'thonkiconn' (or kobiconn): 12x
- encoders (24 steps (ish)) with switch (e.g. PEC11R-4215K-S0024) : 2x (*****)
- 2x5 pin header, 2.54mm : 1x (euro power connector)
- 1x7 pin header, 2.54mm : 1x (for oled)
- 1x7 socket, 2.54mm (low profile) : 1x (for oled)
- 2x single row precision ("machined" / "round") pin header and sockets for teensy 3.1: RM 2.54mm
- 1x5 sockets + matching pin headers  : 1x (ditto)
- 1x2 male header *SMD* (2.54mm for the GPIO pins on the bottom side of the teensy) (****)
- 1x2 pin header, 2.54mm : 1x
- 1x3 pin header, 2.54mm : 1x (***)
- jumpers, 2.54mm : 2x
- tact switches (multimecs 5E/5G): 2x (mouser #: 642-5GTH935)
- + caps (multimecs 1SS09-15.0 or -16.0): 2x (mouser #: 642-1SS09-15.0, or -16.0)
- teensy3.x : 1x (cut the usb/power trace)
- oled: i've used these: http://www.ebay.com/itm/New-Blue-1-3-IIC-I2C-Serial-128-x-64-OLED-LCD-LED-Display-Module-for-Arduino-/261465635352 [ the pinout should be: GND - VCC - D0 - D1 - RST - DC - CS. adafruit's 128x64 (http://www.adafruit.com/products/326) should work as well (in terms of the hardware), at least the corresponding pads are on the pcb as well (untested).

**SMD resistors (0805):**

- 510R :         6x 
-  1k  :         6x
- 1k2  :         1x
- 1k8  :         1x
- 3k   :         2x
- 3k9  :         2x
- 10k  :         6x
- 20k  :         6x (*) 
- 33k :          6x 
- 49k9:          4x
- 100k :         10x

**SMD caps (0805):**

- 1n    : 2x (NP0/C0G)
- 3n3   : 1x (NP0/C0G)
- 18n   : 1x (NP0/C0G)
- 100nF : 15x  
- 330nF : 5x 
- 1uF   : 1x (may be 1206)


#notes:

(*) the (digital) clock outputs are non-inverting op amps, the values on the pcb (20k feedback, 10k to ground) will result in 3x gain, or 9.9v on the outputs. adjust, if you like.

(**) the lm4040 is not really needed at all in this application, but if installed this requires adjusting some values to get the ADC range approx. right (2v5 rather than 3v3); specifically, say 4x 49k9 - > 4x 62k; and 4x 100k -> 4x 130k (or 120k)
 
(***) it's possibly to switch the jack/output labelled "c4" between the DAC and a regular GPIO pin; that's what the one jumper is for. i'd probably just hard-wire it.

(****) connecting to some of the GPIO pins can't be done very elegantly, partly owing to the fact this module is entirely derivative from the DAC8565 one. there's two GPIO pins (28 and 29) on the bottom side of the controller that need to be connected - these are available as SMD pads only; i figure the best way to do it would be to solder a 2-pin SMD pin header to those pins and use corresponding sockets on the board. i just used two standard/through hole angled (90 degr.) pins, removed them from the plastic frame and soldered them directly to the board.

(*****) rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)







