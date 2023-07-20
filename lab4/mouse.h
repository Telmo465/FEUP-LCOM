#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <minix/syslib.h>
#include <minix/sysutil.h>

enum state_t {
  INIT,     /* First state, wainting to press the left bottom  */
  DRAW,     /* 2 state, drawing the first line */
  DONELINE, /* 3 state, wainting to press the right bottom */
  DRAW1,    /* 4 state , drawing the second line*/
};

typedef enum { LDOWN,
               LUP,
               RDOWN,
               RUP,
               WRONGKEY } ev_type_t;

int mouse_subscribe_int(uint16_t *bit__no);

int mouse_unsubscribe_int();

int kbc_read_out_buf(uint8_t *content);

int kbc_write(port_t port, uint8_t cmd);

int write_mousecmd(uint8_t cmd);

int mouse_parse_packet(struct packet *pp);

void gest_handler(struct packet *pp, uint8_t x_len, uint8_t tolerance);

bool(first_line_completed)(uint8_t x_len, uint8_t tolerance, struct packet *pp);

bool(second_line_completed)(uint8_t x_len, uint8_t tolerance, struct packet *pp);

void(first_line_handler)(int negative_tolerance, struct packet *pp);

void(second_line_handler)(int x_tol, uint8_t y_tol, struct packet *pp);

void(event_detector)(struct packet *pp, ev_type_t *evt);

bool(event_handler)(ev_type_t evt, struct packet *pp, uint8_t x_len, int negative_tolerance, uint8_t tolerance);


struct mouse_ev *mouse_event_detect(struct packet *pp);
#endif
