#pragma once

uint32_t power_reset(void* base);

uint32_t power_read(void* base, uint32_t address);

void power_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void power_exit(void* base);
