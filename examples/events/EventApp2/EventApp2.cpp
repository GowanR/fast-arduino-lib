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
#include <fastarduino/Events.hh>
#include <fastarduino/watchdog.hh>

using namespace Events;

static const uint8_t EVENT_QUEUE_SIZE = 32;

class LedHandler: private IOPort
{
public:
	LedHandler() : IOPort{Board::PORT_D, 0xFF}, _value{0} {}
	void operator()(const Event& event)
	{
		UNUSED(event);
		if (_value == 0)
			_value = 0x01;
		else
			_value <<= 1;
		set_PORT(_value);
	}
	
private:
	uint8_t _value;
};

int main()
{
	// Enable interrupts at startup time
	sei();

	// Prepare event queue
	Event buffer[EVENT_QUEUE_SIZE];
	Queue<Event> event_queue = Queue<Event>::create<EVENT_QUEUE_SIZE>(buffer);
	
	// Prepare Dispatcher and Handlers
	Dispatcher dispatcher;
	FunctorHandler<LedHandler> handler{Type::WDT_TIMER, LedHandler{}};
	dispatcher.insert(handler);
	
	// Start watchdog
	Watchdog::set_event_queue(event_queue);
	Watchdog::begin(Watchdog::TO_1s);
	
	// Event Loop
	while (true)
	{
		Event event = event_queue.pull();
		dispatcher.dispatch(event);
	}
	return 0;
}
