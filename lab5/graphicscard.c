#include "graphicscard.h"
#include "vbe.h"
#include <lcom/lcf.h>
#include <math.h>

static char *address;          /* Process (virtual) address to which VRAM is mapped */
static unsigned h_res;           /* Horizontal resolution in pixels */
static unsigned v_res;           /* Vertical resolution in pixels */
static unsigned bits_per_pixel;  /* Number of VRAM bits per pixel */
//static unsigned bytes_per_pixel; /* Number of VRAM bytes per pixel */
vbe_mode_info_t vmi_p;	


void *(map_address)(uint16_t mode) {
  vbe_mode_info_t info;
  vbe_get_mode_info(mode, &info);
  v_res = info.YResolution;
  h_res = info.XResolution;
  bits_per_pixel = info.BitsPerPixel;
  struct minix_mem_range mr;
  unsigned int vram_size = h_res * v_res * ((bits_per_pixel + 7) / 8);
  unsigned int vram_base = info.PhysBasePtr;
  void *video_mem;
  int r;
  mr.mr_base = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;
  if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
  video_mem = vm_map_phys(SELF, (void *) mr.mr_base, vram_size);
  if (video_mem == MAP_FAILED)
    panic("couldn't map video memory");	
  return video_mem;
}	

int(GraphicsMode)(uint16_t mode) {
  struct minix_mem_range mr;
  struct reg86 r;
  mr.mr_limit = 1 << 20;
  mr.mr_base = 0;
  sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr);
  memset(&r, 0, sizeof(r));
  r.bx = 1 << 14 | mode;
  r.intno = 0x10;
  r.ax = 0x4F02;
  if (sys_int86(&r) != OK)
    return 1;
  return 0;
}


void *(vg_init)(uint16_t mode) {
  address = map_address(mode);
  if (address == NULL)
    return NULL;
  if (GraphicsMode(mode) == 1) {
    return NULL;
  }
  return address;
}

int vg_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= h_res || y >= v_res)
    return 1;
  unsigned int bytes_per_pixel = ((bits_per_pixel + 7) / 8);
  for (unsigned int j = 0; j < bytes_per_pixel; j++) {
    uint8_t col = (color >> bits_per_pixel * j);
    *(address + (y * h_res + x) * bytes_per_pixel + j) = col;
  }
  return 0;
}

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (int i = 0; i < len; i++) {
    if (vg_pixel(x + i, y, color) == 1)
      return 1;
  }
  return 0;
}

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (int i = 0; i < height; i++) {
    if (vg_draw_hline(x, y + i, width, color) == 1)
      return 1;
  }
  return 0;
}

unsigned get_vres() {
  return v_res;
}

unsigned get_hres() {
  return h_res;
}

//mode 105
uint32_t(IndexedModeColor)(uint32_t first, unsigned row, unsigned col, uint8_t no_rectangles, uint8_t step) {
  uint32_t color = (first + (row * no_rectangles + col) * step) % (1 << 8);
  return color;
}

//mode 115
uint32_t(DirectModelColor)(uint32_t first, unsigned row, unsigned col, uint8_t no_rectangles, uint8_t step, uint16_t mode) {
  uint32_t new_color;
  vbe_mode_info_t vbmit;
  vbe_get_mode_info(mode, &vbmit);

  //blue
  uint32_t blueM = ((1 << vbmit.BlueMaskSize) - 1);
  uint32_t blue = first;
  blue = (blue >> vbmit.BlueFieldPosition) & blueM;
  blue = (blue + (col + row) * step) % (1 << vbmit.BlueMaskSize);
  //green
  uint32_t greenM = ((1 << vbmit.GreenMaskSize) - 1);
  uint32_t green = first;
  green = (green >> vbmit.GreenFieldPosition) & greenM;
  green = (green + row * step) % (1 << vbmit.GreenMaskSize);
  //red
  uint32_t redM = ((1 << vbmit.RedMaskSize) - 1);
  uint32_t red = first;
  red = (red >> (vbmit.RedFieldPosition)) & redM;
  red = (red + col * step) % (1 << vbmit.RedMaskSize);
  new_color = ((red << vbmit.RedFieldPosition) | (green << vbmit.GreenFieldPosition) | blue);
  return new_color;
}

int vg_draw_pattern(uint8_t no_rectangles, uint32_t first, uint8_t step) {
  uint8_t height = v_res / no_rectangles;
  uint8_t width = h_res / no_rectangles;

  for (int i = 0; i <= no_rectangles; i++) {
    for (int j = 0; j <= no_rectangles; j++) {
      uint32_t index = (first + (i * no_rectangles + j) * step) % (1 << bits_per_pixel);
      vg_draw_rectangle(0 + (j * width), (i * height), width, height, index);
    }
  }

  return 0;
}
