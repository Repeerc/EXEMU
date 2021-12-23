#pragma once
#include "mem.h"
#include "ram.h"
#include "mmu.h"
#include "cp15.h"

typedef struct SoC {
	

	ArmCpu cpu;
	ArmMmu mmu;
	ArmMem mem;
	ArmCP15 cp15;

	ArmRam ROM;
	ArmRam SRAM;

	Boolean running;

}SoC;

void soc_halt();

void raiseCpuIrq(Boolean enable);

