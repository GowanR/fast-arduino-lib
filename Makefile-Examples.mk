#   Copyright 2016-2017 Jean-Francois Poilpret
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# Makefile defining all FastArduino examples to build in various configurations (targets)

# Examples common to all targets
#FIXME this list is in fact wrong because most examples have been set to work only on UNO,MEGA,ATmega328 and ATtiny
# Not all examples have been updated when LEONARDO and NANO boards have been added to FastArduino support
# For NANO, adding support is straightforward (same as UNO) but not for LENOARDO
COMMON_EXAMPLES=	analog/AnalogPin1 analog/AnalogPin2					\
			eeprom/Eeprom1 eeprom/Eeprom2 eeprom/Eeprom3 eeprom/Eeprom4		\
			events/EventApp1 events/EventApp2 events/EventApp3 events/EventApp4	\
			int/ExternalInterrupt1 int/ExternalInterrupt2				\
			io/FastPin1 io/FastPin2 io/FastPin3 io/FastPin4 io/FastPin5		\
			misc/Flash1								\
			pci/PinChangeInterrupt1 pci/PinChangeInterrupt2 pci/PinChangeInterrupt3	\
			analog/PWM1 analog/PWM2 analog/PWM3 analog/PWM4				\
			rtt/RTTApp1b rtt/RTTApp2 rtt/RTTApp3 rtt/RTTApp4			\
			motors/Servo1								\
			rtt/TimerApp3 rtt/TimerApp4						\
			spi/RF24App1 spi/RF24App2 spi/WinBond					\
			uart/UartApp2 uart/UartApp3 uart/UartApp4

EXAMPLES_ARDUINO_UNO=		complete/Conway							\
				int/ExternalInterrupt3						\
				pci/PinChangeInterrupt4						\
				uart/UartApp1							\
				uart/UartApp5							\
				uart/UartApp6							

EXAMPLES_ARDUINO_LEONARDO=	complete/Conway							\
				int/ExternalInterrupt3						\
				uart/UartApp1							

EXAMPLES_ARDUINO_MEGA=		int/ExternalInterrupt3						\
				pci/PinChangeInterrupt4						\
				uart/UartApp1							\
				uart/UartApp5							\
				uart/UartApp6							

EXAMPLES_ARDUINO_NANO=		uart/UartApp1							\
				uart/UartApp5							\
				uart/UartApp6							

EXAMPLES_BREADBOARD_ATMEGA328P=	int/ExternalInterrupt3						\
				pci/PinChangeInterrupt4						\
				uart/UartApp1							\
				uart/UartApp5							\
				uart/UartApp6							

EXAMPLES_BREADBOARD_ATTINYX4=	complete/Conway							\
				pci/PinChangeInterrupt4						

#TODO add missing AccelGyro1 AccelGyro2 DS1307RTC1 DS1307RTC2 Magneto1 Servo1 Servo2 Sonar1 Sonar2 Sonar3 Sonar4 Sonar5 Sonar6

# Finally define all examples supported for the current variant (defined by current configuration)
ALL_EXAMPLES = ${COMMON_EXAMPLES} ${EXAMPLES_${VARIANT}}

# Special build target for all fastArduino examples
examples: build
	$(foreach example, $(ALL_EXAMPLES), $(MAKE) -C examples/$(example) CONF=${CONF};)

clean-examples: clean
	$(foreach example, $(ALL_EXAMPLES), $(MAKE) -C examples/$(example) CONF=${CONF} clean;)