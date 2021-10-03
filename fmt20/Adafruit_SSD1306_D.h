/**
 * Purpose:
 * Adafruit_SSD1306_D inherits from the standard Adafruit_SSD1306D
 * and add methods: 
 *  display_d()
 *    This version allows a void func(void) pointer to be passed. The purpose of this is to allow the encoder position
 *    to be updated after every line update on the display. This was to prevent long updates of the screen (35ms) from causing misse
 *    wrapping of the encoder position.
 * and overloads:
 *  begin()
 */
#ifndef _ADAFRUIT_SSD1306_D_H_
#define _ADAFRUIT_SSD1306_D_H_
  #include <Arduino.h>
  #include <Adafruit_SSD1306.h>

typedef void (* dfunc_t)(void);
  
  class Adafruit_SSD1306_D : public Adafruit_SSD1306{
  	public:
  		Adafruit_SSD1306_D(uint8_t w, uint8_t h, TwoWire *twi=&Wire, int8_t rst_pin=-1,uint32_t clkDuring=400000UL, uint32_t clkAfter=100000UL);
      /**
       * Override of the display function
       */
      void display(dfunc_t dfunc);
      void display(void);

  };
#endif
