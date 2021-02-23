#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "pinctrl.h"


uint32_t PINCTRL_REGS[0xB2];

uint32_t pinctrl_reset(void* base) {
    memset(PINCTRL_REGS, 0x0, sizeof(PINCTRL_REGS));
}

uint32_t pinctrl_read(void* base, uint32_t address) {
    return PINCTRL_REGS[address >> 4];
}

void pinctrl_write(void* base, uint32_t address, uint32_t data, uint8_t mask) {




    uint32_t* data_p;
    data_p = &PINCTRL_REGS[address >> 4];

    //log_f("ICOLL REG:%x, Type:%x, data:%08x, mask:%d\r\n",address >> 4,address & 0xF,data,mask);

    switch (mask) {
    case 3:
    {
        uint32_t* dp = data_p;
        switch (address & 0xF)
        {
        case 0x8:
            *dp &= ~(data);
            break;
        case 0xC:
            *dp ^= data;
            break;
        default:
            *dp = data;
            break;
        }
    }
    break;
    case 1:
    {
        uint16_t* dp = data_p;
        switch (address & 0xF)
        {
        case 0x8:
            *dp &= ~(data);
            break;
        case 0xC:
            *dp ^= data;
            break;
        default:
            *dp = data;
            break;
        }
    }
    break;
    default:
    {
        uint8_t* dp = data_p;
        switch (address & 0xF)
        {
        case 0x8:
            *dp &= ~(data);
            break;
        case 0xC:
            *dp ^= data;
            break;
        default:
            *dp = data;
            break;
        }
    }
    break;
    }

    //log_f("HW_ICOLL_VBASE:%08x\n", ICOLL_REGS[HW_ICOLL_VBASE]);

}

void pinctrl_exit(void* base) {

}

