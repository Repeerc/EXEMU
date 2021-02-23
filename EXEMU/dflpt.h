#pragma once

uint32_t dflpt_ctl_reset(void* base);

uint32_t dflpt_ctl_read(void* base, uint32_t address);

void dflpt_ctl_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void dflpt_ctl_exit(void* base);

uint32_t dflpt_reset(void* base);

uint32_t dflpt_read(void* base, uint32_t address);

void dflpt_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void dflpt_exit(void* base);
