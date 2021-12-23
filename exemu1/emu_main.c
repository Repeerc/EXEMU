#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "emu_main.h"


#include "peripheral.h"
#include "soc_output.h"



#define SRAM_BASE 0x00000000UL
#define SRAM_SIZE 512*1024

#define ROM_BASE  0xFFFF0000UL
#define ROM_SIZE  64*1024

UInt8 *SRAM_BUF;
UInt8 *ROM_BUF;

SoC SoC_STMP3770;

static Boolean vMemF(ArmCpu* cpu, void* buf, UInt32 vaddr, UInt8 size, Boolean write, Boolean priviledged, UInt8* fsrP) {

	SoC* soc = cpu->userData;
	UInt32 pa;

	if (size & (size - 1)) {	//size is not a power of two

		return false;
	}
	if (vaddr & (size - 1)) {	//bad alignment

		return false;
	}

	return mmuTranslate(&soc->mmu, vaddr, priviledged, write, &pa, fsrP) && memAccess(&soc->mem, pa, size, write, buf);
}


static Boolean hyperF(ArmCpu* cpu) {

	return true;
}

static void setFaultAdrF(ArmCpu* cpu, UInt32 adr, UInt8 faultStatus) {

	SoC* soc = cpu->userData;

	cp15SetFaultStatus(&soc->cp15, adr, faultStatus);
}

static Boolean pMemReadF(void* userData, UInt32* buf, UInt32 pa) {	//for DMA engine and MMU pagetable walks

	ArmMem* mem = userData;

	return memAccess(mem, pa, 4, false, buf);
}

static void emulErrF(_UNUSED_ ArmCpu* cpu, const char* str) {
	err_str("Emulation error: <<");
	err_str(str);
	err_str(">> exiting.\r\n");
}

void err_str(const char* str) {

	fprintf(stderr, "%s", str);
}

void __mem_zero(void* addr, UInt16 size) {
	memset(addr, 0, size);
}

void __mem_copy(void* d, const void* s, UInt32 sz) {
	memcpy(d, s, sz);
}

void err_hex(UInt32 hex) {
	printf("%08x", hex);
}

void err_dec(UInt32 val) {
	printf("%d", val);
}

static void dumpCpuState(ArmCpu* cpu, char* label) {

	UInt8 i;

	if (label) {
		err_str("CPU ");
		err_str(label);
		err_str("\r\n");
	}

	for (i = 0; i < 16; i++) {
		err_str("R");
		err_dec(i);
		err_str("\t= 0x");
		err_hex(cpuGetRegExternal(cpu, i));
		err_str("\r\n");
	}
	err_str("CPSR\t= 0x");
	err_hex(cpuGetRegExternal(cpu, ARM_REG_NUM_CPSR));
	err_str("\r\nSPSR\t= 0x");
	err_hex(cpuGetRegExternal(cpu, ARM_REG_NUM_SPSR));
	err_str("\r\n");
}

void err_dump() {
	dumpCpuState(&SoC_STMP3770.cpu, "STMP3770");

}

void soc_halt() {

	SoC_STMP3770.running = false;
}

void raiseCpuIrq(Boolean enable) {
	cpuIrq(&SoC_STMP3770.cpu, false, enable);
}

int emu_main() {
	Err e;
	FILE *f;
	int i,len;


	SRAM_BUF = calloc(SRAM_SIZE, 1);
	if (!SRAM_BUF) {
		emulErrF(NULL, "Failed to malloc SRAM.\n");
		return -1;
	}
	ROM_BUF = calloc(ROM_SIZE, 1);
	if (!ROM_BUF) {
		emulErrF(NULL, "Failed to malloc ROM.\n");
		return -1;
	}
	
	f = fopen("rom.bin", "rb");
	if (f == NULL) {
		emulErrF(NULL, "Failed to load rom.\n");
	}
	else {
		i = fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		printf("Rom Size:%d.\n", len);
		if (len != (long)fread(SRAM_BUF, 1, len, f)) {
			perror("cannot load the rom.");

			free(SRAM_BUF);
			free(ROM_BUF);
			return NULL;
		}

		fclose(f);
	}




	e = cpuInit(&SoC_STMP3770.cpu, 0x00000000, vMemF, emulErrF, hyperF, setFaultAdrF);
	dumpCpuState(&SoC_STMP3770.cpu, "STMP3770");
	
	if (e) {
		emulErrF(NULL, "Failed to init CPU: ");
		return -1;
	}

	SoC_STMP3770.cpu.userData = &SoC_STMP3770;
	memInit(&SoC_STMP3770.mem);
	mmuInit(&SoC_STMP3770.mmu, pMemReadF, &SoC_STMP3770.mem);

	if(!ramInit(&SoC_STMP3770.ROM, &SoC_STMP3770.mem, ROM_BASE, ROM_SIZE, ROM_BUF)) emulErrF(NULL, "Cannot init ROM");
	if(!ramInit(&SoC_STMP3770.SRAM, &SoC_STMP3770.mem, SRAM_BASE, SRAM_SIZE, SRAM_BUF)) emulErrF(NULL, "Cannot init SRAM");



	if(!peripheralInit(&SoC_STMP3770.mem)) emulErrF(NULL, "Cannot init peripheral");

	cp15Init(&SoC_STMP3770.cp15, &SoC_STMP3770.cpu, &SoC_STMP3770.mmu);

	cpuSetReg(&SoC_STMP3770.cpu, 13, 0x70000);

	UInt64 cycles = 0;
	UInt64 cycleCnt = 0;
	UInt32 startTime = soc_get_microsec();
	SoC_STMP3770.running = true;

	while (SoC_STMP3770.running) {
		cpuCycle(&SoC_STMP3770.cpu);
		
		ICOLL_AfterCPUCycle();
		APBH_AfterCPUCycle(SRAM_BUF);
		LCDIF_AfterCPUCycle();
		GPMI_AfterCPUCycle();
		TIMROT_AfterCPUCycle();
		
		if ((soc_get_microsec() - startTime) > 1000000) {
			startTime = soc_get_microsec();
			printf("cycles:%lld, PC=%08x , Freq:%.3llf MIPS\n",
				cycles, 
				cpuGetRegExternal(&SoC_STMP3770.cpu, 15),
				(cycles - cycleCnt) / 1000000.0);
			cycleCnt = cycles;
		}
		

		//if (cycles > 500)
		//	break;
		cycles++;
	}

	free(SRAM_BUF);
	free(ROM_BUF);




	return 0;

}






