#include "keyboard.h"
#include "vga.h"
#include "../libk/ports.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_CAPS 0x3A

static const char scancode_ascii_map[2][58] = {
    // Normal characters (not shifted)
    {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    },
    // Shifted characters
    {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
        '*', 0, ' '
    }
};

// Keyboard state variables
static bool shift_pressed = false;
static bool caps_lock = false;
static bool key_states[256] = {0};

void keyboard_initialize(void)
{
    shift_pressed = false;
    caps_lock = false;
    for (int i = 0; i < 256; i++)
    {
        key_states[i] = false;
    }
}

static inline bool is_letter(unsigned char scancode)
{
    char c = scancode_ascii_map[0][scancode];
    return (c >= 'a' && c <= 'z');
}

static inline char process_scancode(unsigned char scancode)
{
    if (scancode & 0x80)
    {
        unsigned char released_key = scancode & 0x7F;
        key_states[released_key] = false;
        
        if (released_key == KEY_LSHIFT || released_key == KEY_RSHIFT)
        {
            shift_pressed = false;
        }
        return 0;
    }
    
    key_states[scancode] = true;
    
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT)
    {
        shift_pressed = true;
        return 0;
    }
    else if (scancode == KEY_CAPS)
    {
        caps_lock = !caps_lock;
        return 0;
    }
    
    if (scancode < sizeof(scancode_ascii_map[0]))
    {
        char base_char = scancode_ascii_map[0][scancode];
        
        if (base_char == 0)
        {
            return 0;
        }
        
        bool use_shifted = shift_pressed;
        
        if (is_letter(scancode))
        {
            use_shifted = shift_pressed != caps_lock;
        }
        
        return scancode_ascii_map[use_shifted ? 1 : 0][scancode];
    }
    
    return 0;
}

char keyboard_getchar(void)
{
    char c = 0;
    while (c == 0)
    {
        while ((inportb(KEYBOARD_STATUS_PORT) & 1) == 0);
        unsigned char scancode = inportb(KEYBOARD_DATA_PORT);
        c = process_scancode(scancode);
    }
    printc(c); //TEMPORARY //!
    return c;
}

void keyboard_readline(char *buffer, size_t size)
{
    size_t i = 0;
    char c;
    
    while (i < size - 1)
    {
        c = keyboard_getchar();
        if (c == '\n')
        {
            buffer[i] = '\0';
            return;
        }
        else if (c == '\b')
        {
            if (i > 0)
            {
                i--;
            }
        }
        else
        {
            buffer[i++] = c;
        }
    }
    buffer[size - 1] = '\0';
}

bool onkeypress(int key)
{
    if ((inportb(KEYBOARD_STATUS_PORT) & 1) == 0)
    {
        return false;
    }
    
    unsigned char scancode = inportb(KEYBOARD_DATA_PORT);
    if (scancode & 0x80)
    {
        unsigned char released_key = scancode & 0x7F;
        key_states[released_key] = false;
        if (released_key == KEY_LSHIFT || released_key == KEY_RSHIFT)
        {
            shift_pressed = false;
        }
        
        return false;
    }
    key_states[scancode] = true;
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT)
    {
        shift_pressed = true;
        return false;
    }
    else if (scancode == KEY_CAPS)
    {
        caps_lock = !caps_lock;
        return false;
    }
    return (scancode == key);
}

bool onkeyreleased(int key)
{
    if ((inportb(KEYBOARD_STATUS_PORT) & 1) == 0)
    {
        return false;
    }
    
    unsigned char scancode = inportb(KEYBOARD_DATA_PORT);
    if (scancode & 0x80)
    {
        unsigned char released_key = scancode & 0x7F;
        key_states[released_key] = false;
        if (released_key == KEY_LSHIFT || released_key == KEY_RSHIFT)
        {
            shift_pressed = false;
        }
        
        return (released_key == key);
    }
    key_states[scancode] = true;
    
    return false;
}

bool iskeydown(int key)
{
    return key_states[key];
}