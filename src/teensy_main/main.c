#include <stdint.h>
#include "mk20dx128.h"

extern void delay(uint32_t);

/* port control, chapter 11 */
/* ports have 32 pins that can be configured */
/* for interrupt or dma trigger, logic levels */

/* http://www.pjrc.com/teensy/pinout.html */
/* http://www.pjrc.com/teensy/schematic.html */
/* http://forum.pjrc.com/threads/23431-Teensy-3-using-IO-pins */
/* led is on ptc5, ie. pin13 */

static inline void pin_set_gpio_mode(void)
{
  /* interrupt or dma disabled */
  /* not locked */
  /* mux set to gpio mode */
  /* high drive strength */
  /* open drain disabled */
  /* passive filter disabled */
  /* slow slew rate enabled */
  /* internal pullup disabled */

  PORTC_PCR5 = (1 << 8) | (1 << 6) | (1 << 2);

  /* enable PORTC pin 5 write */
  /* PORTC_GPCLR = 1 << (16 + 5); */

  GPIOC_PDDR |= 1 << 5;
}

static inline void pin_set_gpio_val(unsigned int x)
{
  /* warning: GPIOx_PyOR already dereferenced */
  if (x) GPIOC_PSOR = 1 << 5;
  else GPIOC_PCOR = 1 << 5;
}

/* masl api */

static void masl_init(void)
{
  PORTB_PCR17 = (1 << 8) | (1 << 6) | (1 << 2);
  GPIOB_PDDR |= 1 << 17;
}

static inline void masl_notify_master(void)
{
  /* toggle the output register */
  GPIOB_PTOR = 1 << 17;
}

int main(void)
{
  pin_set_gpio_mode();

  masl_init();

  while (1)
  {
    pin_set_gpio_val(1);
    masl_notify_master();
    delay(1000);

    pin_set_gpio_val(0);
    masl_notify_master();
    delay(1000);
  }

  return 0;
}
