// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

#include "i8042.h"
#include "keyboard.h"
#include "graphicscard.h"


extern bool status_error;
#define DirectColor 0x06
#define PackedPixel 0x04

// Any header files included below this line should have been created by you

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab5/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(video_test_init)(uint16_t mode, uint8_t delay) {
  if (vg_init(mode) == NULL)
    return 1;
  sleep(delay);
  vg_exit();
  return 0;
}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  uint8_t size, bytes[2];
  bool make = false, pass = false;
  int r, ipc_status;
  message msg;
  uint8_t k_irq;
  extern uint8_t scancode;

  if (kbc_subscribe_int(&k_irq) == 1)
    return 1;
  if (vg_init(mode) == NULL) return 1;

  vg_draw_rectangle(x, y, width, height, color);

  while (scancode != 0x81) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("ERROR: driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & k_irq) {
            kbc_ih();
			if (scancode == 0xE0) {
              pass = true;
              size = 2;
              bytes[0] = scancode;
            }
            else if (!pass) {
              pass = false;
              bytes[1] = scancode;
			  if (scancode & BIT(7)) {
                make = false;
              }
              else {
                make = true;
              }
            }
            else if (scancode == 0xE0) {
              pass = true;
              size = 2;
              bytes[0] = scancode;
			}
            else {
			  pass = false;
              size = 1;
              bytes[0] = scancode;
            }
            if (pass) {
              pass = false;
              bytes[1] = scancode;
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

  vg_exit();

  if (kbc_unsubscribe_int() == 1) return 1;
  return 0;
}

int(video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {
  uint8_t b;
  uint32_t irq_set;
  vbe_mode_info_t info;
  vbe_get_mode_info(mode, &info);

  if (kbc_subscribe_int(&b)) {
    vg_exit();
    return 1;
  }
  irq_set = (uint32_t) b;

  void *address = vg_init(mode);
  if (address == NULL) return 1;

  uint16_t width = info.XResolution / no_rectangles;
  uint16_t height = info.YResolution / no_rectangles;

  vbe_mode_info_t info1;
  vbe_get_mode_info(mode, &info1);
  uint8_t bits_per_pixel = info1.BitsPerPixel;

  if (bits_per_pixel == 8) { // mode 105
    for (unsigned i = 0; i < no_rectangles; i++) {
      for (unsigned j = 0; j < no_rectangles; j++) {
        uint32_t color = IndexedModeColor(first, i, j, no_rectangles, step);
        vg_draw_rectangle(j * width, i * height, width, height, color);
      }
    }
  }
  else { //mode 115
    //...
    for (unsigned i = 0; i < no_rectangles; i++) {
      for (unsigned j = 0; j < no_rectangles; j++) {
        uint32_t color = DirectModelColor(first, i, j, no_rectangles, step, mode);
        vg_draw_rectangle(j * width, i * height, width, height, color);
      }
    }
  }

  extern uint8_t scancode;
  int r, ipc_status;
  message msg;

  while (scancode != 0x81) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) { //received notification
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            kbc_ih();
            break;
            default: break;
          }
      }
    }
  }

  if (vg_exit())
    return 1;

  if (kbc_unsubscribe_int())
    return 1;
  return 0;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  uint32_t irq_set;
  uint8_t b;
  extern uint8_t scancode;
  int r, ipc_status;
  message msg;
  if (kbc_subscribe_int(&b)) {
    vg_exit();
    return 1;
  }
  irq_set = (uint32_t) b;

  uint16_t mode = 0X105;
  void *address = vg_init(mode);
  if (address == NULL) return 1;
  xpm_image_t img;
  uint8_t *sprite = xpm_load(xpm, XPM_INDEXED, &img);
  if (sprite == NULL)
    return 1;

  uint16_t width = img.width;
  uint16_t height = img.height;
  uint8_t *init = address;

  for (unsigned i = 0; i < height; i++) {
    if ((y + i) >= get_vres())
      break;
    for (unsigned j = 0; j < width; j++) {
      if ((unsigned int) (x + j) >= get_hres())
        break;
      uint8_t *p = init + ((y + i) * get_hres() + x + j);
      *p = sprite[i * width + j]; //set the color of the pixel
    }
  }

  while (scancode != 0x81) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) { //received notification
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            kbc_ih();
            if (scancode == 0)
              break;
            break;
            default: break;
          }
      }
    }
  }

  if (vg_exit()) return 1;

  if (kbc_unsubscribe_int())
    return 1;
  return 0;
}	

int(video_test_move)(xpm_map_t xpm, uint16_t xi, uint16_t yi, uint16_t xf, uint16_t yf, int16_t speed, uint8_t fr_rate) {
  /* To be completed */
  printf("%s(%8p, %u, %u, %u, %u, %d, %u): under construction\n",
         __func__, xpm, xi, yi, xf, yf, speed, fr_rate);

  return 1;
}

int(video_test_controller)() {

  return 0;
}
