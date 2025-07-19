#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stddef.h>

#define KEY_ESC 0x01
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ENTER 0x1C
#define KEY_LCTRL 0x1D
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LALT 0x38
#define KEY_CAPS 0x3A
#define KEY_SPACE 0x39
#define KEY_NUM1 0x4F
#define KEY_NUM2 0x50
#define KEY_NUM3 0x51
#define KEY_NUM4 0x4B
#define KEY_NUM5 0x4C
#define KEY_NUM6 0x4D
#define KEY_NUM7 0x47
#define KEY_NUM8 0x48
#define KEY_NUM9 0x49
#define KEY_NUM0 0x52
#define KEY_NUMPLUS 0x4E
#define KEY_NUMMINUS 0x4A
#define KEY_NUMLOCK 0x45
#define KEY_NUMDOT 0x53
#define KEY_NUMSTAR 0x37
#define KEY_NUMSLASH 0x35
void keyboard_initialize(void);
char keyboard_getchar(void);
void keyboard_readline(char *buffer, size_t size);
bool onkeypress(int key);
bool onkeyreleased(int key);
bool iskeydown(int key);

#endif