#pragma once

uint32_t uart_reset(void* base);

uint32_t uart_read(void* base, uint32_t address);

void uart_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void uart_exit(void* base);
