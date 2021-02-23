#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "dflpt.h"
#include "UART.h"

uint32_t HW_UARTDBGDR;
uint32_t HW_UARTDBGFR;

uint32_t uart_reset(void* base) {
	HW_UARTDBGDR = 0;
    HW_UARTDBGFR = 0;
}

uint32_t uart_read(void* base, uint32_t address) {
    switch (address)
    {
    case 0:
        return HW_UARTDBGDR;
    case 0x18:
        return HW_UARTDBGFR;
    default:
        return 0;
    }

}

void uart_write(void* base, uint32_t address, uint32_t data, uint8_t mask) {
    int* data_p;
    switch (address)
    {
    case 0:
        data_p = &HW_UARTDBGDR;
        break;
    case 0x18:
        data_p = &HW_UARTDBGFR;
        break;
    default:
        return;
    }


    switch (mask) {
    case 3:
    {
        *data_p = data;
    }
    break;
    case 1:
    {
        //short* data_p = (short*)(&HW_UARTDBGDR + address);
        *data_p &= 0xFFFF0000;
        *data_p |= (data & 0xFFFF);
    }
    break;
    default:
    {
        *data_p &= 0xFFFFFF00;
        *data_p |= (data & 0xFF);
    }
    break;
    }
    uart_putc(HW_UARTDBGDR);
}

void uart_exit(void* base) {

}

