#ifndef _KEYBOARD_H
#define _KEYBOARD_H


#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>

#include "i8042.h"
/**
* @brief Handles keyboard interrupts
*
* Reads the status register, the output buffer and the mouse data
* If there was some error, the byte read from the OB should be discarded
*
* All communication with other code must be done via global variables, static if possible
*
* Must be defined using parenthesis around the function name:
*/
void (kbc_ih)(void);
/**
 * @brief Invokes sys_inb() system call but reads the value into a uint8_t variable.
 *
 * @param port the input port that is to be read
 * @param value address of 8-bit variable to be update with the value read
 * @return Return 0 upon success and non-zero otherwise
 */
int (util_sys_inb)(int port, uint8_t* value);
/**
 * @brief Subscribes and enables Keyboard interrupts
 *
 * @param bit_no address of memory to be initialized with the
 *         bit number to be set in the mask returned upon an interrupt
 * @return Return 0 upon success and non-zero otherwise
 */
int(kbc_subscribe_int)(uint8_t* bit_no);

/**
 * @brief Unsubscribes Keyboard interrupts
 *
 * @return Return 0 upon success and non-zero otherwise
 */
int(kbc_unsubscribe_int)();

void (kbc_poll)();

int (reenable_interrupt)();
#endif /*_KEYBOARD_H */
