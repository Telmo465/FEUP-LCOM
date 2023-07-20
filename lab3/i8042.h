#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_
#include <lcom/lcf.h>
/** @defgroup i8042 i8042
 * @{
 *
 * Constants for programming the i8042 Keyboard.
 */
 //#define KEYBOARD_IRQ 0x09 /* Keyboard IRQ address */
 //#define BIT(n) (0x01<<(n))
#define DELAY_US        20000
#define STAT_REG_PAR                  BIT(7)  /* Parity Error */
#define STAT_REG_TIMEOUT        BIT(6)  /* Timeout Error */
#define STAT_REG_AUX                  BIT(5)  /* Mouse Data */
#define STAT_REG_INH                  BIT(4)  /* Inhibit Flag */
#define STAT_REG_A2       BIT(3)  /* A2 Input Line */
#define STAT_REG_SYS                  BIT(2)  /* System Flag */ 
#define STAT_REG_IBF                   BIT(1)  /* Input Buffer Full */
#define STAT_REG_OBF                  BIT(0)  /* Output Buffer Full */
#define READ_COMMAND        0x20   /* Returns Command Byte */
#define OUTPUT_BUF         0x60    /* Output Buffer */
#define INPUT_BUF          0x60    /* Input Buffer */
#define STATUS_REG         0x64    /* Status Register */
#define KBC_CMD_REG        0x64    /* Command Register */
#define INT_ENABLE         BIT(0) /*enable interrupt on OBF, from keyboard*/
#define KBC_CMD_ARG        0x60    /* Command Arguments Adress */
#define ESC_BREAK           0x81
#define TWO_BYTE_SCANCODE   0xE0  /* Two-byte long scancodes use 0xE0 as their first byte */
#define MAKE_CODE           BIT(7)
#endif /* _LCOM_I8042_H */
