
#include "FL/Fl.H"
#include "FL/Fl_Window.H"
#include "FL/Fl_Box.H"
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Double_Window.H>

#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include "glut.h"

#include "threads.h"

#include <Windows.h>
#include <cstdarg>
#include <chrono>   
using namespace std;
using namespace chrono;


#include "soc_output.h"
#include <iostream>

Fl_Window* window;
Fl_Text_Buffer* uartdbg_buf = 0;
Fl_Text_Buffer* emulog_buf = 0;

Fl_Text_Editor* emulog_win;
Fl_Text_Editor* uartdbg_output_log_win;

extern "C" {
	int emu_main();
	extern unsigned char framebuf[258 * 136];
}

class LCD_window : public Fl_Gl_Window {
	void draw();
public:
	LCD_window(int x, int y, int w, int h, const char* l = 0);
};

LCD_window::LCD_window(int x, int y, int w, int h, const char* l) :
	Fl_Gl_Window(x, y, w, h, l) {

}

inline void dotPoint(int x, int y, int c) {
	glColor3f(c / 255.0, c / 255.0, c / 255.0);
	glBegin(GL_POLYGON);
	glVertex2f(x, y);
	glVertex2f(x + 2, y);
	glVertex2f(x + 2, y + 2);
	glVertex2f(x, y + 2);
	glEnd();
}

void LCD_window::draw() {
	// the valid() property may be used to avoid reinitializing your
	// GL transformation for each redraw:
	if (!valid()) {
		valid(1);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glViewport(0, 0, pixel_w(), pixel_h());
		glOrtho(0, pixel_w(), pixel_h(), 0, 0, INT_MAX);

		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_DEPTH_TEST);//关闭深度测试
		glDisable(GL_CULL_FACE); //关闭面剔除

	}
	// draw an amazing graphic:
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//dotPoint(2, 3, 254);
	/*
	for (int i = 0; i < 100;i+=2) {
		dotPoint(i, i, 255 - i);
	}*/

	for (int y = 0; y < 136; y++) {
		for (int x = 0; x < 257; x++) {
			dotPoint(x * 2, y * 2, framebuf[x + (y + 8) * 258]);
		}

	}

	glFlush();
}

LCD_window* lcd_win;





auto start = system_clock::now();
unsigned int soc_get_microsec() {
	auto end = system_clock::now();
	auto duration = duration_cast<microseconds>(end - start);
	return duration.count();

}

void soc_uart_putc(const char c) {
	Fl::lock();
	uartdbg_buf->append(&c);
	uartdbg_output_log_win->scroll(uartdbg_output_log_win->count_lines(0, -1, true), 0);
	Fl::unlock();
}

void soc_lcd_flush() {
	Fl::lock();
	lcd_win->flush();
	Fl::unlock();
}

void soc_log(int type, const char* fmt, ...) {
	if (
		(type == LOG_LCDIF_INFO) ||
		(type == LOG_ICOLL_INFO) 
		
		) {
		return;
	}

	va_list aptr;
	int ret;
	char buffer[1024];
	

	va_start(aptr, fmt);
	ret = vsprintf(buffer, fmt, aptr);
	va_end(aptr);
	Fl::lock();
	emulog_buf->append(buffer);

	
	//cout << emulog_win->count_lines(0, -1, true);
	emulog_win->scroll(emulog_win->count_lines(0, -1, true), 0);
	Fl::unlock();
}

DWORD WINAPI Thread_emu(void* dat) {
	emu_main();
	return 0;
}






int main(int argc, char** argv) {
	uartdbg_buf = new Fl_Text_Buffer();
	emulog_buf = new Fl_Text_Buffer();
	window = new Fl_Window(1360, 800, "ExEmu 0.2");

	(new Fl_Box(0, 5, 200, 20, "UARTDBG Output:"))->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	uartdbg_output_log_win = new Fl_Text_Editor(0, 30, 400, 240);
	uartdbg_output_log_win->hide_cursor();
	uartdbg_output_log_win->buffer(uartdbg_buf);

	(new Fl_Box(0, 305, 200, 20, "Emulator Output:"))->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	emulog_win = new Fl_Text_Editor(0, 330, 400, 240);
	emulog_win->hide_cursor();
	emulog_win->buffer(emulog_buf);
	
	lcd_win = new LCD_window(410, 30,257*2,128*2);
	
	window->end();
	window->show(argc, argv);

	DWORD threadID;   
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, Thread_emu, NULL, 0, &threadID);

	Fl::run();

	
	

	return 0;
}
