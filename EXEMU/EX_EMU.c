#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "EX_EMU.h"
#include "EXEMU.h"
#include "armv4.h"
#include "dflpt.h"
#include "UART.h"
#include "icoll.h"
#include "power.h"
#include "PLL.h"
#include "digctl.h"
#include "APBH.h"
#include "pinctrl.h"
#include "lcdif.h"

#define SIZEOF_PERIPHERAL_CONFIG(cfg)    (sizeof(cfg)/sizeof(struct peripheral_link_t))

#define RAM_SIZE        512*1024
#define ROM_LOAD_ADDR   0x0

struct armv4_cpu_t cpu;


uint8_t* VM_RAM = NULL;

char* get_phy_mem_addr() {
    return VM_RAM;
}

uint32_t memory_reset(void* base) {
    if (VM_RAM == NULL) {
        VM_RAM = calloc(RAM_SIZE, sizeof(uint8_t));
    }
    else {
        free(VM_RAM);
        VM_RAM = calloc(RAM_SIZE, sizeof(uint8_t));
    }
    if (VM_RAM == NULL) {
        log_f("Ram Alloc Error.\n");
        return 0;
    }


    FILE* rom_file;
    struct stat rom_file_stat;
    if (stat("rom.bin", &rom_file_stat) == -1) {
        log("Loading rom.bin fail. 1\r\n");
        return 1;
    }
    rom_file = fopen("rom.bin", "rb");
    if (rom_file == NULL) {
        log("Loading rom.bin fail. 2\r\n");
        return 1;
    }
    log_f("Rom Size:%d Bytes\r\n", rom_file_stat.st_size);
    fseek(rom_file, 0, SEEK_SET);
    log_f("Load ROM AT:%08x, Size:%08x\r\n", ROM_LOAD_ADDR, fread(&VM_RAM[ROM_LOAD_ADDR], sizeof(char), rom_file_stat.st_size, rom_file));



    return 1;
}

void memory_exit(void* base)
{
    free(VM_RAM);
    VM_RAM = NULL;
}

uint32_t memory_read(void* base, uint32_t address)
{
    //log_f("mem read:%08x %08x\r\n",address,*((unsigned int *)&VM_RAM[address]));
    return *((unsigned int*)&VM_RAM[address]);
}

void memory_write(void* base, uint32_t address, uint32_t data, uint8_t mask)
{
    switch (mask) {
    case 3:
    {
        int* data_p = (int*)(VM_RAM + address);
        *data_p = data;
    }
    break;
    case 1:
    {
        short* data_p = (short*)(VM_RAM + address);
        *data_p = data;
    }
    break;
    default:
    {
        char* data_p = (char*)(VM_RAM + address);
        *data_p = data;
    }
    break;
    }
}


struct peripheral_link_t peripheral_config[] = {
    {
        .name = "RAM",
        .mask = ~(RAM_SIZE - 1),
        .prefix = 0x00000000,
        .reg_base = 0x100,
        .reset = memory_reset,
        .read = memory_read,
        .write = memory_write,
        .exit = memory_exit
    },
    {
        .name = "DIGCTL_DFLPT_CTRL",
        .mask = ~(0x80 - 1),
        .prefix = 0x8001C400,
        .reg_base = 0,
        .reset = dflpt_ctl_reset,
        .read = dflpt_ctl_read,
        .write = dflpt_ctl_write,
        .exit = dflpt_ctl_exit
    },
    {
        .name = "DFLPT",
        .mask = ~(0x4000 - 1),
        .prefix = 0x800C0000,
        .reg_base = 0,
        .reset = dflpt_reset,
        .read = dflpt_read,
        .write = dflpt_write,
        .exit = dflpt_exit
    },
    {
        .name = "UART",
        .mask = ~(0x100 - 1),
        .prefix = 0x80070000,
        .reg_base = 0,
        .reset = uart_reset,
        .read = uart_read,
        .write = uart_write,
        .exit = uart_exit
    },
    {
        .name = "ICOLL",
        .mask = ~(0x200 - 1),
        .prefix = 0x80000000,
        .reg_base = 0,
        .reset = icoll_reset,
        .read = icoll_read,
        .write = icoll_write,
        .exit = icoll_exit
    },
    {
        .name = "POWER",
        .mask = ~(0x200 - 1),
        .prefix = 0x80044000,
        .reg_base = 0,
        .reset = power_reset,
        .read = power_read,
        .write = power_write,
        .exit = power_exit
    },
   {
        .name = "PLL",
        .mask = ~(0x200 - 1),
        .prefix = 0x80040000,
        .reg_base = 0,
        .reset = pll_reset,
        .read = pll_read,
        .write = pll_write,
        .exit = pll_exit
    },
   {
        .name = "DIGCTRL",
        .mask = ~(0x400 - 1),
        .prefix = 0x8001C000,
        .reg_base = 0,
        .reset = digctl_reset,
        .read = digctl_read,
        .write = digctl_write,
        .exit = digctl_exit
    },
   {
        .name = "APBH DMA",
        .mask = ~(0x400 - 1),
        .prefix = 0x80004000,
        .reg_base = 0,
        .reset = apbh_dma_reset,
        .read = apbh_dma_read,
        .write = apbh_dma_write,
        .exit = apbh_dma_exit
    },
   {
        .name = "PINCTRL",
        .mask = ~(0xFFF - 1),
        .prefix = 0x80018000,
        .reg_base = 0,
        .reset = pinctrl_reset,
        .read = pinctrl_read,
        .write = pinctrl_write,
        .exit = pinctrl_exit
    },
   {
        .name = "LCDIF",
        .mask = ~(0x100 - 1),
        .prefix = 0x80030000,
        .reg_base = 0,
        .reset = lcdif_reset,
        .read = lcdif_read,
        .write = lcdif_write,
        .exit = lcdif_exit
    }



};

DWORD WINAPI emu_thread(LPVOID p) {

    cpu_init(&cpu);
    peripheral_register(&cpu, &peripheral_config, SIZEOF_PERIPHERAL_CONFIG(peripheral_config));

    register_write(&cpu, 13, 0x70000);

    cpu.running = 1;
    do {
        //log_f("cpu.running=%d\n", cpu.running);
        cpu.code_counter++;
        cpu.decoder.event_id = EVENT_ID_IDLE;
        fetch(&cpu);
        if (cpu.running == 0)break;
        if (EVENT_ID_IDLE == cpu.decoder.event_id) {
            decode(&cpu);
        }
        switch (cpu.decoder.event_id) {
        case EVENT_ID_UNDEF:
            interrupt_exception(&cpu, INT_EXCEPTION_UNDEF);
            log_f("undef:%08x\n", cpu.decoder.instruction_word);
            break;
        case EVENT_ID_SWI:
            interrupt_exception(&cpu, INT_EXCEPTION_SWI);
            break;
        case EVENT_ID_DATAABT:
            interrupt_exception(&cpu, INT_EXCEPTION_DATAABT);
            log_f("da\n");
            Sleep(1000);
            break;
        case EVENT_ID_PREAABT:
            interrupt_exception(&cpu, INT_EXCEPTION_PREABT);
            log_f("pa\n");
            Sleep(1000);
            break;
        case EVENT_ID_WFI:
            //while (!user_event(&peripheral_reg_base, EVENT_TYPE_DETECT)) 
        {
            Sleep(1);
        }
        default:
            if (!cpsr_i(&cpu) && 0 /* IRQ */) {
                interrupt_exception(&cpu, INT_EXCEPTION_IRQ);
            }
        }
        
        
        
    } while (cpu.running);


    log_f("MMU table base: 0x%08x\n", cpu.mmu.reg[2]);
    
    for (int i = 0; i < 4096; i++) {
        UINT32 dat = read_word_without_mmu(&cpu, cpu.mmu.reg[2] + (i << 2));
        if(dat != 0)
        log_f("%d, %08x\n", i, dat);
    }
    log_f("\n-----------MMU table end--------------\n");

    return 0;
}

DWORD WINAPI ThreadDumpRegs(LPVOID p) {
    while (1) {
        if (cpu.running) {
            reg_show(&cpu);
        }
        Sleep(1000);
    }
}

void emu_start()
{
	HANDLE hThread;
	HANDLE thread_dump_regs;
	DWORD  threadId;
    hThread = CreateThread(NULL, 0, emu_thread, 0, 0, &threadId); // 创建线程
    thread_dump_regs = CreateThread(NULL, 0, ThreadDumpRegs, 0, 0, &threadId); // 创建线程


}
