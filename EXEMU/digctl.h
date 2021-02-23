#pragma once


#define HW_DIGCTL_MICROSECONDS		0xC


uint32_t digctl_reset(void* base);

uint32_t digctl_read(void* base, uint32_t address);

void digctl_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void digctl_exit(void* base);
