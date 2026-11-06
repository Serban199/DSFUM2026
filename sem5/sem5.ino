#include <avr/io.h>
#include <avr/interrupt.h>
#include "sunet.h"

#define PAUZA 255

#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

const uint8_t ordine[] PROGMEM = {
  0, 1, 1, 2,
  0, 1, 1, 2,
  3, PAUZA, 4, 7,
  5, 5, 6, 7,
  5, 5, 6, 7
};

volatile uint8_t esantion = 128;

ISR(TIMER2_OVF_vect) {
  static uint8_t acumulator = 0;
  uint8_t vechi = acumulator;
  acumulator += esantion;
  if (acumulator < vechi) PORTD |= _BV(PORTD4);
  else PORTD &= ~_BV(PORTD4);
}

ISR(TIMER1_COMPA_vect, ISR_NOBLOCK) {
  static uint8_t pasMasura = 0;
  static uint16_t pozitie = 0;
  static int16_t pred = 0;
  static int8_t indexPas = INDEX_START;

  uint8_t masura = pgm_read_byte(&ordine[pasMasura]);

  if (masura == PAUZA) {
    esantion = 128;
  } else {
    uint16_t bitCurent = pozitie * 3;
    uint16_t adresa = masura * OCTETI_MASURA + (bitCurent >> 3);
    uint16_t cuvant = (pgm_read_byte(&sunet[adresa]) << 8) | pgm_read_byte(&sunet[adresa + 1]);
    uint8_t cod = (cuvant >> (13 - (bitCurent & 7))) & 7;

    uint16_t pas = pgm_read_word(&pasi[indexPas]);
    uint16_t dif = pas >> 2;
    if (cod & 1) dif += pas >> 1;
    if (cod & 2) dif += pas;

    int32_t nou = (int32_t)pred + ((cod & 4) ? -(int32_t)dif : (int32_t)dif);
    pred = constrain(nou, -32768L, 32767L);

    indexPas = constrain(indexPas + (int8_t)pgm_read_byte(&schimbIndex[cod & 3]), 0, NR_PASI - 1);

    esantion = (pred >> 8) + 128;
  }

  if (++pozitie == ESANTIOANE_MASURA) {
    pozitie = 0;
    pred = 0;
    indexPas = INDEX_START;
    if (++pasMasura == sizeof(ordine)) pasMasura = 0;
  }
}

int main(void) {
  DDRD |= _BV(DDD4) | _BV(DDD7);
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);

  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = F_CPU / 3600 - 1;
  TIMSK1 = _BV(OCIE1A);

  sei();
  for (;;) {}
}