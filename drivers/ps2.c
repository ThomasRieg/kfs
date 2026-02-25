#include "../scancode.h"
#include "../tty/tty.h"
#include "../libk/libk.h"
#include "../tasks/task.h"
#include "../common.h"

static bool shift_held = false;
static bool caps_lock = false;
static bool lctrl_held = false;
static bool extended = false;
static unsigned char *layout[2] = { scan_code_set_1_qwerty, scan_code_set_1_qwerty_shifted };

bool multi_char_inputs(unsigned char scancode) {
	if (extended) {
		switch (scancode) {
		case 0x48: // cursor up
			tty_add_input('\e');
			tty_add_input('[');
			tty_add_input('A');
			return true;
		case 0x50: // cursor down
			tty_add_input('\e');
			tty_add_input('[');
			tty_add_input('B');
			return true;
		case 0x4D: // cursor right
			tty_add_input('\e');
			tty_add_input('[');
			tty_add_input('C');
			return true;
		case 0x4B: // cursor left
			tty_add_input('\e');
			tty_add_input('[');
			tty_add_input('D');
			return true;
		}
	}
	return false;
}

void handle_ps2(unsigned char scancode) {
	if (scancode != 0) {
		if (!(scancode >> 7))
		{ // if not a release event
			char c;
			if (multi_char_inputs(scancode))
				return;

			if (shift_held)
				c = layout[1][scancode];
			else
				c = layout[0][scancode];
			char upper_c = (c >= 'a' && c <= 'z') ? (c - 32) : c;
			if (lctrl_held && upper_c >= 'A' && upper_c <= '_')
				c = upper_c - 0x40;

			if (caps_lock)
				c = invert_caps(c);

			if (lctrl_held && c == '+')
			{
				save_tty();
				next_tty();
			}
			else if (lctrl_held && c == '-')
			{
				save_tty();
				prev_tty();
			}
			else if (c)
				tty_add_input(c);
			else if (scancode == SET1_QW_CAPLOCK)
				caps_lock = !caps_lock;
			else if (scancode == SET1_QW_SHIFT)
				shift_held = true;
			else if (scancode == SET1_QW_LCTRL)
				lctrl_held = true;
		}
		else
		{ // if release
			unsigned char key_scancode = scancode & 0x7F;
			if (key_scancode == SET1_QW_SHIFT)
				shift_held = false;
			else if (key_scancode == SET1_QW_LCTRL)
				lctrl_held = false;
		}
		if (scancode == 0xE0) {
			extended = true;
		} else
			extended = false;
		scancode = 0;
	}
}

void keyboard_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	handle_ps2(inb(PORT_PS2_DATA));
	// pic_eoi(INT_KEYBOARD);
}

void tty_swap_layout(void) {
	if (layout[0] == scan_code_set_1_qwerty) {
		layout[0] = scan_code_set_1_azerty;
		layout[1] = scan_code_set_1_azerty_shifted;
		printk("You are now using the AZERTY layout.\n");
	} else {
		layout[0] = scan_code_set_1_qwerty;
		layout[1] = scan_code_set_1_qwerty_shifted;
		printk("You are now using the QWERTY layout.\n");
	}
}
