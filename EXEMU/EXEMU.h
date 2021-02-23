#pragma once

#include "resource.h"

void log(char* text);
void log(const char text[]);

#ifdef __cplusplus
extern "C" {
#endif

	int log_f(const char* f, ...);
	void uart_output(char* text);
	void uart_putc(char c);
	int reg_dump_log_f(const char* f, ...);
	void clr_reg_dump_text();
	void screen_point(int x, int y, int c);
	void screen_flush();

#ifdef __cplusplus
}
#endif