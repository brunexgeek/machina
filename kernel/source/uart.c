#include <sys/uart.h>
#include <sys/bcm2837.h>
#include <sys/sysio.h>
#include <mc/string.h>
#include <mc/stdio.h>

static void DUMMY(uint32_t value )
{
    (void) value;
}
/*
static inline void busy_wait( int32_t cycles )
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(cycles): [count]"0"(cycles) : "cc");
}*/

// mailbox message changing the clock rate of PL011 to 3MHz tag
volatile unsigned int  __attribute__((aligned(16))) mbox[9] =
{
    9*4, 0, 0x38002, 12, 8, 2, 3000000, 0 ,0
};

void uart_init()
{
    uint32_t flags;
    volatile uint32_t ra;

    // disable UART0
    PUT32(UART0_CR,0);

    // disable pull up/down for all GPIO pins; wait 150 cycles
    PUT32(GPPUD,0);
    for(ra=0;ra<150;ra++) DUMMY(ra);
    // disable pull up/down for pins 14 and 15; wait 150 cycles
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(ra=0;ra<150;ra++) DUMMY(ra);
    // write 0 to GPPUDCLK0 to make it take effect
    PUT32(GPPUDCLK0,0);
    // clear pending interrupts
    PUT32(UART0_ICR,0x7FF);

    // In Respberry PI 3 (and 4) the UART_CLOCK is system-clock dependent by default.
	// Set it to 3Mhz so that we can consistently set the baud rate
    // UART_CLOCK = 30000000;
    unsigned int r = (((unsigned int)(&mbox) & ~0xF) | 8);
    // wait until we can talk to the VC
    while ( GET32(SOC_MAILBOX_POLL) & 0x80000000 ) { }
    // send our message to property channel and wait for the response
    PUT32(SOC_MAILBOX_WRITE, r);
    while ( (GET32(SOC_MAILBOX_POLL) & 0x40000000) || GET32(SOC_MAILBOX_READ) != r ) { }

    // setup GPIO pints 14 and 15
    flags =GET32(GPFSEL1);
    flags &= ~((7 << 12)|(7 << 15)); // gpio14 and gpio15
    flags |= (2 << 12) | (2 << 15); // alt0
    PUT32(GPFSEL1,flags);

    // divider = 3000000 / (16 * 115200) = 1.627 = ~1
    PUT32(UART0_IBRD,1);
    // fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40
    PUT32(UART0_FBRD,40);
    // enable FIFO & 8 bit data transmission (1 stop bit, no parity)
    PUT32(UART0_LCRH,0x70);
    // mask all interrupts
    PUT32(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                  (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
    // enable UART0, receive & transfer part of UART
    PUT32(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc( uint8_t c )
{
    while((GET32(UART0_FR) & 0x20) != 0);
    PUT32(UART0_DR,c);
}

uint8_t uart_getc()
{
    while((GET32(UART0_FR) & 0x10) != 0);
    return (uint8_t) GET32(UART0_DR);
}

void uart_puts( const char *str )
{
    if (str == NULL || *str == 0) return;
    while (*str != 0)
    {
        uart_putc((uint8_t)*str);
        ++str;
    }
}

void _putchar( char c )
{
    uart_putc(c);
}

void uart_print( const char *format, ... )
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}