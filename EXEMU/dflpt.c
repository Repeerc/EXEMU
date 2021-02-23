
#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "dflpt.h"

#define DFLPT_RAM_SIZE  4096*4

char dflpt_ctl[0x80];

char* DFLPT = NULL;

uint32_t dflpt_ctl_reset(void* base) {
    memset(dflpt_ctl, 0, sizeof(dflpt_ctl));
}

uint32_t dflpt_ctl_read(void* base, uint32_t address) {

    return *((uint32_t *)&dflpt_ctl[address]);
}

void dflpt_ctl_write(void* base, uint32_t address, uint32_t data, uint8_t mask) {
    data &= 0xFFF;
    //log_f("dflpt_ctl_write:%08X %08X\r\n",address,data);
    switch (mask) {
    case 3:
    {
        int* data_p = (int*)(dflpt_ctl + address);
        *data_p = data;
    }
    break;
    case 1:
    {
        short* data_p = (short*)(dflpt_ctl + address);
        *data_p = data;
    }
    break;
    default:
    {
        char* data_p = (char*)(dflpt_ctl + address);
        *data_p = data;
    }
    break;
    }
}

void dflpt_ctl_exit(void* base) {

}

uint32_t dflpt_reset(void* base) {
    if (DFLPT == NULL) {
        DFLPT = calloc(DFLPT_RAM_SIZE, sizeof(uint8_t));
    }
    else {
        free(DFLPT);
        DFLPT = calloc(DFLPT_RAM_SIZE, sizeof(uint8_t));
    }
    if (DFLPT == NULL) {
        log_f("DFLPT RAM Alloc Error.\n");
        return 0;
    }

    *((unsigned int*)&DFLPT[0x2000]) = 0x80000C12;
    return 1;
}

uint32_t dflpt_read(void* base, uint32_t address)
{
    //log_f("dflpt mem read:%08x %08x\r\n", address, *((unsigned int*)&DFLPT[address]));
    if (address == 0x800 * sizeof(uint32_t)) {
        return *((unsigned int*)&DFLPT[0x2000]);
    }

    for (int i = 0; i < 8; i++) {
        if ( *((uint32_t*)&dflpt_ctl[i * 0x10])<< 2 == address) {
            return *((unsigned int*)&DFLPT[address]);
        }
    }
    //log_f("DFLPT READ RETURN 0\r\n");
    return 0;
}

void dflpt_write(void* base, uint32_t address, uint32_t data, uint8_t mask)
{
    //log_f("io mem write:%08x %08x\r\n",address,data);

    switch (mask) {
    case 3:
    {
        int* data_p = (int*)(DFLPT + address);
        *data_p = data;
    }
    break;
    case 1:
    {
        short* data_p = (short*)(DFLPT + address);
        *data_p = data;
    }
    break;
    default:
    {
        char* data_p = (char*)(DFLPT + address);
        *data_p = data;
    }
    break;
    }

}

void dflpt_exit(void* base) {
    free(DFLPT);
    DFLPT = NULL;
}
