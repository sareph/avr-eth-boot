#pragma once

#define AS110X_NOOP 0x00		// No Op
#define AS110X_DIG0 0x01		// Digit 0
#define AS110X_DIG1 0x02		// Digit 1
#define AS110X_DIG2 0x03		// Digit 2
#define AS110X_DIG3 0x04		// Digit 3
#define AS110X_DIG4 0x05		// Digit 4
#define AS110X_DIG5 0x06		// Digit 5
#define AS110X_DIG6 0x07		// Digit 6
#define AS110X_DIG7 0x08		// Digit 7
#define AS110X_DECODE_MODE 0x09	// Decode Mode
#define AS110X_INTENSITY_CTRL 0x0A	// Intensity Control
#define AS110X_SCAN_LIMIT 0x0B		// Scan Limit
#define AS110X_SHUTDOWN 0x0C		// Shutdown
#define AS110X_FEATURE 0x0E		// Features
#define AS110X_TEST 0x0F		// Test

#define AS110X_CS_PORT 	PORTB
#define AS110X_CS_DDR 	DDRB
#define AS110X_CS_PIN 	2

//						   ABCDEFG
#define AS110X_FONT_A	0b01110111
#define AS110X_FONT_C	0b01001110
#define AS110X_FONT_E	0b01001111
#define AS110X_FONT_h	0b00010111
#define AS110X_FONT_H	0b00110111
#define AS110X_FONT_o	0b01100011
#define AS110X_FONT_R	0b00000101

#define AS110X_CS_ON() AS110X_CS_PORT &= ~(1 << AS110X_CS_PIN)
#define AS110X_CS_OFF() AS110X_CS_PORT |= (1 << AS110X_CS_PIN)

extern void as110x_setFeature(uint8_t value);
extern void as110x_init();
extern void as110x_test(uint8_t bOn);
extern void as110x_setIntensity(uint8_t value);
extern void as110x_setDecodeMode(uint8_t value, int8_t disp);
extern void as110x_set(uint8_t dig, uint8_t value, int8_t disp);
extern void as110xCtrl(uint8_t r, uint8_t g, uint8_t b, uint8_t id);
extern void as110x_writeAll(uint8_t reg, uint8_t val);
extern void as110x_writeOne(int8_t disp, uint8_t reg, uint8_t value);