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
 * Asynchronous sonar sensor read and conversion.
 * This program shows usage of FastArduino HCSR04 device API with PCINT ISR on 2 pins.
 * In this example, both sonar devices use the same TRIGGER pin.
 * 
 * Wiring: TODO
 * - on ATmega328P based boards (including Arduino UNO):
 * - on Arduino MEGA:
 * - on ATtinyX4 based boards:
 *   - D1: TX output connected to Serial-USB allowing traces display on a PC terminal
 */

#include <fastarduino/boards/board.h>
#include <fastarduino/gpio.h>
#include <fastarduino/time.h>
#include <fastarduino/timer.h>
#include <fastarduino/flash.h>
#include <fastarduino/pci.h>
#include <fastarduino/devices/hcsr04.h>

#if defined(ARDUINO_UNO) || defined(BREADBOARD_ATMEGA328P) || defined(ARDUINO_NANO)
#define HARDWARE_UART 1
#include <fastarduino/uart.h>
static constexpr const board::USART UART = board::USART::USART0;
static constexpr const uint8_t OUTPUT_BUFFER_SIZE = 64;
REGISTER_UATX_ISR(0)
#define TIMER_NUM 1
static constexpr const board::Timer TIMER = board::Timer::TIMER1;
#define PCI_NUM 2
static constexpr const board::DigitalPin TRIGGER = board::DigitalPin::D2_PD2;
static constexpr const board::DigitalPin ECHO = board::InterruptPin::D3_PD3_PCI2;
#elif defined (ARDUINO_MEGA)
#define HARDWARE_UART 1
#include <fastarduino/uart.h>
static constexpr const board::USART UART = board::USART::USART0;
static constexpr const uint8_t OUTPUT_BUFFER_SIZE = 64;
REGISTER_UATX_ISR(0)
#define TIMER_NUM 1
static constexpr const board::Timer TIMER = board::Timer::TIMER1;
#define PCI_NUM 0
static constexpr const board::DigitalPin TRIGGER = board::DigitalPin::D2_PE4;
static constexpr const board::DigitalPin ECHO = board::InterruptPin::D53_PB0_PCI0;
#elif defined(ARDUINO_LEONARDO)
#define HARDWARE_UART 1
#include <fastarduino/uart.h>
static constexpr const board::USART UART = board::USART::USART1;
static constexpr const uint8_t OUTPUT_BUFFER_SIZE = 64;
REGISTER_UATX_ISR(1)
#define TIMER_NUM 1
static constexpr const board::Timer TIMER = board::Timer::TIMER1;
#define PCI_NUM 0
static constexpr const board::DigitalPin TRIGGER = board::DigitalPin::D2_PD1;
static constexpr const board::DigitalPin ECHO = board::InterruptPin::D8_PB4_PCI0;
#elif defined(BREADBOARD_ATTINYX4)
#define HARDWARE_UART 0
#include <fastarduino/soft_uart.h>
static constexpr const board::DigitalPin TX = board::DigitalPin::D8_PB0;
static constexpr const uint8_t OUTPUT_BUFFER_SIZE = 64;
#define TIMER_NUM 1
static constexpr const board::Timer TIMER = board::Timer::TIMER1;
#define PCI_NUM 1
static constexpr const board::DigitalPin TRIGGER = board::DigitalPin::D0_PA0;
static constexpr const board::DigitalPin ECHO = board::InterruptPin::D10_PB2_PCI1;
#else
#error "Current target is not yet supported!"
#endif

// Buffers for UART
static char output_buffer[OUTPUT_BUFFER_SIZE];

using TIMER_TYPE = timer::Timer<TIMER>;
using CALC = timer::Calculator<TIMER>;
using devices::sonar::SonarType;
using SONAR = devices::sonar::HCSR04<TIMER, TRIGGER, ECHO, SonarType::ASYNC_PCINT>;
static constexpr const uint32_t PRECISION = SONAR::DEFAULT_TIMEOUT_MS * 1000UL;
static constexpr const TIMER_TYPE::PRESCALER PRESCALER = CALC::CTC_prescaler(PRECISION);
static constexpr const SONAR::TYPE TIMEOUT = CALC::us_to_ticks(PRESCALER, PRECISION);

using devices::sonar::echo_us_to_distance_mm;
using devices::sonar::distance_mm_to_echo_us;

static constexpr const uint16_t DISTANCE_THRESHOLD_MM = 150;

class SonarListener
{
public:
	SonarListener(uint16_t min_mm)
	:	MIN_TICKS{CALC::us_to_ticks(PRESCALER, distance_mm_to_echo_us(min_mm))}, 
		led_{gpio::PinMode::OUTPUT}
	{
		interrupt::register_handler(*this);
	}
	
	void on_sonar(SONAR::TYPE echo_ticks)
	{
		if (echo_ticks && echo_ticks <= MIN_TICKS)
			led_.set();
		else
			led_.clear();
	}
	
private:
	const SONAR::TYPE MIN_TICKS;
	gpio::FastPinType<board::DigitalPin::LED>::TYPE led_;
};

REGISTER_HCSR04_PCI_ISR_METHOD(TIMER, PCI_NUM, TRIGGER, ECHO, SonarListener, &SonarListener::on_sonar)

int main() __attribute__((OS_main));
int main()
{
	board::init();
	sei();
	
#if HARDWARE_UART
	serial::hard::UATX<UART> uart{output_buffer};
	uart.register_handler();
#else
	serial::soft::UATX<TX> uart{output_buffer};
#endif
	uart.begin(115200);
	auto out = uart.fout();
	
	SonarListener listener{DISTANCE_THRESHOLD_MM};
	TIMER_TYPE timer{timer::TimerMode::NORMAL, PRESCALER};
	timer.begin();
	SONAR sonar{timer};
	sonar.register_handler();

	typename interrupt::PCIType<ECHO>::TYPE signal;
	signal.enable_pin<ECHO>();
	signal.enable();
	
	out << F("Starting...\n") << streams::flush;
	
	while (true)
	{
		sonar.async_echo();
		SONAR::TYPE pulse = sonar.await_echo_ticks(TIMEOUT);
		uint32_t us = CALC::ticks_to_us(PRESCALER, pulse);
		uint16_t mm = echo_us_to_distance_mm(us);
		// trace value to output
		out << F("Pulse: ") << pulse << F(" ticks, ") << us << F("us. Distance: ") << mm << F("mm\n") << streams::flush;
		time::delay_ms(1000);
	}
}
