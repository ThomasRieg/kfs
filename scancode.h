#define SET1_QW_SHIFT 0x2a
#define SET1_QW_CAPLOCK 0x3a
#define SET1_QW_LCTRL 0x1D //TODO test this ont campus

static char scan_code_set_1_qwerty[128] = {
	0, '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y',
	'u', 'i', 'o', 'p', '[', ']', '\r', 0, 'a', 's', 'd',
	'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};
