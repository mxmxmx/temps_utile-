#/* BOM for temps utile (blue pcbs) */


**ICs/semis:**

- TL074 (SOIC) : 2x 
- MCP6004 (SOIC) : 1x 
- NPN transistors (MMBT3904, SOT-23): 2x
- 1N5817 (diode, DO-41): 2x
- LM1117-5v5 (5v0 LDO reg., SOT-223): 1x
- LM4040-5.0 (SOT-23) : 1x 
- 10uH fixed inductor (1206, > 25mA) : 1x

**misc *through-hole*:**

- 22uF  (electrolytic) : 2x
- 470nf (film/ceramic, RM5) : 1x
- 78L33 3v3 regulator: 1x (TO 92)
- trimpot 100k (inline / 9.5mm): 1x
- jacks: 'thonkiconn' (or kobiconn): 12x
- encoders (24 steps (ish)) with switch (e.g. PEC11R-4215K-S0024) : 2x (**)
- 2x5 pin header, 2.54mm : 1x (euro power connector)
- 1x7 socket, 2.54mm (low profile) : 1x (for oled)
- 1x3 pin header, 2.54mm : 1x (†)
- jumper, 2.54mm : 1x (or hardwire)
- socket + pin headers for teensy 3.x, 2.54mm : all outer pins (28) are used + DAC (A14), so best to get breakable ones. (††)
- tact switches (multimecs 5E/5G): 2x (mouser #: 642-5GTH935 or 642-5ETH935)
- + caps (multimecs 1SS09-15.0 or -16.0): 2x (mouser #: 642-1SS09-15.0, or -16.0) (†††)
- teensy3.1 or 3.2 : 1x (cut the usb/power trace)
- oled: i've used these: http://www.ebay.com/itm/New-Blue-1-3-IIC-I2C-Serial-128-x-64-OLED-LCD-LED-Display-Module-for-Arduino-/261465635352 [ the pinout should be: GND - VCC - D0 - D1 - RST - DC - CS. adafruit's 128x64 (http://www.adafruit.com/products/326) should work as well (in terms of the hardware), at least the corresponding pads are on the pcb as well (untested).

**SMD resistors (0805):**

- 100R : 		 4x
- 510R :         2x 
-  1k  :         7x
- 1k2  :         1x
- 1k8  :         1x
- 3k   :         2x
- 3k9  :         2x
- 10k  :         6x (‡)
- 20k  :         6x  
- 33k :          6x 
- 49k9:          4x
- 100k :         10x

**SMD caps (0805):**

- 1n    : 2x (NP0/C0G)
- 3n3   : 1x (NP0/C0G)
- 18n   : 1x (NP0/C0G)
- 100nF : 15x  
- 470nF : 5x 
- 1uF   : 2x
- 10uF  : 1x (may be 1206)

**SMD caps (0603):**

- 10n   : 4x 


#notes (also see the build 'wiki'):

(‡) the (digital) clock outputs are non-inverting op amps, the values on the pcb (20k feedback, 10k to ground) will result in 3x gain, or 9.9v on the outputs. adjust, if you like. for example, 15k would give you 3.3v * (20k/15k + 1) = 7.7, etc.

(*) as the requirements in terms of A/D are minimal, the lm4040 is not needed at all (really) in this particular application; if installed this requires adjusting some values to get the ADC range approx. right (2v5 rather than 3v3); specifically, say 4x 49k9 - > 4x 62k; and 4x 100k -> 4x 130k (or 120k)

(**) rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

(†) it's possibly to switch the jack/output labelled "c4" between the DAC and a regular GPIO pin; that's what the one jumper is for. as typically you'd use the DAC (unless using a teensy 3.0), i'd probably just hard-wire it.

(††) also see the build guide: we need 14 pins on either side plus the DAC pin, so best to simply use 14 on the one side, and 13 + 2 on the other.

(†††) way cheaper than mouser is http://www.soselectronic.com/





