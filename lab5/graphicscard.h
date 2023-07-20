#pragma once

#include <stdint.h>
#include "vbe.h"

void *(vg_init)(uint16_t mode);

unsigned get_hres();

unsigned get_vres();

int vg_pixel(uint16_t x, uint16_t y, uint32_t color);

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

uint32_t(IndexedModeColor)(uint32_t first, unsigned row, unsigned col, uint8_t no_rectangles, uint8_t step);

uint32_t(DirectModelColor)(uint32_t first, unsigned row, unsigned col, uint8_t no_rectangles, uint8_t step, uint16_t mode);

int vg_draw_pattern(uint8_t no_rectangles, uint32_t first, uint8_t step);
