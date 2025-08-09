#include "keyboard.h"
#include "vga.h"
#include "../cpu/isr.h"
#include "../libk/ports.h"
#include "../libk/debug/log.h"
#include <stdbool.h>

#define PS2_DATA_PORT        0x60
#define PS2_STATUS_PORT      0x64
#define PS2_COMMAND_PORT     0x64

#define PS2_STATUS_OUTPUT_FULL    0x01
#define PS2_STATUS_INPUT_FULL     0x02
#define PS2_STATUS_SYSTEM         0x04
#define PS2_STATUS_COMMAND        0x08
#define PS2_STATUS_TIMEOUT        0x40
#define PS2_STATUS_PARITY         0x80

#define PS2_CMD_READ_CONFIG       0x20
#define PS2_CMD_WRITE_CONFIG      0x60
#define PS2_CMD_DISABLE_PORT2     0xA7
#define PS2_CMD_ENABLE_PORT2      0xA8
#define PS2_CMD_TEST_PORT2        0xA9
#define PS2_CMD_DISABLE_PORT1     0xAD
#define PS2_CMD_ENABLE_PORT1      0xAE
#define PS2_CMD_TEST_PORT1        0xAB

#define PS2_CONFIG_PORT1_INT      0x01
#define PS2_CONFIG_PORT2_INT      0x02
#define PS2_CONFIG_PORT1_CLOCK    0x10
#define PS2_CONFIG_PORT2_CLOCK    0x20
#define PS2_CONFIG_PORT1_TRANS    0x40

#define KBD_CMD_SET_LED           0xED
#define KBD_CMD_ECHO              0xEE
#define KBD_CMD_SET_SCANCODE      0xF0
#define KBD_CMD_IDENTIFY          0xF2
#define KBD_CMD_SET_RATE          0xF3
#define KBD_CMD_ENABLE_SCAN       0xF4
#define KBD_CMD_DISABLE_SCAN      0xF5
#define KBD_CMD_RESET             0xFF

#define KBD_RESPONSE_ACK          0xFA
#define KBD_RESPONSE_RESEND       0xFE

#define SCANCODE_RELEASE_MASK     0x80

typedef struct {
    char normal;
    char shifted;
    uint8_t flags;
} key_mapping_t;

#define KEY_FLAG_ALPHA    0x01
#define KEY_FLAG_MODIFIER 0x02
#define KEY_FLAG_SPECIAL  0x04

static key_mapping_t scancode_map[128] = {
    [0x00] = {0, 0, 0},
    [0x01] = {27, 27, KEY_FLAG_SPECIAL},
    [0x02] = {'1', '!', 0},
    [0x03] = {'2', '@', 0},
    [0x04] = {'3', '#', 0},
    [0x05] = {'4', '$', 0},
    [0x06] = {'5', '%', 0},
    [0x07] = {'6', '^', 0},
    [0x08] = {'7', '&', 0},
    [0x09] = {'8', '*', 0},
    [0x0A] = {'9', '(', 0},
    [0x0B] = {'0', ')', 0},
    [0x0C] = {'-', '_', 0},
    [0x0D] = {'=', '+', 0},
    [0x0E] = {8, 8, KEY_FLAG_SPECIAL},
    [0x0F] = {9, 9, KEY_FLAG_SPECIAL},
    [0x10] = {'q', 'Q', KEY_FLAG_ALPHA},
    [0x11] = {'w', 'W', KEY_FLAG_ALPHA},
    [0x12] = {'e', 'E', KEY_FLAG_ALPHA},
    [0x13] = {'r', 'R', KEY_FLAG_ALPHA},
    [0x14] = {'t', 'T', KEY_FLAG_ALPHA},
    [0x15] = {'y', 'Y', KEY_FLAG_ALPHA},
    [0x16] = {'u', 'U', KEY_FLAG_ALPHA},
    [0x17] = {'i', 'I', KEY_FLAG_ALPHA},
    [0x18] = {'o', 'O', KEY_FLAG_ALPHA},
    [0x19] = {'p', 'P', KEY_FLAG_ALPHA},
    [0x1A] = {'[', '{', 0},
    [0x1B] = {']', '}', 0},
    [0x1C] = {10, 10, KEY_FLAG_SPECIAL},
    [0x1D] = {0, 0, KEY_FLAG_MODIFIER},
    [0x1E] = {'a', 'A', KEY_FLAG_ALPHA},
    [0x1F] = {'s', 'S', KEY_FLAG_ALPHA},
    [0x20] = {'d', 'D', KEY_FLAG_ALPHA},
    [0x21] = {'f', 'F', KEY_FLAG_ALPHA},
    [0x22] = {'g', 'G', KEY_FLAG_ALPHA},
    [0x23] = {'h', 'H', KEY_FLAG_ALPHA},
    [0x24] = {'j', 'J', KEY_FLAG_ALPHA},
    [0x25] = {'k', 'K', KEY_FLAG_ALPHA},
    [0x26] = {'l', 'L', KEY_FLAG_ALPHA},
    [0x27] = {';', ':', 0},
    [0x28] = {'\'', '"', 0},
    [0x29] = {'`', '~', 0},
    [0x2A] = {0, 0, KEY_FLAG_MODIFIER},
    [0x2B] = {'\\', '|', 0},
    [0x2C] = {'z', 'Z', KEY_FLAG_ALPHA},
    [0x2D] = {'x', 'X', KEY_FLAG_ALPHA},
    [0x2E] = {'c', 'C', KEY_FLAG_ALPHA},
    [0x2F] = {'v', 'V', KEY_FLAG_ALPHA},
    [0x30] = {'b', 'B', KEY_FLAG_ALPHA},
    [0x31] = {'n', 'N', KEY_FLAG_ALPHA},
    [0x32] = {'m', 'M', KEY_FLAG_ALPHA},
    [0x33] = {',', '<', 0},
    [0x34] = {'.', '>', 0},
    [0x35] = {'/', '?', 0},
    [0x36] = {0, 0, KEY_FLAG_MODIFIER},
    [0x37] = {'*', '*', 0},
    [0x38] = {0, 0, KEY_FLAG_MODIFIER},
    [0x39] = {' ', ' ', 0},
    [0x3A] = {0, 0, KEY_FLAG_MODIFIER}
};

typedef struct {
    bool left_shift;
    bool right_shift;
    bool left_ctrl;
    bool right_ctrl;
    bool left_alt;
    bool right_alt;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} modifier_state_t;

typedef struct {
    char buffer[256];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} input_buffer_t;

static modifier_state_t modifiers = {0};
static input_buffer_t key_buffer = {0};
static bool key_states[256] = {0};

static void ps2_wait_input(void) {
    uint32_t timeout = 100000;
    while ((inportb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) && --timeout);
}

static void ps2_wait_output(void) {
    uint32_t timeout = 100000;
    while (!(inportb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) && --timeout);
}

static uint8_t ps2_read_data(void) {
    ps2_wait_output();
    return inportb(PS2_DATA_PORT);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_input();
    outportb(PS2_DATA_PORT, data);
}

static void ps2_write_command(uint8_t cmd) {
    ps2_wait_input();
    outportb(PS2_COMMAND_PORT, cmd);
}

static uint8_t ps2_read_config(void) {
    ps2_write_command(PS2_CMD_READ_CONFIG);
    return ps2_read_data();
}

static void ps2_write_config(uint8_t config) {
    ps2_write_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);
}

static bool kbd_send_command(uint8_t cmd) {
    for (int retry = 0; retry < 3; retry++) {
        ps2_write_data(cmd);
        uint8_t response = ps2_read_data();
        if (response == KBD_RESPONSE_ACK) {
            return true;
        }
        if (response != KBD_RESPONSE_RESEND) {
            break;
        }
    }
    return false;
}

static void buffer_put_char(char c) {
    if (key_buffer.count < 255) {
        key_buffer.buffer[key_buffer.head] = c;
        key_buffer.head = (key_buffer.head + 1) % 256;
        key_buffer.count++;
    }
}

static char buffer_get_char(void) {
    if (key_buffer.count == 0) {
        return 0;
    }
    
    char c = key_buffer.buffer[key_buffer.tail];
    key_buffer.tail = (key_buffer.tail + 1) % 256;
    key_buffer.count--;
    return c;
}

static bool buffer_has_data(void) {
    return key_buffer.count > 0;
}

static void update_modifier_state(uint8_t scancode, bool pressed) {
    switch (scancode) {
        case 0x2A: modifiers.left_shift = pressed; break;
        case 0x36: modifiers.right_shift = pressed; break;
        case 0x1D: modifiers.left_ctrl = pressed; break;
        case 0x38: modifiers.left_alt = pressed; break;
        case 0x3A:
            if (pressed) {
                modifiers.caps_lock = !modifiers.caps_lock;
            }
            break;
    }
}

static char process_key(uint8_t scancode) {
    if (scancode >= 128) return 0;
    
    key_mapping_t mapping = scancode_map[scancode];
    if (mapping.flags & KEY_FLAG_MODIFIER) return 0;
    
    bool shift_active = modifiers.left_shift || modifiers.right_shift;
    bool caps_active = modifiers.caps_lock;
    
    char result;
    if (mapping.flags & KEY_FLAG_ALPHA) {
        result = (shift_active != caps_active) ? mapping.shifted : mapping.normal;
    } else {
        result = shift_active ? mapping.shifted : mapping.normal;
    }
    
    return result;
}

static void kbd_interrupt_handler(registers_t* regs) {
    if (!(inportb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL)) {
        return;
    }
    
    uint8_t scancode = inportb(PS2_DATA_PORT);
    bool released = (scancode & SCANCODE_RELEASE_MASK) != 0;
    uint8_t key = scancode & ~SCANCODE_RELEASE_MASK;
    
    key_states[key] = !released;
    update_modifier_state(key, !released);
    
    if (!released) {
        char c = process_key(key);
        if (c != 0) {
            buffer_put_char(c);
        }
    }
}

void init_keyboard(void) {
    ps2_write_command(PS2_CMD_DISABLE_PORT1);
    ps2_write_command(PS2_CMD_DISABLE_PORT2);
    
    while (inportb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        inportb(PS2_DATA_PORT);
    }
    
    uint8_t config = ps2_read_config();
    config &= ~(PS2_CONFIG_PORT1_INT | PS2_CONFIG_PORT2_INT | PS2_CONFIG_PORT1_TRANS);
    ps2_write_config(config);
    
    ps2_write_command(PS2_CMD_TEST_PORT1);
    if (ps2_read_data() != 0x00) {
        return;
    }
    
    ps2_write_command(PS2_CMD_ENABLE_PORT1);
    
    config = ps2_read_config();
    config |= PS2_CONFIG_PORT1_INT;
    ps2_write_config(config);
    
    kbd_send_command(KBD_CMD_RESET);
    ps2_read_data();
    
    kbd_send_command(KBD_CMD_SET_SCANCODE);
    ps2_write_data(1);
    
    kbd_send_command(KBD_CMD_ENABLE_SCAN);
    
    register_interrupt_handler(IRQ1, kbd_interrupt_handler, "Keyboard Handler");
    log("Keyboard Initialized.", 1, 0);
}

char get_key(void) {
    char c = buffer_get_char();
    return c;
}

char wait_for_key(void) {
    while (!buffer_has_data());
    return get_key();
}

void read_line(char *buffer, size_t max_size) {
    size_t pos = 0;
    char c;
    
    while (pos < max_size - 1) {
        c = wait_for_key();
        
        if (c == 10) {
            buffer[pos] = '\0';
            return;
        } else if (c == 8) {
            if (pos > 0) {
                pos--;
            }
        } else if (c >= 32 && c <= 126) {
            buffer[pos++] = c;
        }
    }
    
    buffer[max_size - 1] = '\0';
    return;
}

bool is_key_pressed(uint8_t scancode) {
    return key_states[scancode];
}

bool is_shift_pressed(void) {
    return modifiers.left_shift || modifiers.right_shift;
}

bool is_ctrl_pressed(void) {
    return modifiers.left_ctrl || modifiers.right_ctrl;
}

bool is_alt_pressed(void) {
    return modifiers.left_alt || modifiers.right_alt;
}

bool is_caps_lock_on(void) {
    return modifiers.caps_lock;
}