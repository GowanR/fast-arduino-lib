/*
 * LED chaser, with input settings
 * This program shows usage of FastArduino port API to handle several output at a time, plus individual input pins.
 * On Arduino, you should branch LED (in series with 330 Ohm resistors to limit current) on the following pins:
 * - D0-D7
 * i.e. all pins mapped to AVR ATmega328 PORT D
 * Then, you should branch 4 switches (I use DIP switches which are convenient on breadboard):
 * - one side to A0-A2 (number of simultaneously lit LED) and A3 (chase direction)
 * - the other side to GND (we use internal pullup resistors for inputs)
 */

#include <avr/interrupt.h>
#include <util/delay.h>
#include <fastarduino/IO.hh>

static inline uint8_t shift_pattern(uint8_t pattern, uint8_t shift)
{
	uint16_t result = (pattern << shift);
	return result | (result >> 8);
}

static inline uint8_t calculate_pattern(uint8_t num_bits)
{
	uint16_t pattern = (1 << (num_bits + 1)) - 1;
	return pattern;
}

const uint8_t NUM_LEDS = 8;

int main()
{
	// Enable interrupts at startup time
	sei();
	// Prepare ports to write to LEDs
	IOMaskedPort pins[NUM_LEDS];
	for (uint8_t i = 0; i < NUM_LEDS; ++i)
	{
		pins[i] = IOMaskedPort{Board::PORT_D, (1 << i), 0xFF};
	}
	
	// Loop of the LED chaser
	while (true)
	{
		for (uint8_t i = 0; i < 8; ++i)
		{
			pins[i].set_PORT(0xFF);
			_delay_ms(250.0);
			pins[i].set_PORT(0x00);
			_delay_ms(250.0);
		}
	}
	return 0;
}