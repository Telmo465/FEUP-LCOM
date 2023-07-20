#include <lcom/lcf.h>
#include <lcom/lab3.h>
#include <lcom/timer.h>
#include <stdbool.h>
#include <stdint.h>

#include "keyboard.h"


int main(int argc, char* argv[]) {
	// sets the language of LCF messages (can be either EN-US or PT-PT)
	lcf_set_language("EN-US");

	// enables to log function invocations that are being "wrapped" by LCF
	// [comment this out if you don't want/need it]
	lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

	// enables to save the output of printf function calls on a file
	// [comment this out if you don't want/need it]
	lcf_log_output("/home/lcom/labs/lab3/output.txt");

	// handles control over to LCF
	// [LCF handles command line arguments and invokes the right function]
	if (lcf_start(argc, argv))
		return 1;

	// LCF clean up tasks
	// [must be the last statement before return]
	lcf_cleanup();

	return 0;
}

extern uint8_t scancode;
extern uint32_t icalls;
extern uint32_t interrupts;
unsigned int counter;

int(kbd_test_scan)() {
	bool make;
	int istatus, r;
	message msg;
	uint8_t bytes[2], size, k_irq;

	if (kbc_subscribe_int(&k_irq) == 1) {
		printf("Error subscribing int\n");
		return 1;
	}

	while (scancode != 0x81) {
		if ((r = driver_receive(ANY, &msg, &istatus)) == 1) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(istatus)) {
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE:
				if (msg.m_notify.interrupts & k_irq) {

					kbc_ih();
					bytes[0] = scancode;
					bytes[1] = scancode;

					if (bytes[0] == 0xE0) {
						size = 2;
					}
					else {
						size = 1;
					}

					make = false;

					if (scancode & BIT(7)) {
						make = false;
					}
					else {
						make = true;
					}

					if (kbd_print_scancode(make, size, bytes) != 0) {
						printf("Error printing scan code\n");
						return 1;
					}
				}
				break;
			default:
				break;
			}
		}
		else {
		}
	}

	if (kbc_unsubscribe_int() != 0) {
		printf("Error unsubscribing int \n");
		return 1;
	}

	if (kbd_print_no_sysinb(icalls) != 0) {
		printf("Error printing number of sys used\n");
		return 1;
	}

	icalls = 0;
	scancode = 0;

	return 0;
}

int(kbd_test_poll)() {

	uint8_t status, size, bytes[2];
	bool make = false;
	while (scancode != ESC_BREAK) {
		if (util_sys_inb(STATUS_REG, &status) != 0) {
			printf("Error\n");
			return 1;
		}
		if ((status & STAT_REG_OBF) && !(status & STAT_REG_AUX) && !(status & STAT_REG_PAR || status & STAT_REG_TIMEOUT)) {
			if (util_sys_inb(OUTPUT_BUF, &scancode) != 0) {
				printf("Error\n");
				return 1;
			}
			bytes[0] = scancode;
			bytes[1] = scancode;
			if (bytes[0] == TWO_BYTE_SCANCODE) {
				size = 2;
			}
			else {
				size = 1;
			}
			if (scancode & MAKE_CODE) {
				make = false;
			}
			else {
				make = true;
			}
			if (kbd_print_scancode(make, size, bytes) != 0) {
				printf("Error\n");
				return 1;
			}

		}
		else {
			tickdelay(micros_to_ticks(DELAY_US));
		}
	}
	if (kbd_print_no_sysinb(icalls) != 0) {
		printf("Error\n");
		return 1;
	}
	if (reenable_interrupt() != 0) {
		printf("Error re-enabling\n");
		return 1;
	}
	icalls = 0;
	scancode = 0;
	return 0;
}

int(kbd_test_timed_scan)(uint8_t n) {
	if (n < 0) {
		printf("Invalid time\n");
		return 1;
	}
	int r, istatus;
	uint8_t kbc_irq, size, bytes[2], t_irq;
	message msg;
	bool make = false;
	if (timer_subscribe_int(&t_irq) != 0) {
		printf("Error subscribing int (timer)\n");
		return 1;
	}
	if (kbc_subscribe_int(&kbc_irq) != 0) {
		printf("Error subscribing int (keyboard)\n");
		return 1;
	}
	while ((scancode != ESC_BREAK) && (counter / sys_hz() < n)) {
		if ((r = driver_receive(ANY, &msg, &istatus)) == 1) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(istatus)) {
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE:
				if (msg.m_notify.interrupts & kbc_irq) {

					kbc_ih();
					bytes[0] = scancode;
					bytes[1] = scancode;
					if (bytes[0] == TWO_BYTE_SCANCODE) {
						size = 2;
					}
					else {
						size = 1;
					}
					if (scancode & MAKE_CODE) {
						make = false;
					}
					else {
						make = true;
					}
					if (kbd_print_scancode(make, size, bytes) != 0) {
						printf("Error printing scan code\n");
						return 1;
					}
					counter = 0;
				}
				else if (msg.m_notify.interrupts & t_irq) {
					timer_int_handler();
					counter++;
				}
				break;
			default:
				break;
			}
		}
		else {
		}
	}
	if (kbc_unsubscribe_int() != 0) {
		printf("Error unsubscribing int \n");
		return 1;
	}
	if (timer_unsubscribe_int() != 0) {
		printf("Error unsubscribing int\n");
		return 1;
	}
	if (kbd_print_no_sysinb(icalls) != 0) {
		printf("Error printing number of sys used\n");
		return 1;
	}
	icalls = 0;
	scancode = 0;
	return 0;

}
