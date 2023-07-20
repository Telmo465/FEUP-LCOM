#include <lcom/lcf.h>

#include <stdint.h>

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  val &= 0x00FF;
  *lsb = val;
  return 0;
}
int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  val &= 0xFF00;
  val >>= 8;
  *msb = val;
  return 0;
}
int (util_sys_inb)(int port, uint8_t *value) {
  uint32_t new;
  sys_inb(port, &new);
  *value = new;
  return 0;
}
