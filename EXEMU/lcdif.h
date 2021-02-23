#pragma once

uint32_t lcdif_reset(void* base);

uint32_t lcdif_read(void* base, uint32_t address);

void lcdif_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void lcdif_exit(void* base);
