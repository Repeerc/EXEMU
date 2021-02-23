#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "APBH.h"
#pragma pack(1)

uint32_t APBH_DMA_REGS[63];

uint32_t apbh_dma_reset(void* base) {
    memset(APBH_DMA_REGS, 0x0, sizeof(APBH_DMA_REGS));
}

uint32_t apbh_dma_read(void* base, uint32_t address) {
    return APBH_DMA_REGS[address >> 4];
}

void apbh_dma_write(void* base, uint32_t address, uint32_t data, uint8_t mask) {




    uint32_t* data_p;
    data_p = &APBH_DMA_REGS[address >> 4];

    //log_f("APBH REG:%x, Type:%x, data:%08x, mask:%d\r\n",address >> 4,address & 0xF,data,mask);

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

    if ((APBH_DMA_REGS[HW_APBH_CH0_SEMA] & 0xFF) > 0) {     //LCDIF_DMA
        hw_lcdif_DmaDesc *lcdif_dma_chain;
        extern uint8_t *VM_RAM;
        //log_f("LCDIF_DMA HW_APBH_CH0_SEMA:%08x\n", APBH_DMA_REGS[HW_APBH_CH0_SEMA]);
        //log_f("LCDIF_DMA HW_APBH_CH0_NXTCMDAR:%08x\n", APBH_DMA_REGS[HW_APBH_CH0_NXTCMDAR]);
        
        APBH_DMA_REGS[HW_APBH_CH0_CURCMDAR] = APBH_DMA_REGS[HW_APBH_CH0_NXTCMDAR];
        lcdif_dma_chain = (hw_lcdif_DmaDesc *)(&VM_RAM[APBH_DMA_REGS[HW_APBH_CH0_NXTCMDAR]]);
        
        //log_f("sz:%d\n",sizeof(lcdif_dma_chain->DMABytes));

        do {

            if (lcdif_dma_chain->Semaphore) {
                APBH_DMA_REGS[HW_APBH_CH0_SEMA]--;
            }

            log_f("LCDIF_DMA_BUFF_ADDR:%08x\n", lcdif_dma_chain->p_DMABuffer);
            log_f("LCDIF_DMA_BYTES:%08x\n", lcdif_dma_chain->DMABytes);
            log_f("p_Next:%08x\n", lcdif_dma_chain->Next);
            //log_f("APBH_DMA_REGS[HW_APBH_CH0_SEMA]:%08x\n", APBH_DMA_REGS[HW_APBH_CH0_SEMA]);

            if ((uint32_t)lcdif_dma_chain->Next == 0) {
                break;
            }
            lcdif_dma_chain = (hw_lcdif_DmaDesc*)((char*)get_phy_mem_addr() + (uint32_t)lcdif_dma_chain->Next);

        } while (APBH_DMA_REGS[HW_APBH_CH0_SEMA] & 0xFF);

    }

    //log_f("HW_ICOLL_VBASE:%08x\n", ICOLL_REGS[HW_ICOLL_VBASE]);

}

void apbh_dma_exit(void* base) {

}

