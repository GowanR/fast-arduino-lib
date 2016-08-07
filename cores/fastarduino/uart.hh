#ifndef UART_HH
#define	UART_HH

#include "streams.hh"
#include "Board.hh"

// Only MCU with physical USART are supported (not ATtiny then)
#if defined(UCSR0A)

// This macro is internally used in further macros and should not be used in your programs
#define _USE_UART(NAME, INDEX)		\
ISR(NAME ## _RX_vect)				\
{									\
	UART_ReceiveComplete(INDEX);	\
}									\
ISR(NAME ## _UDRE_vect)				\
{									\
	UART_DataRegisterEmpty(INDEX);	\
}

// Those macros should be added somewhere in a cpp file (advised name: vectors.cpp) to indicate you
// use a given UART in your program hence you need the proper ISR vector correctly defined
#define USE_UART0()	_USE_UART(USART0, Board::USART::USART_0)
#define USE_UART1()	_USE_UART(USART1, Board::USART::USART_1)
#define USE_UART2()	_USE_UART(USART2, Board::USART::USART_2)
#define USE_UART3()	_USE_UART(USART3, Board::USART::USART_3)

//TODO check if inheritance can be made private (or protected) rather than public without defining a zillion friends...
//TODO Handle generic errors coming from UART TX (which errors?) in addition to internal overflow

//TODO improve performance and size (data and maybe code) by templatizing AFTER debug is done
class AbstractUART: private InputBuffer, private OutputBuffer
{
public:
	enum class Parity
	{
		NONE = 0x00,
		EVEN = _BV(UPM00),
		ODD = _BV(UPM00) | _BV(UPM01)
	};
	enum class StopBits
	{
		ONE = 0x00,
		TWO = _BV(USBS0)
	};

	template<uint8_t SIZE_RX, uint8_t SIZE_TX>
	AbstractUART(Board::USART usart, char (&input)[SIZE_RX], char (&output)[SIZE_TX])
	:	InputBuffer{input, true}, OutputBuffer{output},
		_usart(usart), _transmitting(false)
	{
		_uart[(uint8_t) usart] = this;
	}
	
	void begin(uint32_t rate, Parity parity = Parity::NONE, StopBits stop_bits = StopBits::ONE);
	void end();
	
	InputBuffer& in()
	{
		return (InputBuffer&) *this;
	}
	
	FormattedInput<InputBuffer> fin()
	{
		return FormattedInput<InputBuffer>(*this);
	}
	
	OutputBuffer& out()
	{
		return (OutputBuffer&) *this;
	}
	
	FormattedOutput<OutputBuffer> fout()
	{
		return FormattedOutput<OutputBuffer>(*this);
	}
	
protected:
	// Listeners of events on the buffer
	virtual void on_flush();
	
private:
	const Board::USART _usart;
	bool _transmitting;
	
	//TODO replace with global InputBuffer/OutputBuffer ?
	static AbstractUART* _uart[Board::USART_MAX];
	//TODO and then remove those stupid friends from here
	friend void UART_DataRegisterEmpty(Board::USART usart);
	friend void UART_ReceiveComplete(Board::USART usart);
};

void UART_DataRegisterEmpty(Board::USART usart);
void UART_ReceiveComplete(Board::USART usart);

//Not sure we need a template...
//template<Board::USART USART>
//class UART: private AbstractUART
//{
//public:
//	UART(InputBuffer& input, OutputBuffer& output):AbstractUART(input, output) {}
//	void begin();
//	void end();
//	
//private:
//	static const constexpr volatile uint8_t& UCSRA = *Board::UCSRA(USART);
//	static const constexpr volatile uint8_t& UCSRB = *Board::UCSRB(USART);
//	static const constexpr volatile uint8_t& UCSRC = *Board::UCSRC(USART);
//	static const constexpr volatile uint8_t& UBRR = *Board::UBRR(USART);
//	static const constexpr volatile uint8_t& UDR = *Board::UDR(USART);
//};

#endif

#endif	/* UART_HH */