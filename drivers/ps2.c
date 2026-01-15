#include "../scancode.h"
#include "../tty/tty.h"
#include "../libk/libk.h"
#include "../common.h"

static bool shift_held = false;
static bool caps_lock = false;
static bool lctrl_held = false;
static unsigned char scancode = 0;
static unsigned char *layout[2] = { scan_code_set_1_qwerty, scan_code_set_1_qwerty_shifted };

void keyboard_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	scancode = inb(PORT_PS2_DATA);
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

void handle_ps2(void) {
	if (scancode != 0) {
		if (!(scancode >> 7))
		{ // if not a release event
			char c;
			if (shift_held)
				c = layout[1][scancode];
			else
				c = layout[0][scancode];
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
