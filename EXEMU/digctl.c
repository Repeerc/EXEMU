#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "digctl.h"

uint32_t DIGCTL_REGS[64];

LARGE_INTEGER start_time;
LARGE_INTEGER counter_freq;
uint32_t digctl_reset(void* base) {
    memset(DIGCTL_REGS, 0x0, sizeof(DIGCTL_REGS));
    QueryPerformanceFrequency(&counter_freq);
    log_f("Counter Frequence:%d Hz\n",counter_freq.QuadPart);
    QueryPerformanceCounter(&start_time);
}



uint32_t digctl_read(void* base, uint32_t address) {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    DIGCTL_REGS[HW_DIGCTL_MICROSECONDS] = (time.QuadPart - start_time.QuadPart)/(double)counter_freq.QuadPart*1e6;

    switch (address >> 4) {
    case HW_DIGCTL_MICROSECONDS:
       
        return  DIGCTL_REGS[HW_DIGCTL_MICROSECONDS];
    }


    return DIGCTL_REGS[address >> 4];
}

void digctl_write(void* base, uint32_t address, uint32_t data, uint8_t mask) {




    uint32_t* data_p;
    data_p = &DIGCTL_REGS[address >> 4];

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

void digctl_exit(void* base) {

}

