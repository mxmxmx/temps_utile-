#/* BOM for temps utile (blue pcbs) */


###SMD resistors (0805):

| value | # | note |
| ---: | ---: | --- |
|100R |		 4x | - |
| 510R |         2x | - |
| 1k  |       7x | - |
| 1k2  |         1x | - |
| 1k8  |         1x | - |
| 3k   |         2x | - |
| 3k9  |        2x | - |
| 6k8  |        1x | - |
| 10k  |         7x | **(‡)** |
| 20k  |         6x  | - | 
| 33k |          6x  | - |
| 100k |         10x | (‡‡) |

- (‡) the (digital) clock outputs are **non-inverting op amps**, the values on the pcb (20k feedback, 10k to ground) will result in 3x gain, or 9.9v on the outputs. adjust, if you like. for example, 15k would give you 3.3v * (20k/15k + 1) = 7.7, etc.
- (‡‡) use 100k where it says 49k9, too ( = 4x)


###SMD caps (0805):

| value | # | note |
| ---: | ---: | --- |
| 1n    | 2x  |(NP0/C0G) (50V)|
|3n3   |1x |(NP0/C0G) (50V)|
|18n   | 1x |(NP0/C0G) (50V)|
|100nF | 9x | (25V or better)|
|470nF | 5x | (25V or better) **(‡)**|
| 1uF   | 2x | (25V or better)|
|10uF  | 1x |(16V or better; may be 1206)|

- (‡) use one in lieu of 330nF, adjacent to the 78L33 regulator.

###SMD caps (0603):
| value | # | note |
| ---: | ---: | --- |
|10n   | 4x | (NP0/C0G) (16V or better)|

###ICs/semis:

| what | package | # | note |
| --- | --- | ---: | --- |
| TL074 | SOIC-14 | 2x | - |
| MCP6004 | SOIC-14 | 1x | - |
| MMBT3904 | SOT-23 | 2x | NPN |
| 1N5817 | DO-41 | 2x | Schottky, reverse voltage protection|
| LM1117-5v0 | SOT-223 | 1x | 5v LDO |
| LM4040-5.0 | SOT-23 |  1x | prec. voltage reference, 5v0|
| fixed inductor, 10uH | 1206 | 1x |  > 25mA |

###misc *through-hole*:
| what | package | # | note |
| --- | --- | ---: | --- |
| 22uF | 2.5mm | 2x |  electrolytic, 35v | 
| 470nf | 5mm |  1x | film/ceramic, 35v | 
| 78L33 |  TO-92 | 1x | 3v3 regulator | 
|  jacks | [PJ301M-12](https://www.thonk.co.uk/shop/3-5mm-jacks/) | 12x |  'thonkiconn' (or kobiconn) | 
| encoder w/ switch | 9mm | 2x | 24 steps (e.g. PEC11R-4215K-S0024) **(‡)**  | 
|  2x5 male header | 2.54mm |  1x |  (euro power connector) | 
|  1x7 female socket |  2.54mm  |  1x |  low profile (for oled) | 
|  1x3 make header |  2.54mm |  1x | or use a piece of wire **(†)**  | 
|  + jumper |  2.54mm |  1x |  ditto |  
|  (breakable) sockets / headers for teensy 3.x |  2.54mm | 2x | 1x14 + 1 **(††)** | 
| tact switches | multimecs 5E/5G | 2x  | mouser #: 642-5GTH935 or 642-5ETH935 |
| + caps | multimecs 5E/5G | 2x | mouser #: 642-1SS09-15.0, or -16.0| 
| teensy 3.x | - | 1x | cut the usb/power trace! |
| OLED, 1.3" | 7 pin | 1x | SH1106, 128x64 **(†††)** |

- (‡) rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

- (†) it's possibly to switch the jack/output labelled `clk4` between the **DAC** and a regular **GPIO** pin; that's what the one jumper is for. as typically you'd use the DAC (unless using a teensy 3.0), i'd probably just hard-wire it.

- (††) also see the build guide: we need 14 pins on either side plus the DAC pin, so best to simply use 14 on the one side, and 13 + 2 on the other.

- (†††) OLEDs:
  - you can find these 1.3" displays on ebay or aliexpress for < 10$. as long as the description claims that they are `SH1106` or `SSD1306` and the pinout is: `GND - VCC - D0 - D1 - RST - DC - CS`, they should work (or `GND - VCC - CLK - MOSI - RES - DC - CS`, which is the same). **make sure you get the right size**: 1.3" (not 0.96")! 
  - alternatively, [here](https://github.com/mxmxmx/O_C/tree/master/hardware/gerbers/128x64_1_3_oled) are **.brd/.sch** files for a/the OLED carrier board. in that case, you'd need to get the **bare** OLED (and some passives). [for example here](http://www.buydisplay.com/default/serial-spi-1-3-inch-128x64-oled-display-module-ssd1306-white-on-black) (though there's cheaper options for getting bare OLEDs).















