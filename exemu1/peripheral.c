

#include "mem.h"
#include "peripheral.h"
#include "emu_main.h"
#include "soc_output.h"

#include <string.h>
#include <stdio.h>

#define DEBUG
#define DEBUG_UART
//#define DEBUG_WRMEM
//#define DEBUG_DFLTP




#ifdef DEBUG
#define DEBUG_INFO(...)  printf(__VA_ARGS__) 

#ifdef DEBUG_WRMEM
#define DEBUG_WRMEM_INFO(...)  printf(__VA_ARGS__) 
#else
#define DEBUG_WRMEM_INFO
#endif


#ifdef DEBUG_WRREG
#define DEBUG_WRREG_INFO(...)  printf(__VA_ARGS__) 
#else
#define DEBUG_WRREG_INFO
#endif

#ifdef DEBUG_UART
#define DEBUG_UART_INFO(...)  printf(__VA_ARGS__) 
#else
#define DEBUG_UART_INFO
#endif

#ifdef DEBUG_DFLTP
#define DEBUG_DFLTP_INFO(...)  printf(__VA_ARGS__) 
#else
#define DEBUG_DFLTP_INFO
#endif

#else
#define DEBUG_INFO
#endif // DEBUG




#define PERIPHERAL_BASE 0x80000000
#define PERIPHERAL_SIZE 0x000C4000

#define SET  0x4
#define CLR  0x8
#define TOG  0xC
#define NWR  0x0

#define DIGCTRL_BASE 0x8001C000
#define DIGCTRL_SIZE 0x3F4
static UInt32 DIGCTRLRegs[64];


static UInt32 MPTERegs[8];
#define MPTE_BASE	0x8001C400
#define MPTE_SIZE	0x80


static UInt32 FLPTE[8];
static UInt32 PTE_2048 = 0x80000C12;
#define FLPT_BASE	0x800C0000
#define FLPT_SIZE	0x00004000



#define UARTDBG_BASE  0x80070000
#define UARTDBG_SIZE  0x4C
static UInt32 UARTDBGRegs[13];


#define ICOLL_BASE  0x80000000
#define ICOLL_SIZE  0x1D4
static UInt32 ICOLLRegs[30];
static UInt32 IRQStatus[64];
static int currentIRQ = -1;
static UInt32 currentIRQVECT = -1;

#define POWER_BASE	0x80044000
#define POWER_SIZE	0x114
static UInt32 POWERRegs[18];


#define PLL_BASE	0x80040000
#define PLL_SIZE	0x104
static UInt32 PLLRegs[16];

#define APBH_BASE	0x80004000
#define APBH_SIZE	0x3F4
static UInt32 APBHRegs[63];

#define APBX_BASE	0x80024000
#define APBX_SIZE	0x3F4
static UInt32 APBXRegs[63];


#define PINCTRL_BASE	0x80018000
#define PINCTRL_SIZE	0xB30
static UInt32 PINCTRLRegs[180];

#define LCDIF_BASE	0x80030000
#define LCDIF_SIZE	0xE4
static UInt32 LCDIFRegs[15];

unsigned char framebuf[258 * 136];

#define TIMROT_BASE	0x80068000
#define TIMROT_SIZE	0xA4
static UInt32 TIMROTRegs[12];

#define GPMI_BASE	0x8000C000
#define GPMI_SIZE	0xD4
static UInt32 GPMIRegs[0xD];

#define ECC8_BASE	0x80008000
#define ECC8_SIZE	0xA4
static UInt32 ECC8Regs[0xA];


#define USBCONT_BASE	0x80080000
#define USBCONT_SIZE	0x1D4
static UInt32 USBCONTRegs[0x1D];


#define USBPHY_BASE	0x8007C000
#define USBPHY_SIZE	0x84
static UInt32 USBPHYRegs[0x8];


inline Boolean wrMem(void* base, UInt32 offset, UInt8 size, Boolean write, void* bufP) {
	if (write) {
		switch (size)
		{
		case 4:
			DEBUG_WRMEM_INFO("WR %p[%x, 4]=%08x\n", base, offset, *((UInt32*)bufP));
			*((UInt32*)((UInt8 *)base + offset)) = *((UInt32*)bufP);
			return true;
		case 2:
			DEBUG_WRMEM_INFO("WR %p[%x, 2]=%08x\n", base, offset, *((UInt16*)bufP));
			*((UInt16*)((UInt8*)base + offset)) = *((UInt16*)bufP);
			return true;
		case 1:
			DEBUG_WRMEM_INFO("WR %p[%x, 1]=%08x\n", base, offset, *((UInt8*)bufP));
			*((UInt8*)((UInt8*)base + offset)) = *((UInt8*)bufP);
			return true;
		default:
			return false;
		}
	}
	else {
		switch (size)
		{
		case 4:
			DEBUG_WRMEM_INFO("RD %p[%x, 4]=%08x\n",base, offset, *((UInt32*)((UInt8*)base + offset)));
			*((UInt32*)(bufP)) = *((UInt32*)((UInt8*)base + offset));
			return true;
		case 2:
			DEBUG_WRMEM_INFO("RD %p[%x, 2]=%08x\n", base, offset, *((UInt16*)((UInt8*)base + offset)));
			*((UInt16*)(bufP)) = *((UInt16*)((UInt8*)base + offset));
			return true;
		case 1:
			DEBUG_WRMEM_INFO("RD %p[%x, 1]=%08x\n", base, offset, *((UInt8*)((UInt8*)base + offset)));
			*((UInt8*)(bufP)) = *((UInt8*)((UInt8*)base + offset));
			return true;
		default:
			return false;
		}
	}

}

inline Boolean wrtReg(UInt32 *base, UInt32 val, UInt32 opt) {
	switch (opt)
	{
	case NWR:
		DEBUG_WRREG_INFO("WR %08X\n", val);
		*base = val;
		return true;
	case CLR:
		DEBUG_WRREG_INFO("CLR %08X ", val);
		*base &= (~val);
		DEBUG_WRREG_INFO("RES %08X \n", *base);
		return true;
	case TOG:
		DEBUG_WRREG_INFO("TOG %08X\n", val);
		*base ^= val;
		return true;
	case SET:
		DEBUG_WRREG_INFO("SET %08X\n", val);
		*base |= val;
		return true;
	default:
		return false;
	}
}

static Boolean access_MPTE_regs(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	UInt32* regBase = userData;
	int regInd;
	pa -= MPTE_BASE;
	if (pa >= MPTE_SIZE) return false;
	regInd = pa >> 4;
	DEBUG_DFLTP_INFO("ACCESS MPTE {ind = %d}: ", regInd);
	return wrMem(&regBase[regInd], 0, size, write, bufP);

}

static Boolean access_FLPT(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	UInt32* regBase = userData;
	UInt32 section;
	int FLPT_ind = -1;
	pa -= FLPT_BASE;
	if (pa >= FLPT_SIZE) return false;
	DEBUG_DFLTP_INFO("ACCESS DFLPT {pa = %08x}:\n", pa);
	section = pa >> 2;
	if (section == 0x800) {
		if (write) {
			return true;
			UInt32 val;
			wrMem(&val, 0, size, 1, bufP);
			DEBUG_DFLTP_INFO("WR PTE2048:%x\n", val);
			PTE_2048 &= 0xF21B;
			val &= ~0xF21B;
			PTE_2048 |= val;
			return true;
		}
		else {
			return wrMem(&PTE_2048, 0, size, false, bufP);
		}
	}

	for (int i = 0; i < 8;i++) {
		if (section == MPTERegs[i]) {
			FLPT_ind = i;
			DEBUG_DFLTP_INFO("Found section{%x} >> FLPT_ind{%d}\n", section, FLPT_ind);
			break;
		}
	}
	if (FLPT_ind != -1) {
		return wrMem(&regBase[FLPT_ind], 0, size, write, bufP);
	}
	else {
		if (write) {
			return true;
		}
		else {
			UInt32 ret = 0;
			return wrMem(&ret, 0, size, false, bufP);			
		}
	}

}

static Boolean access_UARTDBG(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 val = 0;
	pa -= UARTDBG_BASE;
	switch (pa)
	{
	case 0:			//HW_UARTDBGDR
		ret = wrMem(&UARTDBGRegs[0], 0, size, write, bufP);
		if (ret) {
			//DEBUG_UART_INFO("UARTDBG PUT: %c\n", UARTDBGRegs[0] & 0xFF);
			soc_uart_putc(UARTDBGRegs[0] & 0xFF);
		}
		return ret;

	case 0x18:		//HW_UARTDBGFR
		ret = wrMem(&UARTDBGRegs[2], 0, size, write, bufP);
		return ret;
	case 0x38:
		return true;
	default:
		soc_log(LOG_UARTDBG_ERR, "Unsupported to access UARTDBG Regs addr[%08x]\n", pa);
		soc_halt();
		break;
	}
	return false;
}

static Boolean access_ICOLL(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - ICOLL_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	if (regBase > 5 && regBase < 22) { //HW_ICOLL_PRIORITY0 - HW_ICOLL_PRIORITY15
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	switch (regBase)
	{
	case 0:
		if (write) {	//HW_ICOLL_VECTOR
			wrMem(&val, 0, size, write, bufP);
			
			if (currentIRQVECT == val) {
				soc_log(LOG_ICOLL_INFO, "ACK IRQ addr:%08x Cur:%08x\n", val, ICOLLRegs[regBase]);
				raiseCpuIrq(false);
				IRQStatus[currentIRQ]--;
				currentIRQVECT = -1;
				currentIRQ = -1;
			}
			
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 1:
		if (write) {	//HW_ICOLL_LEVELACK
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 2:		//HW_ICOLL_CTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_ICOLL_INFO, "RESET ICOLL\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 3:
		if (write) {	//HW_ICOLL_STAT
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 0x16:		//HW_ICOLL_VBASE
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			val &= 0xFFFFFFFC;
			ret = wrtReg(&ICOLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ICOLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}


	default:
		soc_log(LOG_ICOLL_ERR, "Unsupported to access ICOLL Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}
	return false;
}

static Boolean access_PINCTRL(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - PINCTRL_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	if (regBase >= 0x10 && regBase <= 0x17) { //HW_PINCTRL_MUXSEL0 - HW_PINCTRL_MUXSEL7
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x20 && regBase <= 0x2E) { //HW_PINCTRL_DRIVE0 - HW_PINCTRL_DRIVE15
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x40 && regBase <= 0x42) { //HW_PINCTRL_DOUT0 - HW_PINCTRL_DOUT2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x50 && regBase <= 0x52) { //HW_PINCTRL_DIN0 - HW_PINCTRL_DIN2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x60 && regBase <= 0x62) { //HW_PINCTRL_DOE0 - HW_PINCTRL_DOE2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x70 && regBase <= 0x72) { //HW_PINCTRL_PIN2IRQ0 - HW_PINCTRL_PIN2IRQ2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x80 && regBase <= 0x82) { //HW_PINCTRL_IRQEN0 - HW_PINCTRL_IRQEN2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0x90 && regBase <= 0x92) { //HW_PINCTRL_IRQLEVEL0 - HW_PINCTRL_IRQLEVEL2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0xA0 && regBase <= 0xA2) { //HW_PINCTRL_IRQPOL0 - HW_PINCTRL_IRQPOL2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	if (regBase >= 0xB0 && regBase <= 0xB2) { //HW_PINCTRL_IRQSTAT0 - HW_PINCTRL_IRQSTAT2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	}

	switch (regBase)
	{

	case 0:		//HW_LCDIF_CTRL
			if (write) {
				wrMem(&val, 0, size, write, bufP);
				if ((val & 0xC0000000) == 0x80000000) {
					soc_log(LOG_PINCTRL_INFO, "PINCTRL RESET\n");
					val |= 0xC0000000;
				}
				ret = wrtReg(&PINCTRLRegs[regBase], val, opt);
				return ret;
			}
			else {
				ret = wrMem(&PINCTRLRegs[regBase], 0, size, write, bufP);
				return ret;
			}


	default:
		soc_log(LOG_PINCTRL_ERR, "Unsupported to access PINCTRL Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}
	return false;
}

static Boolean access_POWER(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - POWER_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_POWER_CTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&POWERRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&POWERRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 0xB:		//HW_POWER_STS
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&POWERRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&POWERRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 0xF:		//HW_POWER_DEBUG
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&POWERRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&POWERRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 4:		//HW_POWER_VDDDCTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			soc_log(LOG_POWER_INFO, "SET Voltage:%08x\n",val);
			ret = wrtReg(&POWERRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&POWERRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	default:
		soc_log(LOG_POWER_ERR, "Unsupported to access POWER Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_LCDIF(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - LCDIF_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_LCDIF_CTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_LCDIF_INFO, "LCDIF RESET\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 1:		//HW_LCDIF_CTRL1
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 2:		//HW_LCDIF_TIMING
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 7:		//HW_LCDIF_DVICTRL0
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 8:		//HW_LCDIF_DVICTRL1
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 0xB:		//HW_LCDIF_DATA
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			

			ret = wrtReg(&LCDIFRegs[regBase], val, opt);
			soc_log(LOG_LCDIF_INFO, "  LCD REG DAT:%08x\n", LCDIFRegs[regBase]);
			return ret;
		}
		else {
			ret = wrMem(&LCDIFRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	default:
		soc_log(LOG_LCDIF_ERR, "Unsupported to access LCDIF Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_PLL(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - PLL_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_CLKCTRL_PLLCTRL0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 2:		//HW_CLKCTRL_CPU
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 3:		//HW_CLKCTRL_HBUS
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 6:		//HW_CLKCTRL_PIX
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 8:		//HW_CLKCTRL_GPMI
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}


	case 0xD:		//HW_CLKCTRL_FRAC
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 0xE:		//HW_CLKCTRL_CLKSEQ
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	default:
		soc_log(LOG_PLL_ERR, "Unsupported to access PLL Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_DIGCTRL(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - DIGCTRL_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;


	switch (regBase)
	{


	case 0x0:		//HW_DIGCTL_CTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&PLLRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&PLLRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 0xC:		//HW_DIGCTL_MICROSECONDS
		if (write) {
			return true;
		}
		else {			
			val = soc_get_microsec();
			ret = wrMem(&val, 0, size, write, bufP);
			return ret;
		}

	default:
		soc_log(LOG_DIGCTRL_ERR, "Unsupported to access DIGCTRL Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}


static Boolean access_APBH(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - APBH_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_APBH_CTRL0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_APBH_INFO, "APBH ICOLL\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 1:		//HW_APBH_CTRL1
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 8:		//HW_APBH_CH0_SEMA (LCDIF)
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}


	case 5:		//HW_APBH_CH0_NXTCMDAR
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 0x21:		//HW_APBH_CH4_NXTCMDAR (GPMI)
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 0x24:		//HW_APBH_CH4_SEMA (GPMI)
		if (write) {
			wrMem(&val, 0, size, write, bufP);

			ret = wrtReg(&APBHRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&APBHRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	default:
		soc_log(LOG_APBH_ERR, "Unsupported to access APBH Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}


static Boolean access_TIMROT(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - TIMROT_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_TIMROT_ROTCTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_TIMROT_INFO, "TIMROT RESET\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 2:		//HW_TIMROT_TIMCTRL0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 3:		//HW_TIMROT_TIMCOUNT0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 4:		//HW_TIMROT_TIMCTRL1
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 5:		//HW_TIMROT_TIMCOUNT2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 6:		//HW_TIMROT_TIMCTRL2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 7:		//HW_TIMROT_TIMCOUNT2
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 8:		//HW_TIMROT_TIMCTRL3
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&TIMROTRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&TIMROTRegs[regBase], 0, size, write, bufP);
			return ret;
		}





	default:
		soc_log(LOG_TIMROT_ERR, "Unsupported to access TIMROT Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_GPMI(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - GPMI_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_GPMI_CTRL0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_GPMI_INFO, "GPMI RESET\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&GPMIRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&GPMIRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	case 6:		//HW_GPMI_CTRL1
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&GPMIRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&GPMIRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 7:		//HW_GPMI_TIMING0
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&GPMIRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&GPMIRegs[regBase], 0, size, write, bufP);
			return ret;
		}
	case 8:		//HW_GPMI_TIMING1
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			ret = wrtReg(&GPMIRegs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&GPMIRegs[regBase], 0, size, write, bufP);
			return ret;
		}

	default:
		soc_log(LOG_GPMI_ERR, "Unsupported to access GIMP Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_USBCONT(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - USBCONT_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;

	if (write) {
		wrMem(&val, 0, size, write, bufP);
		ret = wrtReg(&USBCONTRegs[regBase], val, opt);
		return ret;
	}
	else {
		ret = wrMem(&USBCONTRegs[regBase], 0, size, write, bufP);
		return ret;
	}
	switch (regBase)
	{

	default:
		soc_log(LOG_USBCONT_ERR, "Unsupported to access USB CONTLLER Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_USBPHY(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - USBPHY_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;

	//return true;
	if (write) {
		wrMem(&val, 0, size, write, bufP);
		ret = wrtReg(&USBPHYRegs[regBase], val, opt);
		return ret;
	}
	else {
		ret = wrMem(&USBPHYRegs[regBase], 0, size, write, bufP);
		return ret;
	}

	switch (regBase)
	{

	default:
		soc_log(LOG_USBPHY_ERR, "Unsupported to access USB PHY Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_APBX(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - APBX_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;

	//return true;
	if (write) {
		wrMem(&val, 0, size, write, bufP);
		ret = wrtReg(&APBXRegs[regBase], val, opt);
		return ret;
	}
	else {
		ret = wrMem(&APBXRegs[regBase], 0, size, write, bufP);
		return ret;
	}

	switch (regBase)
	{

	default:
		soc_log(LOG_USBPHY_ERR, "Unsupported to access USB PHY Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}

static Boolean access_ECC8(void* userData, UInt32 pa, UInt8 size, Boolean write, void* bufP) {
	int ret;
	UInt32 base = pa - ECC8_BASE;
	UInt32 regBase = base >> 4;
	UInt32 opt = base & 0xF;
	UInt32 val = 0;
	switch (regBase)
	{
	case 0:		//HW_ECC8_CTRL
		if (write) {
			wrMem(&val, 0, size, write, bufP);
			if ((val & 0xC0000000) == 0x80000000) {
				soc_log(LOG_ECC8_INFO, "ECC8 RESET\n");
				val |= 0xC0000000;
			}
			ret = wrtReg(&ECC8Regs[regBase], val, opt);
			return ret;
		}
		else {
			ret = wrMem(&ECC8Regs[regBase], 0, size, write, bufP);
			return ret;
		}


	default:
		soc_log(LOG_ECC8_ERR, "Unsupported to access ECC8 Regs [%08x]\n", regBase);
		soc_halt();
		break;
	}

	return false;


}


void raiseIrq(UInt32 IRQNum) {

	if (IRQStatus[IRQNum] > 0) {
		return;
	}
	IRQStatus[IRQNum]++;

}

void LCDIF_Decode(UInt8 *SRAM, UInt8 opa, UInt8 NumPioWords, UInt32 *PIOWORD, UInt8 *DMA_data, UInt16 nBytes)
{
	static UInt8 datSel;
	static UInt8 CurrentCmd;
	static UInt16 Cx, Cy, bytecnt;
	static UInt16 Mx, My;
	static UInt8 dats[32];

	if (nBytes > 0) {
		switch (opa)
		{
		case READ_FROM_RAM:
			if (NumPioWords > 0) {
				datSel = (PIOWORD[0] >> 18) & 1;
				if (datSel) {
					soc_log(LOG_LCDIF_INFO, "SEND DAT TO LCD %d\n  ", nBytes);
					switch (CurrentCmd)
					{
					case 0x2a:
						if (bytecnt < 4) {
							for (int i = 0; (i < nBytes) && (i < 4); i++) {
								dats[3 - bytecnt] = DMA_data[i];
								bytecnt++;
							}
						}
						Mx = *((UInt16*)dats) * 3;
						Cx = *((UInt16*)dats + 2);
						soc_log(LOG_LCDIF_INFO, "  LCD SET X:{%d - %d}\n  ", Cx, Mx);
						break;

					case 0x2b:
						if (bytecnt < 4) {
							for (int i = 0; (i < nBytes) && (i < 4); i++) {
								dats[3 - bytecnt] = DMA_data[i];
								bytecnt++;
							}
						}
						My = *((UInt16*)dats);
						Cy = *((UInt16*)dats + 2);
						soc_log(LOG_LCDIF_INFO, "  LCD SET Y:{%d - %d}\n  ", Cy, My);
						break;

					case 0x2C:
						for (int i = 0; i < nBytes; i++) {
							
							//if ((Cy > 7) && (Cx < 255)) {
								framebuf[Cx + Cy * 255] = 255 - DMA_data[i];
							//}

							Cx++;
							if ((Cx >= 258) || (Cx >= Mx)) {
								Cx = 0;
								Cy++;
							}
							if (Cy >= My) {
								Cy = 0;
							}
							
						}
						soc_lcd_flush();
						break;

					default:
						break;
					}
					

				}
				else {
					soc_log(LOG_LCDIF_INFO, "SEND CMD TO LCD %d\n  ", nBytes);
					CurrentCmd = DMA_data[0];
					bytecnt = 0;
				}


				if (nBytes > 8) {
					for (int i = 0; i < 8; i++) {
						soc_log(LOG_LCDIF_INFO, " %02x", DMA_data[i]);
					}
					soc_log(LOG_LCDIF_INFO, " ...\n");
				}
				else {
					for (int i = 0; i < nBytes; i++) {
						soc_log(LOG_LCDIF_INFO, " %02x", DMA_data[i]);
					}
					
					soc_log(LOG_LCDIF_INFO, "\n");
				}

				break;
			}
			
		default:
			break;
		}
	}
}


void GPMI_AfterCPUCycle() {

	if ((GPMIRegs[0] & 0xC0000000) == 0x80000000) {//HW_GPMI_CTRL
		GPMIRegs[0] &= 0x3FFFFFFF;

	}


}

void GPMI_Decode(UInt8* SRAM, UInt8 opa, UInt8 NumPioWords, UInt32* PIOWORD, UInt8* DMA_data, UInt16 nBytes) {

		switch (opa)
		{
		case XFER:
			soc_log(LOG_GPMI_INFO, "PROG GPMI %d\n  ", nBytes);
			break;
		case READ_FROM_RAM:
			soc_log(LOG_GPMI_INFO, "SEND DAT TO FLASH %d\n  ", nBytes);
			break;

		case WRITE_TO_RAM:
			soc_log(LOG_GPMI_INFO, "WRITE DAT TO FLASH %d\n  ", nBytes);
			break;

		}
	


}

void ICOLL_AfterCPUCycle() {

	if ((ICOLLRegs[2] & 0xC0000000) == 0x80000000) {//HW_ICOLL_CTRL
		ICOLLRegs[2] &= 0x3FFFFFFF;
		
	}

	if (currentIRQVECT != -1) {
		return;
	}


	for (int IRQNum = 0; IRQNum < 64; IRQNum++) {
		if (IRQStatus[IRQNum] > 0) {
			UInt32 regInd = (IRQNum / 4) + 6;
			UInt8 irq_reg = ICOLLRegs[regInd] >> ((IRQNum % 4) * 8);
			UInt8 PRIORITY = irq_reg & 3;
			UInt8 VECTOR_PITCH = ((ICOLLRegs[2] >> 21) & 7) * 4;		//HW_ICOLL_CTRL
			Boolean Enable = (irq_reg >> 2) & 1;
			if (VECTOR_PITCH == 0) {
				VECTOR_PITCH = 4;
			}
			if (Enable) {
				ICOLLRegs[1] = (1 << PRIORITY);
				ICOLLRegs[0] = ICOLLRegs[0x16] + VECTOR_PITCH * IRQNum;
				ICOLLRegs[3] = IRQNum & 0x3F;
				soc_log(LOG_ICOLL_INFO, "Raised IRQ:%d\n", IRQNum);
				IRQStatus[IRQNum] = 1;
				currentIRQVECT = ICOLLRegs[0];
				currentIRQ = IRQNum;
				raiseCpuIrq(true);
				break;
			}

			
		}
	}








}



void APBH_AfterCPUCycle(UInt8* SRAM) {
	hw_DmaDesc* dmadesc;

	if ((APBHRegs[0] & 0xC0000000) == 0x80000000) {//HW_APBH_CTRL0
		APBHRegs[0] &= 0x3FFFFFFF;
		
	}
	if ((APBHRegs[8] & 0xFF) > 0) {	//HW_APBH_CH0_SEMA (LCDIF)
		if ((APBHRegs[5] % 4) != 0) {
			soc_log(LOG_APBH_ERR, "HW_APBH_CH0_NXTCMDAR is not aligned:%08x\n", APBHRegs[5]);
			soc_halt();
		}
		if ((APBHRegs[5] >= 0x80000U) && (APBHRegs[5] < 0xFFFF0000U)) {
			soc_log(LOG_APBH_ERR, "HW_APBH_CH0_NXTCMDAR out of range:%08x\n", APBHRegs[5]);
			soc_halt();
		}
		dmadesc = (hw_DmaDesc*)(SRAM + APBHRegs[5]);
		do {
			
			//soc_log(LOG_APBH_INFO, "  cmd:%d\n", dmadesc->Command);

			LCDIF_Decode(	SRAM, 
							dmadesc->Command, 
							dmadesc->NumPIOWords,
							dmadesc->PioWord, 
							SRAM + (UInt32)(dmadesc->p_DMABuffer) ,
							dmadesc->DMABytes);

			if (dmadesc->Semaphore && ((APBHRegs[8] & 0xFF) > 0)) {
				APBHRegs[8]--;
			}
			if (dmadesc->IRQOnCompletion) {

				raiseIrq(STMP3770_IRQ_LCDIF_DMA);
			}
			if (dmadesc->Chain == 0) {
				break;
			}
			dmadesc = (hw_DmaDesc*)(SRAM + (UInt32)dmadesc->p_Next);
		} while (dmadesc != NULL);
		

	}


	if ((APBHRegs[0x24] & 0xFF) > 0) {	//HW_APBH_CH4_SEMA (GPMI)
		soc_log(LOG_APBH_INFO, "  GPMI DMA\n");
		if ((APBHRegs[0x21] % 4) != 0) {
			soc_log(LOG_APBH_ERR, "HW_APBH_CH4_NXTCMDAR is not aligned:%08x\n", APBHRegs[0x21]);
			soc_halt();
		}
		if ((APBHRegs[0x21] >= 0x80000U) && (APBHRegs[5] < 0xFFFF0000U)) {
			soc_log(LOG_APBH_ERR, "HW_APBH_CH4_NXTCMDAR out of range:%08x\n", APBHRegs[0x21]);
			soc_halt();
		}
		dmadesc = (hw_DmaDesc*)(SRAM + APBHRegs[0x21]);
		do {

			//soc_log(LOG_APBH_INFO, "  cmd:%d\n", dmadesc->Command);

			GPMI_Decode(SRAM,
				dmadesc->Command,
				dmadesc->NumPIOWords,
				dmadesc->PioWord,
				SRAM + (UInt32)(dmadesc->p_DMABuffer),
				dmadesc->DMABytes);

			if (dmadesc->Semaphore && ((APBHRegs[0x24] & 0xFF) > 0)) {
				APBHRegs[0x24]--;
			}

			if (dmadesc->IRQOnCompletion) {
				if ((APBHRegs[1] >> 12) & 1) {
					APBHRegs[1] |= (1 << 4);		//CH4_CMDCMPLT_IRQ
					raiseIrq(STMP3770_IRQ_GPMI_DMA);
				}
				
			}

			if (dmadesc->Chain == 0) 
				break;
			dmadesc = (hw_DmaDesc*)(SRAM + (UInt32)dmadesc->p_Next);
		} while (dmadesc != NULL);



	}

	if (((APBHRegs[0] >> 16) & 0xFF) != 0) { //RESET_CHANNEL
		soc_log(LOG_APBH_INFO, "RESET DMA CHANNEL:%02X\n", APBHRegs[0] >> 16);
		APBHRegs[0] &= ~(0xFF << 16);

	}



}


void LCDIF_AfterCPUCycle() {

	if ((LCDIFRegs[0] & 0xC0000000) == 0x80000000) {//HW_APBH_CTRL0
		LCDIFRegs[0] &= 0x3FFFFFFF;
		
	}

}
void TIMROT_AfterCPUCycle() {

	static UInt64 div = 0;

	div = div + 1;
	
	if(div % 10000 == 0)
	for (int timN = 0; timN < 4; timN++) {
		if ((TIMROTRegs[timN + 2] >> 14) & 1) { //IRQ_EN
							//soc_log(LOG_TIMROT_INFO, "tick %d,%08x\n", timN, TIMROTRegs[timN + 3]);
			TIMROTRegs[timN + 2] |= (1 << 15);
			raiseIrq(28 + timN);
		}
	}

	/*
	static UInt64 clock_tick = 0;
	UInt32 th = 0;
	//   1000 1khZ	 11
	//    250 4kHz   10
	//    125 8kHz   9
	//	   31 32kHz  8
	
	if ((TIMROTRegs[0] & 0xC0000000) == 0x80000000) {//HW_TIMROT_ROTCTRL
		TIMROTRegs[0] &= 0x3FFFFFFF;
	}

	for (int timN = 0; timN < 4;timN++) {
		if ((TIMROTRegs[timN + 2] >> 7) & 1) {	//Time 0 UPDATE

			switch (TIMROTRegs[timN + 2] & 0xF)
			{
			case 0x8:
				th = 31;
				break;
			case 0x9:
				th = 125;
				break;
			case 0xA:
				th = 250;
				break;
			case 0xB:
				th = 1000;
				break;

			default:
				th = 1;
				break;
			}
			//soc_log(LOG_TIMROT_INFO, "tick th %d\n",th);
			if ((soc_get_microsec() - clock_tick) > th) { 
				//soc_log(LOG_TIMROT_INFO, "tick\n");
				if ((TIMROTRegs[timN + 3] >> 16) > 0) {
					TIMROTRegs[timN + 3] -= 0x10000;
					
					if ((TIMROTRegs[timN + 3] >> 16) == 0) {
						//
						if ((TIMROTRegs[timN + 2] >> 14) & 1) { //IRQ_EN
							//soc_log(LOG_TIMROT_INFO, "tick %d,%08x\n", timN, TIMROTRegs[timN + 3]);
							TIMROTRegs[timN + 2] |= (1 << 15);
							raiseIrq(28 + timN);
						}
					}
				}
				else {
					if ((TIMROTRegs[timN + 2] >> 6) & 1) {	//RELOAD
						TIMROTRegs[timN + 3] &= 0xFFFF;
						TIMROTRegs[timN + 3] |= (TIMROTRegs[timN + 3] << 16);
					}
				}
			}

		}



	}




	clock_tick = soc_get_microsec();*/


}

Boolean peripheralInit(ArmMem* mem) {

	if (!memRegionAdd(mem, MPTE_BASE, MPTE_SIZE, &access_MPTE_regs, MPTERegs)) {
		err_str("failed to initialize MPTE.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, FLPT_BASE, FLPT_SIZE, &access_FLPT, FLPTE)) {
		err_str("failed to initialize DFLPT.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, UARTDBG_BASE, UARTDBG_SIZE, &access_UARTDBG, FLPTE)) {
		err_str("failed to initialize UARTDBG.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, ICOLL_BASE, ICOLL_SIZE, &access_ICOLL, ICOLLRegs)) {
		err_str("failed to initialize ICOLL.\r\n");
		return false;
	}
	
	if (!memRegionAdd(mem, POWER_BASE, POWER_SIZE, &access_POWER, POWERRegs)) {
		err_str("failed to initialize POWER.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, PLL_BASE, PLL_SIZE, &access_PLL, PLLRegs)) {
		err_str("failed to initialize PLL.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, DIGCTRL_BASE, DIGCTRL_SIZE, &access_DIGCTRL, DIGCTRLRegs)) {
		err_str("failed to initialize DIGCTRL.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, APBH_BASE, APBH_SIZE, &access_APBH, APBHRegs)) {
		err_str("failed to initialize APBH.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, APBX_BASE, APBX_SIZE, &access_APBX, APBXRegs)) {
		err_str("failed to initialize APBX.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, PINCTRL_BASE, PINCTRL_SIZE, &access_PINCTRL, PINCTRLRegs)) {
		err_str("failed to initialize PINCTRL.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, LCDIF_BASE, LCDIF_SIZE, &access_LCDIF, LCDIFRegs)) {
		err_str("failed to initialize LCDIF.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, TIMROT_BASE, TIMROT_SIZE, &access_TIMROT, TIMROTRegs)) {
		err_str("failed to initialize TIMROT.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, GPMI_BASE, GPMI_SIZE, &access_GPMI, GPMIRegs)) {
		err_str("failed to initialize GPMI.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, ECC8_BASE, ECC8_SIZE, &access_ECC8, ECC8Regs)) {
		err_str("failed to initialize ECC8.\r\n");
		return false;
	}
	if (!memRegionAdd(mem, USBCONT_BASE, USBCONT_SIZE, &access_USBCONT, USBCONTRegs)) {
		err_str("failed to initialize USB Controller.\r\n");
		return false;
	}
	
	if (!memRegionAdd(mem, USBPHY_BASE, USBPHY_SIZE, &access_USBPHY, USBPHYRegs)) {
		err_str("failed to initialize USB Phy.\r\n");
		return false;
	}




	UARTDBGRegs[2] = 0x90;

	for (int i = 0; i < 64;i++) {
		IRQStatus[i] = 0;
	}

	return true;
}

Boolean peripheralDeinit(ArmMem* mem) {

	return memRegionDel(mem, PERIPHERAL_BASE, PERIPHERAL_SIZE);
}


