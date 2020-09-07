#include <sys/uart.h>
#include <sys/soc.h>
#include <sys/sysio.h>
#include <mc/string.h>
#include <mc/stdio.h>

#define GPFSEL1     (CPU_IO_BASE + 0x200004)
#define GPSET0      (CPU_IO_BASE + 0x20001C)
#define GPCLR0      (CPU_IO_BASE + 0x200028)
#define GPPUD       (CPU_IO_BASE + 0x200094)
#define GPPUDCLK0   (CPU_IO_BASE + 0x200098)

#define UART0_BASE   (CPU_IO_BASE + 0x201000)
#define UART0_DR     (UART0_BASE + 0x00)
#define UART0_RSRECR (UART0_BASE + 0x04)
#define UART0_FR     (UART0_BASE + 0x18)
#define UART0_ILPR   (UART0_BASE + 0x20)
#define UART0_IBRD   (UART0_BASE + 0x24)
#define UART0_FBRD   (UART0_BASE + 0x28)
#define UART0_LCRH   (UART0_BASE + 0x2C)
#define UART0_CR     (UART0_BASE + 0x30)
#define UART0_IFLS   (UART0_BASE + 0x34)
#define UART0_IMSC   (UART0_BASE + 0x38)
#define UART0_RIS    (UART0_BASE + 0x3C)
#define UART0_MIS    (UART0_BASE + 0x40)
#define UART0_ICR    (UART0_BASE + 0x44)
#define UART0_DMACR  (UART0_BASE + 0x48)
#define UART0_ITCR   (UART0_BASE + 0x80)
#define UART0_ITIP   (UART0_BASE + 0x84)
#define UART0_ITOP   (UART0_BASE + 0x88)
#define UART0_TDR    (UART0_BASE + 0x8C)


static void DUMMY(uint32_t value )
{
    (void) value;
}

void uart_init()
{
    uint32_t ra;

    PUT32(UART0_CR,0);

    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=4<<12;    //alt0
    ra&=~(7<<15); //gpio15
    ra|=4<<15;    //alt0
    PUT32(GPFSEL1,ra);

    PUT32(GPPUD,0);
    for(ra=0;ra<150;ra++) DUMMY(ra);
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(ra=0;ra<150;ra++) DUMMY(ra);
    PUT32(GPPUDCLK0,0);

    PUT32(UART0_ICR,0x7FF);
    PUT32(UART0_IBRD,1);
    PUT32(UART0_FBRD,40);
    PUT32(UART0_LCRH,0x70);
    PUT32(UART0_CR,0x301);
}

void uart_putc( uint8_t c )
{
    while(1)
    {
        if((GET32(UART0_FR)&0x20)==0) break;
    }
    PUT32(UART0_DR,c);
}

uint8_t uart_getc()
{
    while(1)
    {
        if((GET32(UART0_FR)&0x10)==0) break;
    }
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