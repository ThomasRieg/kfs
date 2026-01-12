#include "scancode.h"
#include "tty/tty.h"
#include "libk/libk.h"
#include "common.h"

bool shift_held = false;
bool caps_lock = false;
bool lctrl_held = false;
unsigned char scancode = 0;

void keyboard_handler(__attribute__((unused)) t_regs *regs)
{
	scancode = inb(PORT_PS2_DATA);
	// pic_eoi(INT_KEYBOARD);
}

void handle_ps2(void) {
	if (scancode != 0) {
		if (!(scancode >> 7))
		{ // if not a release event
			char c;
			if (shift_held)
				c = scan_code_set_1_qwerty_shifted[scancode];
			else
				c = scan_code_set_1_qwerty[scancode];
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
			{
				tty_add_input(c);
			}
			else if (scancode == SET1_QW_CAPLOCK)
			{
				caps_lock = !caps_lock;
			}
			else if (scancode == SET1_QW_SHIFT)
			{
				shift_held = true;
			}
			else if (scancode == SET1_QW_LCTRL)
			{
				lctrl_held = true;
			}
		}
		else
		{ // if release
			scancode &= 0x7F;
			if (scancode == SET1_QW_SHIFT)
			{
				shift_held = false;
			}
			else if (scancode == SET1_QW_LCTRL)
			{
				lctrl_held = false;
			}
		}
		scancode = 0;
	}
}
