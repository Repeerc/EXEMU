#pragma once
#pragma pack(4)

typedef union
{
    uint32_t U : 32;
    struct
    {
        unsigned COUNT : 16;
        unsigned RUN : 1;
        unsigned WORD_LENGTH : 1;
        unsigned DATA_SELECT : 1;
        unsigned DOTCLK_MODE : 1;
        unsigned VSYNC_MODE : 1;
        unsigned DATA_SWIZZLE : 2;
        unsigned BYPASS_COUNT : 1;
        unsigned DVI_MODE : 1;
        unsigned SHIFT_NUM_BITS : 2;
        unsigned DATA_SHIFT_DIR : 1;
        unsigned WAIT_FOR_VSYNC_EDGE : 1;
        unsigned READ_WRITEB : 1;
        unsigned CLKGATE : 1;
        unsigned SFTRST : 1;
    } B;
} hw_lcdif_ctrl_t;

typedef enum hw_lcdif_DmaCommand {
    LCDIF_NO_DMA_XFER = 0,
    LCDIF_DMA_WRITE = 1, //从设备写到内存
    LCDIF_DMA_READ = 2   //从内存读到设备
} hw_lcdif_DmaCommand;


typedef struct _hw_lcdif_DmaDesc //用于LCD屏幕控制器的DMA描述符
{
    uint32_t Next : 32;
    union {
        struct
        {
            union {
                struct
                {
                    uint8_t Command : 2;
                    uint8_t Chain : 1;
                    uint8_t IRQOnCompletion : 1;
                    uint8_t NANDLock : 1;
                    uint8_t NANDWaitForReady : 1;
                    uint8_t Semaphore : 1;
                    uint8_t WaitForEndCommand : 1;
                };
                uint8_t Bits;
            };
            uint8_t Reserved : 4;
            uint8_t PIOWords : 4;
            uint16_t DMABytes : 16;
        };
        uint32_t CommandBits : 32;
    };
    uint32_t p_DMABuffer : 32;
    hw_lcdif_ctrl_t PioWord;

} hw_lcdif_DmaDesc;

#define HW_APBH_CH0_CURCMDAR	0x4
#define HW_APBH_CH0_NXTCMDAR	0x5
#define HW_APBH_CH0_CMD			0x6
#define HW_APBH_CH0_BAR			0x7
#define HW_APBH_CH0_SEMA		0x8


uint32_t apbh_dma_reset(void* base);

uint32_t apbh_dma_read(void* base, uint32_t address);

void apbh_dma_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void apbh_dma_exit(void* base);
