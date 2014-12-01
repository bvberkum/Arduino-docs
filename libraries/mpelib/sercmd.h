
typedef struct {
	char code;
	uint8_t bytes;
	void (*fun)(void);
} Commands;

void sercmd_init(int size);
void reset(void);
void process_uart(const Commands* cmds);
void uart_v_get(void* ptr, uint8_t len);
void uart_cchar_get(const char* v);

