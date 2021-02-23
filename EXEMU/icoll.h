#pragma once

uint32_t icoll_reset(void* base);

uint32_t icoll_read(void* base, uint32_t address);

void icoll_write(void* base, uint32_t address, uint32_t data, uint8_t mask);

void icoll_exit(void* base);


#define HW_ICOLL_VECTOR			0
#define HW_ICOLL_LEVELACK		1
#define HW_ICOLL_CTRL			2
#define HW_ICOLL_STAT			3
#define HW_ICOLL_RAW0			4
#define HW_ICOLL_RAW1			5
#define HW_ICOLL_PRIORITY0		6
#define HW_ICOLL_PRIORITY1		7
#define HW_ICOLL_PRIORITY2		8
#define HW_ICOLL_PRIORITY3		9
#define HW_ICOLL_PRIORITY4		10
#define HW_ICOLL_PRIORITY5		11
#define HW_ICOLL_PRIORITY6		12
#define HW_ICOLL_PRIORITY7		13
#define HW_ICOLL_PRIORITY8		14
#define HW_ICOLL_PRIORITY9		15
#define HW_ICOLL_PRIORITY10		16
#define HW_ICOLL_PRIORITY11		17
#define HW_ICOLL_PRIORITY12		18
#define HW_ICOLL_PRIORITY13		19
#define HW_ICOLL_PRIORITY14		20
#define HW_ICOLL_PRIORITY15		21
#define HW_ICOLL_VBASE			22





