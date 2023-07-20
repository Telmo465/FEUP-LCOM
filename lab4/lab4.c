// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <stdint.h>
#include <stdio.h>

// Any header files included below this line should have been created by you

#include "mouse.h"
#include "i8254.h"
#include "i8042.h"

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need/ it]
  lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab4/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

extern int mouse_id;
extern uint8_t BYTE1;
extern uint8_t byte;

int(mouse_test_packet)(uint32_t cnt) {
  int r, istatus;
  message msg;
  unsigned int b = 0;
  uint32_t printscnt = 0;
  uint16_t m_irq;
  
  struct packet p;
	
  if (mouse_subscribe_int(&m_irq)) {
    printf("Could not subscribe Mouse!\n");
    return 1;
  }

  sys_irqdisable(&mouse_id);
  if (write_mousecmd(0xF4)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);

  while (cnt > printscnt) {
    if ((r = driver_receive(ANY, &msg, &istatus)) != 0) {
      printf("ERROR: driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(istatus)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & m_irq) { 
            mouse_ih();
            if ((b == 0 && (BIT(3) & BYTE1)) || b == 1 || b == 2) {
              p.bytes[b] = BYTE1; 
              b++;
            }
            if (b == 3) {
              b = 0;
              mouse_parse_packet(&p);
              mouse_print_packet(&p);
              printscnt++;
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

  //Resetting MINIX's mouse state to its default
  sys_irqdisable(&mouse_id);
  if (write_mousecmd(0xF5)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);


  if (mouse_unsubscribe_int()) {
    printf("Could not unsubscribe Mouse!\n");
    return 1;
  }
  
  return 0;
}

int(mouse_test_remote)(uint16_t period, uint8_t cnt) {
  uint8_t counter_ppp=0, stat;
  struct packet ppp;

  while (counter_ppp <cnt ){ 
    if(write_mousecmd(0xEB)!=0){     
      printf("Error\n");
      return 1;
    }
    if(  (util_sys_inb(STAT_REG,&stat)) != (0) ){
      printf("Error\n");
      return 1;
    }
    if((stat & 0x01)==0x01){
      if((stat & (STAT_REG_PAR|STAT_REG_TIMEOUT))==0){
        for(int size=0;size<3;size++){
          if (util_sys_inb(OUT_BUF, &BYTE1) != 0) {
          printf("Error\n");
          return 1;
          }
          if ((size == 0 && (BIT(3) & BYTE1)) || size == 1 || size == 2) {                                                                   
            ppp.bytes[size] = BYTE1;
          }
        }
        mouse_parse_packet(&ppp); 
        mouse_print_packet(&ppp); 
      }
    }
    counter_ppp++;                       
    tickdelay(micros_to_ticks(period*1000));
  }

  if (write_mousecmd(0xEA) != 0) {
	printf("Error \n");
	return 1;
  }
  if (write_mousecmd(0xF5) != 0) {    
    printf("Error \n");
    return 1;
  }
  if (sys_outb(0X64,0x60)!=0){
    printf("Error");
    return 1;
  }

  uint8_t default_cmd=minix_get_dflt_kbc_cmd_byte();
  
  if (sys_outb(0x60,default_cmd)!=0){ 
	printf("Error");
    return 1;
  }
  
  BYTE1 = 0;
  return 0;
}

int(mouse_test_async)(uint8_t idle_time) {
  unsigned int b = 0; 
  struct packet p;
  uint16_t m_irq;
  uint8_t t_irq;
  int r, istatus;
  message msg;
  extern unsigned int counter;

  if (timer_subscribe_int(&t_irq) || mouse_subscribe_int(&m_irq)) {
    printf("Could not subscribe Timer/Mouse!\n");
    return 1;
  }

  sys_irqdisable(&mouse_id); 
  if (write_mousecmd(0xF4)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);

  while (counter < (idle_time * sys_hz())) {
    if ((r = driver_receive(ANY, &msg, &istatus)) != 0) {
      printf("ERROR: driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(istatus)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & m_irq) {
            counter = 0;
            mouse_ih();
            if ((b == 0 && (BIT(3) & BYTE1)) || b == 1 || b == 2) { 
              p.bytes[b] = BYTE1;
              b++;
            }
            if (b == 3) {
              b = 0;
              mouse_parse_packet(&p);
              mouse_print_packet(&p);
            }
          }
          else if (msg.m_notify.interrupts & t_irq) {
            timer_int_handler();
          }
          break;
        default:
          break;
      }
    }
  }

  sys_irqdisable(&mouse_id);
  if (write_mousecmd(0xF5)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);

  if (mouse_unsubscribe_int()) {
    printf("Could not unsubscribe Mouse!\n");
    return 1;
  }

  if (timer_unsubscribe_int()) {
    printf("Could not unsubscribe Timer!\n");
    return 1;
  }
  return 0;
}

int(mouse_test_gesture)(uint8_t x_len, uint8_t tolerance) {
  int istatus, r;
  message msg;
  uint16_t m_irq;
  unsigned int b = 0;
  extern unsigned int counter;

  struct packet p;

  int n_tolerance = tolerance;
  n_tolerance *= -1;

  ev_type_t evt;
  bool pass = true;
 
  if (write_mousecmd(0xF4))
    return 1;

  if (mouse_subscribe_int(&m_irq)) {
    printf("Could not subscribe Mouse!\n");
    return 1;
  }
  
  sys_irqdisable(&mouse_id);
  if (write_mousecmd(0xF4)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);

  while (pass){
    if ((r = driver_receive(ANY, &msg, &istatus)) != 0) {
      printf("driver_receive failes with: %d", r);
      continue;
    }

    if (is_ipc_notify(istatus)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & m_irq) {
            counter = 0;
            mouse_ih();
            if ((b == 0 && (BIT(3) & BYTE1)) || b == 1 || b == 2) {
              p.bytes[b] = BYTE1;
              b++;
            }
            if (b == 3) {
              b = 0;
              mouse_parse_packet(&p);
              mouse_print_packet(&p);
            }
            event_detector(&p, &evt);
            pass = event_handler(evt, &p, x_len, n_tolerance, tolerance);
          }
        default:
          break;
      }
    }
  }

  //Resetting MINIX's mouse state to its default	
  sys_irqdisable(&mouse_id);
  if (write_mousecmd(0xF5)) {
    printf("Could not disable data reporting!");
    return 1;
  }
  sys_irqenable(&mouse_id);

  if (mouse_unsubscribe_int()) {
    printf("Could not unsubscribe Mouse!\n");
    return 1;
  }
  return 0;
}
