#include "Adafruit_SSD1306_D.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#define pgm_read_byte(addr)                                                    \
  (*(const unsigned char *)(addr)) ///< PROGMEM workaround for non-AVR
#endif

#if !defined(__ARM_ARCH) && !defined(ENERGIA) && !defined(ESP8266) &&          \
    !defined(ESP32) && !defined(__arc__)
#include <util/delay.h>
#endif

#include "Adafruit_SSD1306.h"
#include "splash.h"
#include <Adafruit_GFX.h>

// SOME DEFINES AND STATIC VARIABLES USED INTERNALLY -----------------------

#if defined(I2C_BUFFER_LENGTH)
#define WIRE_MAX min(256, I2C_BUFFER_LENGTH) ///< Particle or similar Wire lib
#elif defined(BUFFER_LENGTH)
#define WIRE_MAX min(256, BUFFER_LENGTH) ///< AVR or similar Wire lib
#elif defined(SERIAL_BUFFER_SIZE)
#define WIRE_MAX                                                               \
  min(255, SERIAL_BUFFER_SIZE - 1) ///< Newer Wire uses RingBuffer
#else
#define WIRE_MAX 32 ///< Use common Arduino core default
#endif

#define ssd1306_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

#if ARDUINO >= 100
#define WIRE_WRITE wire->write ///< Wire write function in recent Arduino lib
#else
#define WIRE_WRITE wire->send ///< Wire write function in older Arduino lib
#endif

#ifdef HAVE_PORTREG
#define SSD1306_SELECT *csPort &= ~csPinMask;       ///< Device select
#define SSD1306_DESELECT *csPort |= csPinMask;      ///< Device deselect
#define SSD1306_MODE_COMMAND *dcPort &= ~dcPinMask; ///< Command mode
#define SSD1306_MODE_DATA *dcPort |= dcPinMask;     ///< Data mode
#else
#define SSD1306_SELECT digitalWrite(csPin, LOW);       ///< Device select
#define SSD1306_DESELECT digitalWrite(csPin, HIGH);    ///< Device deselect
#define SSD1306_MODE_COMMAND digitalWrite(dcPin, LOW); ///< Command mode
#define SSD1306_MODE_DATA digitalWrite(dcPin, HIGH);   ///< Data mode
#endif

#if (ARDUINO >= 157) && !defined(ARDUINO_STM32_FEATHER)
#define SETWIRECLOCK wire->setClock(wireClk)    ///< Set before I2C transfer
#define RESWIRECLOCK wire->setClock(restoreClk) ///< Restore after I2C xfer
#else // setClock() is not present in older Arduino Wire lib (or WICED)
#define SETWIRECLOCK ///< Dummy stand-in define
#define RESWIRECLOCK ///< keeps compiler happy
#endif

#if defined(SPI_HAS_TRANSACTION)
#define SPI_TRANSACTION_START spi->beginTransaction(spiSettings) ///< Pre-SPI
#define SPI_TRANSACTION_END spi->endTransaction()                ///< Post-SPI
#else // SPI transactions likewise not present in older Arduino SPI lib
#define SPI_TRANSACTION_START ///< Dummy stand-in define
#define SPI_TRANSACTION_END   ///< keeps compiler happy
#endif

// The definition of 'transaction' is broadened a bit in the context of
// this library -- referring not just to SPI transactions (if supported
// in the version of the SPI library being used), but also chip select
// (if SPI is being used, whether hardware or soft), and also to the
// beginning and end of I2C transfers (the Wire clock may be sped up before
// issuing data to the display, then restored to the default rate afterward
// so other I2C device types still work).  All of these are encapsulated
// in the TRANSACTION_* macros.

// Check first if Wire, then hardware SPI, then soft SPI:
#define TRANSACTION_START                                                      \
  if (wire) {                                                                  \
    SETWIRECLOCK;                                                              \
  } else {                                                                     \
    if (spi) {                                                                 \
      SPI_TRANSACTION_START;                                                   \
    }                                                                          \
    SSD1306_SELECT;                                                            \
  } ///< Wire, SPI or bitbang transfer setup
#define TRANSACTION_END                                                        \
  if (wire) {                                                                  \
    RESWIRECLOCK;                                                              \
  } else {                                                                     \
    SSD1306_DESELECT;                                                          \
    if (spi) {                                                                 \
      SPI_TRANSACTION_END;                                                     \
    }                                                                          \
  } ///< Wire, SPI or bitbang transfer end

/**
 * 
 */
Adafruit_SSD1306_D::Adafruit_SSD1306_D(uint8_t w, uint8_t h, TwoWire *twi=&Wire, int8_t rst_pin=-1,uint32_t clkDuring=400000UL, uint32_t clkAfter=100000UL) :
      Adafruit_SSD1306(w, h, twi,rst_pin,clkDuring,clkAfter)
{

}

/**
 * Direct copy from Adafruit_SSD1306
 */
// LOW-LEVEL UTILS ---------------------------------------------------------

// Issue single byte out SPI, either soft or hardware as appropriate.
// SPI transaction/selection must be performed in calling function.
inline void Adafruit_SSD1306::SPIwrite(uint8_t d) {
  if (spi) {
    (void)spi->transfer(d);
  } else {
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
#ifdef HAVE_PORTREG
      if (d & bit)
        *mosiPort |= mosiPinMask;
      else
        *mosiPort &= ~mosiPinMask;
      *clkPort |= clkPinMask;  // Clock high
      *clkPort &= ~clkPinMask; // Clock low
#else
      digitalWrite(mosiPin, d & bit);
      digitalWrite(clkPin, HIGH);
      digitalWrite(clkPin, LOW);
#endif
    }
  }
}

void Adafruit_SSD1306_D::display(void){
  Adafruit_SSD1306::display();
}

void Adafruit_SSD1306_D::display(dfunc_t dfunc){
    TRANSACTION_START
  static const uint8_t PROGMEM dlist1[] = {
    SSD1306_PAGEADDR,
    0,                         // Page start address
    0xFF,                      // Page end (not really, but works here)
    SSD1306_COLUMNADDR,
    0 };                       // Column start address
  //ssd1306_commandList(dlist1, sizeof(dlist1));
  //ssd1306_command1(WIDTH - 1); // Column end address

#if defined(ESP8266)
  // ESP8266 needs a periodic yield() call to avoid watchdog reset.
  // With the limited size of SSD1306 displays, and the fast bitrate
  // being used (1 MHz or more), I think one yield() immediately before
  // a screen write and one immediately after should cover it.  But if
  // not, if this becomes a problem, yields() might be added in the
  // 32-byte transfer condition below.
  yield();
#endif
  uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
  uint8_t *ptr   = buffer;
  if(wire) { // I2C
    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x40);
    uint8_t bytesOut = 1;
    while(count--) {
      if(bytesOut >= WIRE_MAX) {
        wire->endTransmission();
        wire->beginTransmission(i2caddr);
        WIRE_WRITE((uint8_t)0x40);
        bytesOut = 1;
      }
      WIRE_WRITE(*ptr++);
      bytesOut++;
      //every line give the dfunc a chance to run
      if(dfunc !=NULL){
        if(!(count % WIDTH)){
          wire->endTransmission();
          TRANSACTION_END
            dfunc();
          TRANSACTION_START
          wire->beginTransmission(i2caddr);
          WIRE_WRITE((uint8_t)0x40);
          bytesOut = 1;
        }
      }
    }
    wire->endTransmission();
  } else { // SPI
    SSD1306_MODE_DATA
    while(count--) SPIwrite(*ptr++);
  }
  TRANSACTION_END
 }
