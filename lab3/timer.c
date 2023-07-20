#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  uint8_t config, controlw;
  uint8_t lsb, msb;
  if (timer<0||timer>2) return 1;
  if (freq<19 || freq>TIMER_FREQ) return 1;

  if(timer_get_conf(timer, &config)) return 1;

  config &= 0x0f;
  controlw = config | TIMER_LSB_MSB;

  if (timer == 0)
    controlw |= TIMER_SEL0;
  else if (timer == 1)
    controlw |= TIMER_SEL1;
  else if (timer == 2)
    controlw |= TIMER_SEL2;
  else return 1;

  if(sys_outb(TIMER_CTRL, controlw)) return 1;

  uint16_t initialValue = (uint16_t) (TIMER_FREQ / freq);

  if(util_get_LSB(initialValue, &lsb)) return 1;
  if(util_get_MSB(initialValue, &msb)) return 1;
  
  if(sys_outb(TIMER_ADDR(timer), lsb)) return 1;
  if(sys_outb(TIMER_ADDR(timer), msb)) return 1;

  return 0;
}

int id = 0;

int (timer_subscribe_int)(uint8_t *bit_no) {
  *bit_no = (uint8_t) id;
  if(sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &id)) return 1;
  return 0;
}

int (timer_unsubscribe_int)() {
  if(sys_irqrmpolicy(&id)) return 1;
  return 0;
}

uint32_t interrupts = 0; 

void (timer_int_handler)() {
  interrupts++;
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) 
{
  if (timer < 0 || timer > 2)
    return 1;
  uint8_t mode;
  uint32_t ler = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);
  
  if(sys_outb(TIMER_CTRL, ler)) return 1;
  
  if (timer == 0){
    if (!util_sys_inb(TIMER_0, &mode)){
      *st = mode;
      return 0;
    }
    else return 1;
  }
  else if (timer == 1){
    if (!util_sys_inb(TIMER_1, &mode)){
      *st = mode;
      return 0;
    }
    else return 1;
  }
  else if (timer == 2){
    if (!util_sys_inb(TIMER_2, &mode)){
      *st = mode;
      return 0;
    }
    else return 1;
  }

  return 1;
}

int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
  if (timer < 0 || timer>2) return 1;
  union timer_status_field_val VAR;

  if (field == tsf_all){
    VAR.byte = st;
  }

  if (field == tsf_mode){
    st &= 0x0E;
    st >>= 1;
    if (st == 6) 
      VAR.count_mode = 2; 
    else if (st == 7) 
      VAR.count_mode = 3;
    else
      VAR.count_mode = st;
  }

  if (field == tsf_initial) {
    st >>= 4;
    st &= 0x30;

    VAR.in_mode = st;
  }
  
  if (field == tsf_base){
    if (st & TIMER_BCD) 
      VAR.bcd = true;
    else  VAR.bcd = false;
  }

  if(timer_print_config(timer, field, VAR)) 
    return 1;
  return 1;
}
