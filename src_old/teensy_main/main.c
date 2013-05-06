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


/* dspi module, chapter 43 */

static inline void dspi_halt(void)
{
  SPI0_MCR |= SPI_MCR_HALT;
}

static inline void dspi_start(void)
{
  SPI0_MCR &= ~SPI_MCR_HALT;
}

static inline unsigned int dspi_is_running(void)
{
#ifndef SPI_SR_TXRXS
#define SPI_SR_TXRXS ((uint32_t)0x40000000)
#endif
  return SPI0_SR & SPI_SR_TXRXS;
}

static void dspi_setup_slave(void)
{
  /* configure inout ports */
  /* chapters 10, 11, 12 */
  /* PTD0, SPI0_CS0, ALT2 */
  PORTD_PCR0 = 2 << 8;
  /* PTD1, SPI0_SCK, ALT2 */
  PORTD_PCR1 = 2 << 8;
  /* PTD2, SPI0_SOUT, ALT2 */
  PORTD_PCR2 = 2 << 8;
  /* PTD3, SPI0_SIN, ALT2 */
  PORTD_PCR3 = 2 << 8;

  /* enable module clocking */
  if ((SIM_SCGC6 & SIM_SCGC6_SPI0) == 0) SIM_SCGC6 |= SIM_SCGC6_SPI0;

  /* stop the dspi module */
  dspi_halt();
  while (dspi_is_running()) ;

  /* spi0_mcr: dspi module configuration register */
  SPI0_MCR = 0;

  /* spi0_ctar0: framing and clocking setup */
  /* 8 bits wide frames */
  /* clock polarity is inactive low */
  /* sampled at leading edge, changed at following edge */
  SPI0_CTAR0 = SPI_CTAR_FMSZ(8 - 1);

  dspi_start();
}

static void dspi_push_data(uint8_t* s, uint32_t n)
{
  static volatile uint8_t* spio0_pushr = (volatile uint8_t*)0x4002c034;
  for (; n; --n, ++s) *spio0_pushr = *s;
}

static unsigned int dspi_tx_counter(void)
{
  return (SPI0_SR >> 12) & 0xf;
}

static unsigned int dspi_is_empty(void)
{
  return dspi_tx_counter() == 0;
}


/* main */

int main(void)
{
  uint8_t buf[4];
  uint8_t i;
  uint32_t x = 0;
  uint32_t n = 0;

  pin_set_gpio_mode();

  masl_init();

  dspi_setup_slave();

  while (1)
  {
    /* blink the led */
    pin_set_gpio_val(x & 1);
    delay(500);
    x ^= 1;

    if (dspi_is_empty())
    {
      for (i = 0; i < sizeof(buf); ++i) buf[i] = (uint8_t)(n + i);
      ++n;
      dspi_push_data(buf, sizeof(buf));
      masl_notify_master();
    }
  }

  return 0;
}
