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
      void Adafruit_SSD1306_D::display_d(void);
      void Adafruit_SSD1306_D::display_d(dfunc_t dfunc);
      boolean      begin(uint8_t switchvcc=SSD1306_SWITCHCAPVCC,
                 uint8_t i2caddr=0, boolean reset=true,
                 boolean periphBegin=true);
    private:
      inline void SPIwrite_d(uint8_t d);
      SPIClass    *spi_d;
      TwoWire     *wire_d;
      int8_t       i2caddr_d;
#if ARDUINO >= 157
      uint32_t     wireClk_d;    // Wire speed for SSD1306 transfers
      uint32_t     restoreClk_d; // Wire speed following SSD1306 transfers
#endif
#if defined(SPI_HAS_TRANSACTION)
  SPISettings  spiSettings_d;
#endif

  };
#endif
