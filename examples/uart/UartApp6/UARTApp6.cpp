//   Copyright 2016-2017 Jean-Francois Poilpret
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

/*
 * Hardware UART example.
 * This program demonstrates usage of FastArduino Hardware UART support (on target supporting it) and formatted
 * input streams.
 * 
 * Wiring:
 * - on Arduino UNO, Arduino NANO and Arduino MEGA:
 *   - Use standard TX/RX
 * - on ATmega328P based boards:
 *   - Use standard TX/RX connected to an Serial-USB converter
 * - on ATtinyX4 based boards:
 *   - NOT SUPPORTED
 */

#include <fastarduino/time.h>
#include <fastarduino/uart.h>

// Define vectors we need in the example
REGISTER_UART_ISR(0)

// Buffers for UART
static const uint8_t INPUT_BUFFER_SIZE = 64;
static const uint8_t OUTPUT_BUFFER_SIZE = 64;
static char input_buffer[INPUT_BUFFER_SIZE];
static char output_buffer[OUTPUT_BUFFER_SIZE];

using namespace streams;

using INPUT = FormattedInput<InputBuffer>;
using OUTPUT = FormattedOutput<OutputBuffer>;

template<typename T>
static void handle(OUTPUT& out, INPUT& in, const flash::FlashStorage* type)
{
	out << type << F(": ") << flush;
	T value{};
	in >> skipws >> value;
	out << value << endl;
}

template<typename T>
static void display_num(OUTPUT& out, T value)
{
	out << bin << value << endl;
	out << dec << value << endl;
	out << oct << value << endl;
	out << hex << value << endl;
}

template<typename T>
static void handle_num(OUTPUT& out, T value, const flash::FlashStorage* type)
{
	out << F("testing output of ") << type << F(" (") << dec << value << ')' << endl;
	display_num<T>(out, value);

	out << showbase;
	display_num<T>(out, value);
	out << noshowbase;

	out << uppercase;
	display_num<T>(out, value);
	out << nouppercase;

	out << uppercase << showbase;
	display_num<T>(out, value);
	out << nouppercase << noshowbase;

	out << showpos;
	display_num<T>(out, value);
	out << noshowpos;
}

int main() __attribute__((OS_main));
int main()
{
	// Enable interrupts at startup time
	sei();
	
	// Start UART
	serial::hard::UART<board::USART::USART0> uart{input_buffer, output_buffer};
	uart.register_handler();
	uart.begin(115200);
	INPUT in = uart.fin();
	OUTPUT out = uart.fout();

	// Check all output manipulators
	handle_num<uint16_t>(out, 1234, F("uint16_t"));
	handle_num<int16_t>(out, 1234, F("int16_t"));
	handle_num<int16_t>(out, -1234, F("int16_t"));

	handle_num<uint32_t>(out, 123456, F("uint32_t"));
	handle_num<int32_t>(out, 123456, F("int32_t"));
	handle_num<int32_t>(out, -123456, F("int32_t"));

	//TODO check floats
	//TODO check other types

	//TODO check justification: setw(), setfill(), left, right...

	// Event Loop
	while (true)
	{
		handle<char>(out, in, F("char"));
		handle<uint16_t>(out, in, F("uint16_t"));
		handle<int16_t>(out, in, F("int16_t"));
		handle<uint32_t>(out, in, F("uint32_t"));
		handle<int32_t>(out, in, F("int32_t"));
		handle<bool>(out, in, F("bool"));
		
		time::delay_ms(1000);
	}
}
