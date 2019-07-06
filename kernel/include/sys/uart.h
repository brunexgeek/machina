#ifndef MACHINA_UART_HH
#define MACHINA_UART_HH


#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

void uart_init();
void uart_putc( uint8_t c );
//void uart_puts( const char *str );
void uart_puts( const char16_t *str );
void uart_print( const char16_t *format, ... );
uint8_t uart_getc();

#ifdef __cplusplus
}
#endif

#endif // MACHINA_UART_HH
