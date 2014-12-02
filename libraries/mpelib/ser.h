#define CHAR_NEWLINE '\n'
#define CHAR_RETURN '\r'
#define RETURN_NEWLINE "\r\n"


extern void uart0_println(char *line);
extern void uart0_putint(int value);
extern void print_value(char *id, int value);
extern void showNibble(char nibble);
extern void showByte(char value);
extern void uart_ok(void);

extern void mpelib_ser_start(void);
extern void mpelib_ser_announce(const char *sketch, int version);
