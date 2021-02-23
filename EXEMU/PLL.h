#pragma once

uint32_t pll_reset(void* base);

uint32_t pll_read(void* base, uint32_t address);

void pll_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void pll_exit(void* base);
