#pragma once

uint32_t pinctrl_reset(void* base);

uint32_t pinctrl_read(void* base, uint32_t address);

void pinctrl_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void pinctrl_exit(void* base);
