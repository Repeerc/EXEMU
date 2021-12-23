#ifndef _RAM_H_
#define _RAM_H_


#include "emu_types.h"
#include "mem.h"

typedef struct{

	UInt32 adr;
	UInt32 sz;
	UInt32* buf;

}ArmRam;


Boolean ramInit(ArmRam* ram, ArmMem* mem, UInt32 adr, UInt32 sz, UInt32* buf);
Boolean ramDeinit(ArmRam* ram, ArmMem* mem);





#endif

