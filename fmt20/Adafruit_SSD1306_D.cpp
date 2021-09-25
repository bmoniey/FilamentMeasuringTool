#include "Adafruit_SSD1306_D.h"


#ifdef HAVE_PORTREG
// #define SSD1306_SELECT       *csPort &= ~csPinMask; ///< Device select
// #define SSD1306_DESELECT     *csPort |=  csPinMask; ///< Device deselect
// #define SSD1306_MODE_COMMAND *dcPort &= ~dcPinMask; ///< Command mode
// #define SSD1306_MODE_DATA    *dcPort |=  dcPinMask; ///< Data mode
#define SSD1306_SELECT       //*csPort &= ~csPinMask; ///< Device select
#define SSD1306_DESELECT     //*csPort |=  csPinMask; ///< Device deselect
#define SSD1306_MODE_COMMAND //*dcPort &= ~dcPinMask; ///< Command mode
#define SSD1306_MODE_DATA    //*dcPort |=  dcPinMask; ///< Data mode
#else
 #define SSD1306_SELECT       digitalWrite(csPin, LOW);  ///< Device select
 #define SSD1306_DESELECT     digitalWrite(csPin, HIGH); ///< Device deselect
 #define SSD1306_MODE_COMMAND digitalWrite(dcPin, LOW);  ///< Command mode
 #define SSD1306_MODE_DATA    digitalWrite(dcPin, HIGH); ///< Data mode
#endif

#if (ARDUINO >= 157) && !defined(ARDUINO_STM32_FEATHER)
 #define SETWIRECLOCK wire_d->setClock(wireClk_d)    ///< Set before I2C transfer
 #define RESWIRECLOCK wire_d->setClock(restoreClk_d) ///< Restore after I2C xfer
#else // setClock() is not present in older Arduino Wire lib (or WICED)
 #define SETWIRECLOCK ///< Dummy stand-in define
 #define RESWIRECLOCK ///< keeps compiler happy
#endif

#if defined(SPI_HAS_TRANSACTION)
 #define SPI_TRANSACTION_START spi_d->beginTransaction(spiSettings_d) ///< Pre-SPI
 #define SPI_TRANSACTION_END   spi_d->endTransaction()              ///< Post-SPI
#else // SPI transactions likewise not present in older Arduino SPI lib
 #define SPI_TRANSACTION_START ///< Dummy stand-in define
 #define SPI_TRANSACTION_END   ///< keeps compiler happy
#endif

// Check first if Wire, then hardware SPI, then soft SPI:
#define TRANSACTION_START   \
 if(wire_d) {                 \
   SETWIRECLOCK;            \
 } else {                   \
   if(spi_d) {                \
     SPI_TRANSACTION_START; \
   }                        \
   SSD1306_SELECT;          \
 } ///< Wire, SPI or bitbang transfer setup
#define TRANSACTION_END     \
 if(wire_d) {                 \
   RESWIRECLOCK;            \
 } else {                   \
   SSD1306_DESELECT;        \
   if(spi_d) {                \
     SPI_TRANSACTION_END;   \
   }                        \
 } ///< Wire, SPI or bitbang transfer end 

#if defined(BUFFER_LENGTH)
 #define WIRE_MAX BUFFER_LENGTH          ///< AVR or similar Wire lib
#elif defined(SERIAL_BUFFER_SIZE)
 #define WIRE_MAX (SERIAL_BUFFER_SIZE-1) ///< Newer Wire uses RingBuffer
#else
 #define WIRE_MAX 32                     ///< Use common Arduino core default
#endif

#define ssd1306_swap(a, b) \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation
  
#if ARDUINO >= 100
 #define WIRE_WRITE wire_d->write ///< Wire write function in recent Arduino lib
#else
 #define WIRE_WRITE wire_d->send  ///< Wire write function in older Arduino lib
#endif

Adafruit_SSD1306_D::Adafruit_SSD1306_D(uint8_t w, uint8_t h, TwoWire *twi=&Wire, int8_t rst_pin=-1,uint32_t clkDuring=400000UL, uint32_t clkAfter=100000UL) :
      spi_d(NULL),wire_d(twi ? twi : &Wire),wireClk_d(clkDuring), restoreClk_d(clkAfter),Adafruit_SSD1306(w, h, twi, rst_pin,clkDuring, clkAfter)
{

#ifdef SPI_HAS_TRANSACTION
  //spiSettings_d = SPISettings(bitrate, MSBFIRST, SPI_MODE0);
#endif
}

boolean Adafruit_SSD1306_D::begin(uint8_t switchvcc=SSD1306_SWITCHCAPVCC,
                 uint8_t i2caddr=0, boolean reset=true,
                 boolean periphBegin=true){
                    
                    i2caddr_d = i2caddr;
                    
                    return Adafruit_SSD1306::begin(switchvcc,i2caddr,reset,periphBegin);
                 }


/*!
    @brief  Push data currently in RAM to SSD1306 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void Adafruit_SSD1306_D::display_d(void) {
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
  uint8_t *ptr   = getBuffer();
  if(wire_d) { // I2C
    wire_d->beginTransmission(i2caddr_d);
    WIRE_WRITE((uint8_t)0x40);
    uint8_t bytesOut = 1;
    while(count--) {
      if(bytesOut >= WIRE_MAX) {
        wire_d->endTransmission();
        wire_d->beginTransmission(i2caddr_d);
        WIRE_WRITE((uint8_t)0x40);
        bytesOut = 1;
      }
      WIRE_WRITE(*ptr++);
      bytesOut++;
    }
    wire_d->endTransmission();
  } else { // SPI
    SSD1306_MODE_DATA
    while(count--) SPIwrite_d(*ptr++);
  }
  TRANSACTION_END
#if defined(ESP8266)
  yield();
#endif
}

// Issue single byte out SPI, either soft or hardware as appropriate.
// SPI transaction/selection must be performed in calling function.
inline void Adafruit_SSD1306_D::SPIwrite_d(uint8_t d) {
//  if(spi) {
//    (void)spi_d->transfer(d);
//  } else {
//    for(uint8_t bit = 0x80; bit; bit >>= 1) {
//#ifdef HAVE_PORTREG
//      if(d & bit) *mosiPort |=  mosiPinMask;
//      else        *mosiPort &= ~mosiPinMask;
//      *clkPort |=  clkPinMask; // Clock high
//      *clkPort &= ~clkPinMask; // Clock low
//#else
//      digitalWrite(mosiPin, d & bit);
//      digitalWrite(clkPin , HIGH);
//      digitalWrite(clkPin , LOW);
//#endif
//    }
//  }
}

void Adafruit_SSD1306_D::display_d(dfunc_t dfunc){
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
  uint8_t *ptr   = getBuffer();
  if(wire_d) { // I2C
    wire_d->beginTransmission(i2caddr_d);
    WIRE_WRITE((uint8_t)0x40);
    uint8_t bytesOut = 1;
    while(count--) {
      if(bytesOut >= WIRE_MAX) {
        wire_d->endTransmission();
        wire_d->beginTransmission(i2caddr_d);
        WIRE_WRITE((uint8_t)0x40);
        bytesOut = 1;
      }
      WIRE_WRITE(*ptr++);
      bytesOut++;
      //every line give the dfunc a chance to run
      if(dfunc !=NULL){
        if(!(count % WIDTH)){
          wire_d->endTransmission();
          TRANSACTION_END
            dfunc();
          TRANSACTION_START
          wire_d->beginTransmission(i2caddr_d);
          WIRE_WRITE((uint8_t)0x40);
          bytesOut = 1;
        }
      }
    }
    wire_d->endTransmission();
  } else { // SPI
    SSD1306_MODE_DATA
    while(count--) SPIwrite_d(*ptr++);
  }
  TRANSACTION_END
 }
