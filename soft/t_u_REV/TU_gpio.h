#ifndef TU_GPIO_H_
#define TU_GPIO_H_

#define CV1 A5
#define CV2 A4
#define CV3 A6
#define CV4 A3

#define TR1 0
#define TR2 1 // reset

#define CLK1 5
#define CLK2 6
#define CLK3 7
#define CLK4 A14 // alt = 29
#define CLK5 8
#define CLK6 2

#define but_top 3
#define but_bot 12

#define OLED_DC 9
#define OLED_RST 4
#define OLED_CS 10

// OLED CS is active low
#define OLED_CS_ACTIVE LOW
#define OLED_CS_INACTIVE HIGH

#define encR1 15
#define encR2 16
#define butR  13

#define encL1 22
#define encL2 21
#define butL  23

// NOTE: back side :(
#define TU_GPIO_DEBUG_PIN1 30
#define TU_GPIO_DEBUG_PIN2 29

#define TU_GPIO_BUTTON_PINMODE INPUT_PULLUP
#define TU_GPIO_TRx_PINMODE INPUT_PULLUP
#define TU_GPIO_ENC_PINMODE INPUT_PULLUP
#define TU_GPIO_OUTPUTS_PINMODE OUTPUT

#endif // TU_GPIO_H_
