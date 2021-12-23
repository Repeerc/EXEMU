#pragma once


#ifdef __cplusplus

extern "C" void soc_uart_putc(const char c);
extern "C" void soc_log(int type, const char* fmt, ...);
extern "C" unsigned int soc_get_microsec();
extern "C" void soc_lcd_flush();

#else

void soc_uart_putc(const char c);
void soc_log(int type, const char* fmt, ...);
unsigned int soc_get_microsec();
void soc_lcd_flush();

#endif // cplusplus




#define LOG_ICOLL_ERR					(0x10)
#define LOG_ICOLL_INFO					(0x11)
#define LOG_UARTDBG_ERR					(0x30)
#define LOG_UARTDBG_INFO				(0x31)
#define LOG_POWER_ERR					(0x40)
#define LOG_POWER_INFO					(0x41)
#define LOG_PLL_ERR						(0x50)
#define LOG_PLL_INFO					(0x51)
#define LOG_DIGCTRL_ERR					(0x60)
#define LOG_DIGCTRL_INFO				(0x61)
#define LOG_APBH_ERR					(0x70)
#define LOG_APBH_INFO					(0x71)

#define LOG_PINCTRL_ERR					(0x80)
#define LOG_PINCTRL_INFO				(0x81)


#define LOG_LCDIF_ERR					(0x90)
#define LOG_LCDIF_INFO					(0x91)


#define LOG_TIMROT_ERR					(0xA0)
#define LOG_TIMROT_INFO					(0xA1)

#define LOG_GPMI_ERR					(0xB0)
#define LOG_GPMI_INFO					(0xB1)
#define LOG_ECC8_ERR					(0xC0)
#define LOG_ECC8_INFO					(0xC1)
#define LOG_USBCONT_ERR					(0xD0)
#define LOG_USBCONT_INFO				(0xD1)
#define LOG_USBPHY_ERR					(0xE0)
#define LOG_USBPHY_INFO					(0xE1)


