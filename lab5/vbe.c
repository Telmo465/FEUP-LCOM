#include "vbe.h"
#include <lcom/lcf.h>

int vbe_get_info_mode(unsigned short mode, vbe_mode_info_t *vmi_p) {
  mmap_t mm;
  struct reg86 reg86;
  if (vg_init(true) == NULL)
    vg_init(true);
  lm_alloc(sizeof(vmi_p), &mm);

  memset(&reg86, 0, sizeof(reg86));    /* zero the structure */
  reg86.ah = 0x4F;                     /* invoking VBE function */
  reg86.intno = 0x10;               /* BIOS video services */
  reg86.cx = mode;
  reg86.al = 0x01;                  /* Return VBE Mode Information*/
  reg86.di = PB2OFF(mm.phys);
  reg86.es = PB2BASE(mm.phys);

  //temporarily switches from 32-bit protected mode to 16-bit real-mode to access the BIOS calls
  if (sys_int86(&reg86) != OK) { // if Operation fail
    lm_free(&mm);
    printf("vg_init(): sys_int86() failed \n");
    return 1;
  }

  *vmi_p = *(vbe_mode_info_t *) mm.virt;

  if (lm_free(&mm) == 0)
    return 1;

  return 0;
}
