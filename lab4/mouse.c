#include <lcom/lcf.h>

#include <stdint.h>

#include "i8042.h"
#include "mouse.h"

int mouse_id = KBD_AUX_IRQ;
uint8_t BYTE1; // output byte

int(mouse_subscribe_int)(uint16_t *bit_no) {
  *bit_no = BIT(mouse_id);
  if (sys_irqsetpolicy(KBD_AUX_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &mouse_id))
    return 1;
  return 0;
}

int(mouse_unsubscribe_int)() {
  if (sys_irqrmpolicy(&mouse_id) != 0)
    return 1;
  return 0;
}

void(mouse_ih)() {
  kbc_read_out_buf(&BYTE1);
}

int kbc_read_out_buf(uint8_t *content) {
  uint32_t status;
  uint32_t content32; // 32-bit mask for the 8-bit content
  int retry = 0;

  while (retry < 4) {
    sys_inb(STAT_REG, &status);

    //... Checking if there is any error and if the byte came from AUX
    if ((status & STAT_REG_PAR) || (status & STAT_REG_TIMEOUT) || !(status & STAT_REG_AUX)) { // Parity or Timeout error, invalid data
      return 1;
    }

    //... Checking if OUT_BUF is FULL to read
    if (status & STAT_REG_OBF) {
      //... Reading the Output Buffer
      if (sys_inb(OUT_BUF, &content32)) {
        return 1;
      }
      // The information in address content now contains content32 in 8-bit
      *content = (uint8_t) content32;
      return 0;
    }

    tickdelay(micros_to_ticks(DELAY_US)); // IF NOT EMPTY wait for IN_BUF to be empty
    retry++;
  }

  return 1;
}

int kbc_write(port_t port, uint8_t cmd) {
  uint32_t status;
  int retry = 0;

  while (retry < 4) {
    sys_inb(STAT_REG, &status);

    //... Checking if IN_BUF is OK to write to
    if (!(status & STAT_REG_IBF)) {
      //.... Writing the command
      if (sys_outb(port, cmd)) {
        return 1;
      }
      return 0;
    }

    tickdelay(micros_to_ticks(DELAY_US)); // IF NOT EMPTY wait for IN_BUF to be empty
    retry++;
  }

  return 1;
}

int mouse_parse_packet(struct packet *pp) {

  //... Checking X Coordinate Sign
  if (pp->bytes[0] & MOUSE_X_SIGN) {
    pp->delta_x = -pp->bytes[1];
    pp->delta_x ^= 0xFF;
    pp->delta_x += 1;
  }
  else {
    pp->delta_x = pp->bytes[1];
  }

  //... Checking Y Coordinate Sign
  if (pp->bytes[0] & MOUSE_Y_SIGN) {
    pp->delta_y = -pp->bytes[2];
    pp->delta_y ^= 0xFF;
    pp->delta_y += 1;
  }
  else {
    pp->delta_y = pp->bytes[2];
  }

  //... Checking coordinates overflow
  if (pp->bytes[0] & MOUSE_X_OVFL) { // X Coordinate
    pp->x_ov = true;
  }
  else {
    pp->x_ov = false;
  }

  if (pp->bytes[0] & MOUSE_Y_OVFL) { // Y Coordinate
    pp->y_ov = true;
  }
  else {
    pp->y_ov = false;
  }

  //... Checking mouse buttons
  if (pp->bytes[0] & MOUSE_LB) { // Left Button
    pp->lb = true;
  }
  else {
    pp->lb = false;
  }

  if (pp->bytes[0] & MOUSE_RB) { // Right Button
    pp->rb = true;
  }
  else {
    pp->rb = false;
  }

  if (pp->bytes[0] & MOUSE_MB) { // Middle Button
    pp->mb = true;
  }
  else {
    pp->mb = false;
  }
  return 0;
}

int write_mousecmd(uint8_t cmd) {
  uint8_t response;

  kbc_write(CTRL_REG, KBC_WRITE_MOUSE_BYTE);
  kbc_write(IN_BUF, cmd);
  tickdelay(micros_to_ticks(DELAY_US));
  kbc_read_out_buf(&response);

  // Checking if invalid byte
  if (response != ACK) {
    // repeat process if invalid
    kbc_write(CTRL_REG, KBC_WRITE_MOUSE_BYTE);
    kbc_write(IN_BUF, cmd);
    tickdelay(micros_to_ticks(DELAY_US));
    kbc_read_out_buf(&response);
    // Checking if second consecutive invalid byte
    if (response == ERROR)
      return 1;
  }

  return 0;
}

struct mouse_ev evt;

struct mouse_ev *mouse_event_detect(struct packet *pp) {

  //if false, wasn t press
  static int lrb = 0, llb = 0;

  evt.delta_x = pp->delta_x;
  evt.delta_y = pp->delta_y;

  //Left button pressed, when all buttons were released
  if (pp->rb && !lrb && !pp->lb && !pp->mb) { //check if rb is press and wasn t before
    evt.type = RB_PRESSED;
    lrb = 1; //warning to now if the event happen or just continue
  }

  //Left button released, when only left button was pressed
  else if (!pp->rb && lrb && !pp->lb && !pp->mb) { //check if rb isnt press and was before
    evt.type = RB_RELEASED;
    lrb = 0; //warning to now if the event happen or just continue
  }

  //Right button pressed, when all buttons were released.
  else if (pp->lb && !llb && !pp->rb && !pp->mb) { //check if lb is press and was before
    evt.type = LB_PRESSED;
    llb = 1;
  }

  //Right button released, when only left button was pressed
  else if (!pp->lb && llb && !pp->rb && !pp->mb) { //check if lb isnt press and was before
    evt.type = LB_RELEASED;
    llb = 0;
  }

  //Mouse movement event
  else if (evt.delta_x || evt.delta_y) {
    evt.type = MOUSE_MOV; //there was movement of th mouse
  }

  //Any other mouse buttons event
  else {
    evt.type = BUTTON_EV; //middle bottom pressed
                          //mb = 1;
  }
  return &evt;
}

void gest_handler(struct packet *pp, uint8_t x_len, uint8_t tolerance) {
  extern bool end;            /*flag to end the main loop */
  if (mouse_parse_packet(pp)) /*receve the status of mouse */
    return;
  struct mouse_ev *event = mouse_event_detect(pp);
  static enum state_t state = INIT; // initial state; keep state
  static int16_t lengthx, lengthy;  //length of the moviment

  switch (state) {

    case INIT:     //initial state
      lengthx = 0; //movemnt in x
      lengthy = 0; //movemnt in y

      //if LB and only LB is press, go to next state
      if (event->type == LB_PRESSED) {
        state = DRAW;
      }
      break;

    case DRAW: //2 state

      //if other bottons else then LB are press
      if (event->type == RB_PRESSED || event->type == BUTTON_EV) {
        state = INIT; //then go to init
      }

      else if (event->type == LB_RELEASED) {
        //if the length of x is greater then x_len and slope greater than 1
        if ((lengthx >= x_len) && (abs(lengthy / lengthx) > 1)) {
          state = DONELINE; //if the line
        }
        else {
          //if left bottom release with conditions true go back to initial state
          state = INIT;
        }
      }

      else if (event->type == MOUSE_MOV) {

        //if lin of movement is positive or between the tolerence
        if ((event->delta_x > 0 && event->delta_y > 0) || (abs(event->delta_x) < tolerance && abs(event->delta_y) < tolerance)) {
          lengthx += event->delta_x; //add x moviment
          lengthy += event->delta_y; //add y moviment
        }
        else {
          state = INIT;
        }
      }
      break;

    case DONELINE: //3 state
      lengthx = 0;
      lengthy = 0;

      //if other bottons else then RB are press
      if (event->type == LB_PRESSED || event->type == BUTTON_EV)
        state = INIT; //then go to init

      else if (event->type == RB_PRESSED) {
        state = DRAW1;
      }
      break;

    case DRAW1: //4 state

      if (event->type == RB_RELEASED) {
        //if length is greater ou equal and scope is greater than 1
        if (lengthx >= x_len && abs(lengthy / lengthx) > 1) {
          state = INIT; //final state
          end = true;   //loop can end in lab4
        }
        else {
          state = INIT; //moviment fail
        }
      }

      //if negative line is begin draw,stay in this state
      else if (event->type == MOUSE_MOV) {
        //if lin of movement is negative or between the tolerence
        if ((event->delta_x > 0 && event->delta_y < 0) || (abs(event->delta_x) < tolerance && abs(event->delta_y) < tolerance)) {
          lengthy += event->delta_y;
          lengthx += event->delta_x;
        } /* stay in this state */
        else {
          state = INIT;
        }
      }
      break;
  }
}

int length_x = 0, length_y = 0;
bool reading_first_line = false;
bool reading_second_line = false;
bool first_line_complete = false;
enum state_t state = INIT;

void(reset_line_data)() //Resets the lengths measured
{
  length_x = 0;
  length_y = 0;
  return;
}

bool(first_line_completed)(uint8_t x_len, uint8_t tolerance, struct packet *pp) //Checks if the first line was completed
{
  //Returns true if the line fulfills the minimum x length, the slope is greater than 1 and if the mouse movement in this packet wasn't bigger than the tolerance
  return (length_x >= x_len && length_y > length_x && abs(pp->delta_x) <= tolerance && abs(pp->delta_y) <= tolerance);
}

bool(second_line_completed)(uint8_t x_len, uint8_t tolerance, struct packet *pp) //Checks if the second line was completed
{
  //Returns true if the line fulfills the minimum x length, the slope absolute value is greater than 1 and if the mouse movement in this packet wasn't bigger than the tolerance
  return (length_x >= x_len && abs(length_y) > length_x && abs(pp->delta_x) <= tolerance && abs(pp->delta_y) <= tolerance);
}

void(first_line_handler)(int negative_tolerance, struct packet *pp) //Handles packets for the first line
{
  if (pp->delta_x >= negative_tolerance && pp->delta_y >= negative_tolerance) { //If the movement is within the tolerance increments the lengths measured
    length_x += pp->delta_x;
    length_y += pp->delta_y;
  }
  else
    reset_line_data();
}

void(second_line_handler)(int x_tol, uint8_t y_tol, struct packet *pp) //Handles packets for the first line
{
  if (pp->delta_x >= x_tol && pp->delta_y <= y_tol) { //If the movement is within the tolerance increments the lengths measured
    length_x += pp->delta_x;
    length_y += pp->delta_y;
  }
  else {
    reset_line_data();
    reading_second_line = false; //In order to reset the drawing to the beginning
  }
}

void(event_detector)(struct packet *pp, ev_type_t *evt) //Reads the events of the mouse
{
  bool left_button_pressed = (pp->lb && !pp->rb && !pp->mb);  //If only the left button is pressed
  bool right_button_pressed = (pp->rb && !pp->lb && !pp->mb); //If only the rigth button is pressed
  bool no_button_pressed = (!pp->rb && !pp->lb && !pp->mb);   //If no button is pressed

  if (left_button_pressed) { //Event: Left buton pressed
    *evt = LDOWN;
    if (!reading_first_line)
      reading_first_line = true; //Starts reading the first line
  }
  else if (no_button_pressed && reading_first_line) { //Event: Left button released
    *evt = LUP;
    reading_first_line = false; //Finishes reading the first line
  }
  else if (right_button_pressed) { //Event: Right button pressed
    *evt = RDOWN;
    if (!reading_second_line && first_line_complete)
      reading_second_line = true; //Starts reading the second line
  }
  else if (no_button_pressed && reading_second_line) { //Event: Right button released
    *evt = RUP;
    reading_second_line = false; //Finishes reading the second line
  }
  else if (!no_button_pressed)
    *evt = WRONGKEY; //Event: middle key is pressed, or if more than 1 key are pressed

  return;
}

bool(event_handler)(ev_type_t evt, struct packet *pp, uint8_t x_len, int negative_tolerance, uint8_t tolerance) {
  switch (state) {
    case INIT:
      reset_line_data();
      if (evt == LDOWN) //If the left button is pressed, starts the first line
        state = DRAW;
      break;
    case DRAW:
      if (evt == LDOWN) //While the left button is pressed, keeps drawing the line
        first_line_handler(negative_tolerance, pp);
      else if (evt == LUP) {                              //If the left button is lifted, ends the first line
        if (first_line_completed(x_len, tolerance, pp)) { //Checks if the first line is valid
          state = DONELINE;                               //Goes to the vertex
          first_line_complete = true;
        }
      }
      else //Else resets the drawing
        state = INIT;
      break;
    case DONELINE:
      reset_line_data();       //Resets the lengths
      if (evt == RDOWN)        //If the rigth key is pressed
        state = DRAW1;         //Starts drawing the 2 line
      else if (evt == LDOWN) { //If the left key is pressed
        state = DRAW;         //starts drawing the second line
        reset_line_data();     //Resets the lengths
      }
      else if (evt == WRONGKEY) //If the middle key is pressed, or if more than 1 keys are pressed
        state = INIT;           //Resets the drawing
      break;
    case DRAW1:
      if (evt == RDOWN) //If the rigth key is pressed
        second_line_handler(negative_tolerance, tolerance, pp);
      else if (evt == RUP) {                             //If the rigth key is lifted
        if (second_line_completed(x_len, tolerance, pp)) //Checks if the second line is valid
          return false;
      }
      else if (evt == LDOWN) { //If the left key is pressed
        state = DRAW;         //starts drawing the second line
        reset_line_data();
      }
      else //Else resets the drawing
        state = INIT;
      break;
    default:
      break;
  }

  return true;
}
