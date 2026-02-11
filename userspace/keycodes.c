#include <stdio.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

int main(void) {
	for (unsigned int scancode = 0; scancode < 256; scancode++) {
		struct kbkeycode kc;
		kc.scancode = scancode;
		if (ioctl(0, KDGETKEYCODE, &kc))
			perror("ioctl");

		printf("%u -> %u\n", scancode, kc.keycode);
	}
}
