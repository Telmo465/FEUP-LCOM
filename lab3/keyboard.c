#include "keyboard.h"


uint8_t scancode = 0;
uint32_t counter = 0;
int kbc_id = KEYBOARD_IRQ;
uint32_t icalls = 0;


int (util_sys_inb)(int port, uint8_t* v) {

	uint32_t nval;

	if (sys_inb(port, &nval) != 0) {
		printf("Error in util_sys_inb\n");
		return 1;
	}
	*v = nval & 0xFF;
#ifdef LAB3
	icalls++;
# endif

	return 0;
}

void (kbc_ih)(void) {
	uint8_t status_reg;

	if (util_sys_inb(0x64, &status_reg) != 0) {
		return;
	}
	if (((status_reg & STAT_REG_OBF) == 0) || ((status_reg & (STAT_REG_PAR | STAT_REG_TIMEOUT)) != 0) || ((status_reg & STAT_REG_AUX) != 0)) {
		return;
	}
	util_sys_inb(0x60, &scancode);

}


int (kbc_subscribe_int)(uint8_t* b) {
	*b = BIT(kbc_id);
	if (sys_irqsetpolicy(KEYBOARD_IRQ, (IRQ_REENABLE | IRQ_EXCLUSIVE), &kbc_id) == 1) {
		printf("Error subscribing int\n");
		return 1;
	}
	return 0;
}

int (kbc_unsubscribe_int)() {
	if (sys_irqrmpolicy(&kbc_id) == 1) {
		printf("Error unsubscribing int\n");
		return 1;
	}
	return 0;
}

int sys_inb_cnt(port_t port, uint8_t* byte) {
	uint32_t byte32;
	if (sys_inb(port, &byte32)) {
		return 1;
	}
	*byte = (uint8_t)byte32;
	icalls++;
	return 0;
}

void (kbc_poll)()
{
	uint8_t st;

	if (sys_inb_cnt(0x64, &st)) return;

	if (st & STAT_REG_OBF) {
		if ((st & STAT_REG_AUX) == 0) {
			if (sys_inb_cnt(0x60, &scancode)) return;

			if ((st & (STAT_REG_PAR | STAT_REG_TIMEOUT)) != 0)
				scancode = 0;
		}
	}
	else scancode = 0;

}

int scanCodeHandler(bool* make, uint8_t* size, uint8_t* bytes)//simplified function to read the scanCode
{
	int handler = 0;

	if (scancode != 0) //Means that there wasn't any error while reading the output buffer
	{
		if (scancode == 0x81) handler = 2; //Loop Ends

		if (scancode != 0xE0) {
			if (*size) {
				(*size)++;
				bytes[1] = scancode;
			}
			else {
				(*size)++;
				bytes[0] = scancode;
			}

			if (scancode & BIT(7)) *make = false;
			else *make = true;

			if (handler == 2) return handler; //Means that the ESC key breakcode should be printed and loop should end
			else return 1; //Means that the scancode should be printed and size should be put at zero and jump to the next iteration
		}
		else {
			bytes[0] = scancode;
			(*size)++;
			return 0; // Means that the second byte of the scancode should be read in the next iteration
		}
	}
	return 0; //There was an error or the OUT_BUF was empty. Should proceed to the nest iteration
}

int kbc_cmd_handler(uint32_t port, uint8_t cmd) {
	uint32_t status;
	int i = 0;
	while (true) {
		if (sys_inb(0x64, &status)) {
			printf("Error reading status\n");
			return 1;
		}
		if (status & BIT(0)) {
			util_sys_inb(0x60, &scancode);
		}
		if (!(status & BIT(1))) {
			if (sys_outb(port, cmd)) {
				printf("Error writing cmd\n");
				return 1;
			}
			return 0;
		}
		i++;
	}
}

int (reenable_interrupt)()
{
	uint8_t command;
	if (sys_outb(0x64, 0x20) != 0) {
		printf("Error writing read-command\n");
		return 1;
	}
	if (util_sys_inb(0x60, &command) != 0) {
		printf("Error reading output buffer in enable_interrupt\n");
		return 1;
	}
	command = command | BIT(0);
	if (sys_outb(0x64, 0x60) != 0) {
		printf("Error writing output buffer\n");
		return 1;
	}
	if (sys_outb(0x60, command) != 0) {
		printf("Error writing new buffer status\n");
		return 1;
	}
	return 0;
}

