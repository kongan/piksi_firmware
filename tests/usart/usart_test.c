/*
 * Copright (C) 2011-2014 Swift Navigation Inc <info@swift-nav.com>
 * Contact: Colin Beighley <colin@swift-nav.com>
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * Based on usart_test.c from libopencm3.
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/f4/nvic.h>

#include "main.h"
#include "board/leds.h"
#include "error.h"
#include "peripherals/usart.h"

void clock_setup(void)
{
  /* Clock the USARTs. */
  RCC_APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_USART6EN;
  RCC_APB1ENR |= RCC_APB1ENR_USART3EN;

  /* GPIO pins corresponding to the USART. */
  RCC_AHB1ENR |= RCC_AHB1ENR_IOPAEN | RCC_AHB1ENR_IOPCEN;
}

void gpio_setup(void)
{
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
  gpio_set_af(GPIOC, GPIO_AF8, GPIO6 | GPIO7);

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
  gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10 | GPIO11);
  gpio_set_af(GPIOC, GPIO_AF7, GPIO10 | GPIO11);
}

int main(void)
{
  clock_setup();
  gpio_setup();
  usart_set_parameters(USART6, USART_DEFAULT_BAUD_FTDI);
  usart_set_parameters(USART1, USART_DEFAULT_BAUD_TTL);
  usart_set_parameters(USART3, USART_DEFAULT_BAUD_TTL);

  led_setup();

  led_on(LED_RED);
  led_off(LED_GREEN);

  /* Blink the LED on the board with every transmitted byte. */
  char c = 0;
  while (1) {
    led_toggle(LED_RED);
    led_toggle(LED_GREEN);
    for (int i = 0; i < 100000; i++) /* Wait a bit. */
      __asm__("NOP");
    DO_EVERY(26,
      for (int i = 0; i < 100000; i++) /* Wait a bit. */
        __asm__("NOP");
      USART6_DR = '\n';
      USART1_DR = '\n';
      USART3_DR = '\n';
      for (int i = 0; i < 100000; i++) /* Wait a bit. */
        __asm__("NOP");
      USART6_DR = '\r';
      USART1_DR = '\r';
      USART3_DR = '\r';
    );
    USART6_DR = c + 'A';
    USART1_DR = c + 'A';
    USART3_DR = c + 'A';
    c = (c + 1) % 26;
  }

  return 0;
}
