#/* BOM for temps utile, rev1, rev1.b, rev1.c */

###OLED:

- you can find these **7 pin**, 1.3" displays on ebay, aliexpress and other places for < 10$. they'll be ok, as long as the description claims that they are `SH1106` (or `SSD1306`); **and** that the pinout is: 

  `GND - VCC - D0 - D1 - RST - DC - CS`, or 

  `GND - VCC - CLK - MOSI - RES - DC - CS`, or

  `GND - VDD - SCK - SDA - RES - DC - CS`

- **make sure you get the right size**: 1.3" (not 0.96")! and once you've received it, make sure the display is configured for **SPI** (see resistors/labels on the back side of the OLED).
- alternatively, [here](https://github.com/mxmxmx/O_C/tree/master/hardware/gerbers/128x64_1_3_oled) are **.brd/.sch** files for a/the OLED carrier board. in that case, you'd need to get the **bare** OLED (and some passives). [for example here](http://www.buydisplay.com/default/serial-spi-1-3-inch-128x64-oled-display-module-ssd1306-white-on-black) (though there's cheaper options for getting bare OLEDs).

###SMD resistors (0805):

| value | # | part | note |
| ---: | ---: | --- |  --- |
|100R |		 4x | 603-RC0805FR-07100RL | 1% |
| 510R |         2x | 603-RC0805JR-07510RL | or simply use **1k**, value not critical |
| 1k  |       7x | 603-RC0805FR-071KL | 1% |
| 1k2  |         1x | 71-CRCW08051K20FKEA | 1% |
| 1k8  |         1x | 71-CRCW0805-1.8K-E3 | 1% |
| 3k   |         2x | 660-RK73H2ATTD3001F | 1% |
| 3k9  |        2x | 660-RK73H2ATTD3901F | 1% |
| 6k8  |        1x | 660-RK73H2ATTD6801F | 1% |
| 10k  |         7x | 660-RK73H2ATTD1002F | 1% **(‡)** |
| 20k  |         6x  | 71-CRCW0805-20K-E3  | 1% **(‡)**| 
| 33k |          6x  | 660-RK73H2ATTD3302F | 1% |
| 100k |         10x | 660-RK73H2ATTD1003F | 1% |

- (‡) the (digital) clock outputs are **non-inverting op amps**, the values on the pcb (20k feedback, 10k to ground) will result in 3x gain, or 9.9v on the outputs. adjust, if you like. for example, 15k would give you 3.3v * (20k/15k + 1) = 7.7, etc.

###SMD caps (0805):

| value | # | part | note |
| ---: | ---: | --- | --- |
| 1n    | 2x  | 581-08055A102J | (NP0/C0G) (50V)|
| 3n3   |1x | 77-VJ0805A332JXATBC | (NP0/C0G) (50V)|
| 18n   | 1x | 81-GRM21B5C1H183JA1L | (NP0/C0G) (50V)|
| 100nF | 9x | 80-C0805C104K5R | (25V or better)|
| 470nF | 5x | 603-CC805ZRY5V9BB474 | (25V or better) |
| 1uF   | 2x | 581-08055G105ZAT2A | (25V or better)|
| 10uF  | 1x | 81-GRM21BR61E106KA3L | (16V or better; may be 1206)|

###SMD caps (0603):
| value | # | part | note |
| ---: | ---: | --- | --- |
|10n   | 4x | 81-GRM1885C1H103JA1D | (NP0/C0G) (16V or better)|

###ICs/semis:

| what | package | # | part | note |
| --- | --- | ---: | --- | --- |
| TL074 | SOIC-14 | 2x | 595-TL074CDR | output amplifier |
| MCP6004 | SOIC-14 | 1x | 579-MCP6004T-I/SL | CV input buffer |
| MMBT3904 | SOT-23 | 2x | 512-MMBT3904 | NPN |
| 1N5817 | DO-41 | 2x | 621-1N5817 | Schottky, reverse voltage protection|
| LM1117-5v0 | SOT-223 | 1x | 511-LD1117S50 | 5v LDO |
| LM4040-5.0 | SOT-23 |  1x |  926-LM4040DIM350NOPB | precision voltage reference, 5v0|
| fixed inductor, 10uH | 1206 | 1x | 81-LQH31MN100K03L | > 25mA |

###misc **through-hole/mechanical**:

| what | package | # | part | note |
| --- | --- | ---: | --- | --- |
| 22uF | 2.5mm | 2x | 647-UPM1V220MDD1TD |  electrolytic, 35V+ | 
| 470nF | 5mm |  1x | 505-MKS2C034701CJSSD | film/ceramic, 35V+ | 
| 78L33 |  TO-92 | 1x | 511-L78L33ACZ | 3v3 regulator |
|  jacks | - | 12x | [PJ301M-12](https://www.thonk.co.uk/shop/3-5mm-jacks/) | 'thonkiconn' (or kobiconn) | 
| encoder w/ switch | 9mm | 2x | 652-PEC11R-4220F-S24  | 20mm, D-shaft **(‡)** |
|  2x5 male header | 2.54mm |  1x | 649-67996-410HLF | euro power connector | 
|  1x7 female socket |  2.54mm  |  1x | 517-929870-01-07-RA | low profile socket (for OLED) | 
|  1x14 **+ 1** sockets+headers | 2.54mm | 2x | - | breakable, 'machine' pin (round pin) **(†)** | 
| tact switches | multimecs 5E/5G | 2x | 642-5GTH935 or 642-5ETH935 | - |
| + caps | multimecs 5E/5G | 2x | 642-1SS09-15.0, or -16.0| - |
| teensy 3.x | - | 1x | oshpark / mouser #: 485-2756  | cut the usb/power trace! |
| OLED, 1.3" | 7 pin | 1x | SH1106, 128x64 | `GND - VCC - D0 - D1 - RST - DC - CS` **(see note above)** |
| spacer | 10mm, 3M | 1x | 855-R30-1611000 | - | 
| jumpers | - | 1x | - | for `DAC`: use a piece of wire |

 
- (‡) rotary encoder w/ switch: alternatives include mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

- (†) also see the build guide: we need **14 pins** on either side **plus** the DAC pin, so best to simply use 14 on the one side, and 13 + 2 on the other. the PCBs holes are small, so best to use so-called "machine" pin sockets + pin strip to match.















