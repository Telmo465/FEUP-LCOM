#include <lcom/lcf.h>

#include <stdint.h>

#include "i8042.h"
#include "mouse.h"

int ms_hook_id = 2;
int mouse_id = KBD_AUX_IRQ;
uint8_t BYTE1; // output byte
u32_t scan_code_mouse=0;
uint32_t num_packets=0;
uint8_t byte;

int (mouse_subscribe_int)(uint8_t *bit_no) //Subscribed mouse interrupts
{
    *bit_no = (uint8_t) ms_hook_id;
    if(sys_irqsetpolicy(12, IRQ_REENABLE | IRQ_EXCLUSIVE, &ms_hook_id)) return 1;

    return 0;
}

int (mouse_unsubscribe)()  //Unsubscribed mouse interrupts
{
    if(sys_irqrmpolicy(&ms_hook_id)) return 1;
    return 0;
}

void (mouse_ih)(){
  uint32_t stat, output;
  if (sys_inb(0x64, &stat)) return;
  if (stat & BIT(0)){
    if (sys_inb(0x60, &output)) return;
    if (stat & (BIT(6) | BIT(7)))
      BYTE1 = 0;
    else BYTE1 = output;
  }
  else BYTE1 = 0;
}

int (write_mouse_command)(u32_t command) //Writes the command specified in its argument to the mouse and checks the acknowledgement
    {
    uint32_t response, stat;

    while(true) {
        while( true ) {
            if(sys_inb(STAT_REG, &stat)) return 1;
            if( (stat & BIT(1)) == 0 ) {
                if(sys_outb(0x64, 0xD4)) return 1;
                break;
            }
            tickdelay(micros_to_ticks(DELAY_US));
        }

        while( true ) {
            if(sys_inb(STAT_REG, &stat)) return 1;
            if( (stat & BIT(1)) == 0 ) {
                if(sys_outb(0x60, command)) return 1;
                break;
            }
            tickdelay(micros_to_ticks(DELAY_US));
        }

        if( sys_inb(0x60, &response)) return 1;

        if(response == ERROR) return 1;
        else if (response == ACK) return 0;

        tickdelay(micros_to_ticks(DELAY_US));
    }
}


void (mouse_parse_packet)(uint8_t *bytes,struct packet *pp){
    pp->bytes[0]=bytes[0];
    pp->bytes[1]=bytes[1];
    pp->bytes[2]=bytes[2];

    pp->lb= pp->bytes[0] & MOUSE_LB;
    pp->rb= pp->bytes[0] & MOUSE_RB;
    pp->mb= pp->bytes[0] & MOUSE_MB;

    pp->delta_x= pp->bytes[1];
    pp->delta_y= pp->bytes[2];

    pp->x_ov= pp->bytes[0] & MOUSE_X_OVFL;
    pp->y_ov= pp->bytes[0] & MOUSE_Y_OVFL;

    if (pp->bytes[0] & MOUSE_X_SIGN) pp->delta_x |= (0x00FF << 8);
    if (pp->bytes[0] & MOUSE_Y_SIGN) pp->delta_y |= (0x00FF << 8);
}
