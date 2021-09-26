/*
 
 FMT2.0
 F.ilament M.easuring T.ool 2.0
 Goals:
 Measure filament to nearest 1.0mm
 Use AS5600 encoder with 12bit,1024, resolution
 Read a button to reset the length counters to zero
 Read a button to change the units from mm,inche,meter,and feet
 Display data on an 128x64 OLED display
 -Length
 -Unit of measure

Notes:
This version is totally new version of hardware.
The filament is pulled through.The filamanet is pinched between a large 100mm wheel and
a 22x7x8mm bearing. It used an AS5600 magnetic encoder to read position.

calibration data with 100mm wheel
1000mm/raw counts
13398
13416
13446
13415
13413
13423
13427
13465
13444

 Written by Brian Moran 9/12/2021
 */
#include "Arduino.h" 
#include <SPI.h>
#include <Wire.h>
#include <AS5600.h>
#include <EEPROM.h>

//#include <FixedPoints.h>
//#include <FixedPointsCommon.h>
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define SERIAL SerialUSB
  #define SYS_VOL   3.3
#else
  #define SERIAL Serial
  #define SYS_VOL   5
#endif
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "Adafruit_SSD1306_D.h"

#define VERSION "FMT 2.1.0"

#define  SW1_RESET_PIN 5
#define  SW2_UNITS_PIN 6
#define  DEBUG_PIN 7
#define  REVOLUTION 4095
#define  WRAP_ZONE 128
#define  DISPLAY display_d
#define  EEMAGIC 0x80010674
#define  EEWRITE 0x01
#define  EEREAD 0x00
#define  HARDCAL_MM  (float)(0.07447433532*999.0/989.0)
#define  HARDCAL_IN  (float)(0.07447433532*999.0/989.0/25.4)
#define  CAL_MODE    0x1
#define  NORMAL_MODE 0x0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin) was 4
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306_D display(SCREEN_WIDTH, SCREEN_HEIGHT);

AMS_5600 ams5600;

char cstr[64];
uint8_t swrst;
uint8_t swunt;
uint8_t ledstate;
uint8_t encoder_state = 0;
uint8_t encoder_onEntry = 1;
uint8_t encoder_counter = 0;
uint8_t mode = NORMAL_MODE;

enum unit_t{mm,inch};
float cal_mm,cal_in;

typedef struct{
  uint32_t magic;
  float    cal;
}eestore_t;

typedef union{
  eestore_t es;
  uint8_t b[sizeof(eestore_t)];
}eestore_u;

eestore_u eestore;
int32_t angle_start = 0;
int32_t angle[2];
int32_t angle_abs;
int32_t angle_delta;
uint32_t timer = 0;
unit_t unit = mm;

void updateDisplay(void);
void doEncoder(void);
void do_switches(void);

void eestore_rw(uint8_t * buff, size_t len,uint8_t rw){
  size_t ii;
  if(rw == EEREAD){
    for(ii=0;ii<len;ii++){
      buff[ii] = EEPROM.read(ii);
    }
  }else{
    for(ii=0;ii<len;ii++){
      EEPROM.write(ii,buff[ii]);
    }
  }
}


void setup() {
  char sval[16];
  swrst = 0;
  swunt = 0;
  ledstate = 0;

  pinMode(SW1_RESET_PIN, INPUT_PULLUP);
  pinMode(SW2_UNITS_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DEBUG_PIN, OUTPUT);
  
  SERIAL.begin(115200);
  delay(100);
  
  SERIAL.println(F("Begining FMT:"));
  
  eestore_rw(eestore.b,sizeof(eestore),EEREAD);
  if(eestore.es.magic == EEMAGIC){
    SERIAL.println(F("eeload success!"));
  }else{
    SERIAL.println(F("initializing eestore!"));
    eestore.es.magic = EEMAGIC;
    eestore.es.cal = HARDCAL_MM;
    eestore_rw(eestore.b,sizeof(eestore),EEWRITE);
    dtostrf(eestore.es.cal, 6, 6, sval);
    sprintf(cstr,"%6s",sval);
    SERIAL.print(F("cal:"));
    SERIAL.println(cstr);
  }

    
  cal_mm = eestore.es.cal;
  cal_in = cal_mm / 25.4;

  SERIAL.print(F("cal_mm:"));
  dtostrf(cal_mm, 6, 6, sval);
  sprintf(cstr,"%6s",sval);
  SERIAL.println(cstr);
  SERIAL.print(F("cal_in:"));
  dtostrf(cal_in, 6, 6, sval);
  sprintf(cstr,"%6s",sval);
  SERIAL.println(cstr);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    SERIAL.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.DISPLAY();

  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);            // Start at top-left corner
  display.println(F(VERSION));
  display.DISPLAY();
  delay(1000);

  digitalWrite(DEBUG_PIN, LOW);

  Wire.begin();
  
  SERIAL.println(F(">>>>>>>>>>>>>>>>>>>>>>>>>>> "));
  
  if(ams5600.detectMagnet() == 0 ){
    while(1){
        if(ams5600.detectMagnet() == 1 ){
            SERIAL.print(F("Current Magnitude: "));
            SERIAL.println(ams5600.getMagnitude());
            break;
        }
        else{
            SERIAL.println(F("Can not detect magnet"));
        }
        delay(1000);
    }
  }
}

void loop() 
{
  // Here we go.
  doEncoder();
//  do_swrst();
//  do_swunt();
  do_switches();
  
  if(millis() > timer){
    
    updateDisplay();
    
    timer = millis() + 200;
    sprintf(cstr,"%d",angle_abs);
    Serial.println(cstr);
  }
}

void do_switches(void){
  swrst = digitalRead(SW1_RESET_PIN) == LOW ? (swrst++ > 254 ? 254 : swrst) : 0;
  swunt = digitalRead(SW2_UNITS_PIN) == LOW ? (swunt++ > 254 ? 254 : swunt) : 0;

  if(mode == NORMAL_MODE){
    if(swrst > 50 && swunt == 0){
      //reset the length calculation here
      SERIAL.println(F("Reset Length"));
      angle_abs=0;
      swrst = 0;
      swunt = 0;
    }else if(swrst == 0 && swunt > 50 ){
      //change the units heres
      SERIAL.println(F("change Unit"));
      unit = unit == mm ? inch : mm;
      swrst = 0;
      swunt = 0;
    }else if(swrst > 250 && swunt > 250){
      //toggle the mode
      mode =  CAL_MODE;
      swrst = 0;
      swunt = 0;
    }
  }else{
    //calibration mode
    if(swrst > 50 && swunt == 0){
      //reset the length calculation here
      SERIAL.println(F("Reset Length"));
      angle_abs=0;
      swrst = 0;
      swunt = 0;
    }else if(swrst > 50 && swunt == 0){
      //reset the length calculation here
      SERIAL.println(F("Return to normal mode"));
      mode = NORMAL_MODE;
      swrst = 0;
      swunt = 0;
    }
    else if(swrst == 0 && swunt > 100 ){
       swrst = 0;
       swunt = 0;
      //change the units heres
      if(angle_abs > 0){
          SERIAL.println(F("save calibration pos"));
          eestore.es.cal = 1000.0 / angle_abs;
       }else if(angle_abs < 0){
          SERIAL.println(F("save calibration neg"));
          eestore.es.cal = -1000.0 / angle_abs;
        }else{
          SERIAL.println(F("zero calibration not possible"));
        }
          eestore_rw(eestore.b,sizeof(eestore),EEWRITE);
          cal_mm = eestore.es.cal;
          cal_in = cal_mm / 25.4;
          mode = NORMAL_MODE;
    }
  }
}

//void do_swrst(void)
//{
//  swrst[1] = swrst[0];
//  swrst[0] = digitalRead(SW1_RESET_PIN);
//  if(swrst[0] == 0 && swrst[1] == 1){
//    //reset the length calculation here
//    SERIAL.println(F("Reset Length"));
//    angle_abs=0;
//  }
//}
//
//void do_swunt(void)
//{
//  swunt[1] = swunt[0];
//  swunt[0] = digitalRead(SW2_UNITS_PIN);
//  if(swunt[0] == 0 && swunt[1] == 1){
//    //change the units heres
//    SERIAL.println(F("change Unit"));
//    unit = unit == mm ? inch : mm;
//  }
//}

void doEncoder(){
  digitalWrite(DEBUG_PIN, HIGH);
   angle[1] = angle[0];
   angle[0] = ams5600.getRawAngle();
   
   //if(angle[0] < 128 && angle[1]> (4095-128)){
   // angle_delta = angle[0] - angle[1] + 4095;
   //}else if(angle[0]>(4095-128) && angle[1] < 128){
   // angle_delta = (angle[0] - angle[1] - 4095); 
   //}else{
   // angle_delta = angle[0]-angle[1];
   //}

   angle_delta = angle[0]-angle[1];
   if(angle_delta >= 4095-512 || angle_delta <= -4095 + 512){
    if(angle[0] >=0 && angle[0] <= 512){
      angle_delta = angle[0] + 4095 - angle[1];
    }else{
      angle_delta = angle[0] - 4095 - angle[1];
    }
    //Serial.println(F("wrap"));
   }
   
   switch(encoder_state){
    case 0:
      if(encoder_onEntry){
        Serial.println(F("Initializing Encoder:"));
        encoder_onEntry=1;
        encoder_state = 1;
        encoder_counter = 0;
        
        angle[0] = ams5600.getRawAngle();
        angle[1] = angle[0];
        angle_start = angle[0];
        angle_abs = 0;
      }
      
    break;
    case 1:
      if(encoder_onEntry){
        encoder_onEntry = 0;
        encoder_counter = 0;
        Serial.println(F("monitoring:"));
      }
       angle_abs += angle_delta;
      
    break;
   }

   digitalWrite(DEBUG_PIN, LOW);
}

void dfunc(void){
  Serial.print(".");
}

void updateDisplayNormal(void){
  display.clearDisplay();
 //draw frame
  display.drawLine(0,0,0,display.height()-1,SSD1306_WHITE);
  display.drawLine(0,display.height()-1,display.width()-1,display.height()-1,SSD1306_WHITE);
  display.drawLine(display.width()-1,display.height()-1,display.width()-1,0,SSD1306_WHITE);
  display.drawLine(display.width()-1,0,0,0,SSD1306_WHITE);
  display.drawLine(0,18,display.width()-1,18,SSD1306_WHITE);
    
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(2,2);                                                                                                     // Start at top-left corner
  sprintf(cstr,"Unit:%s",unit == mm ? "mm":"in");
  display.println(cstr);
  
  float value;
  char sval[8];
  if(unit == mm){
    value = angle_abs * cal_mm;
    dtostrf(value, 4, 1, sval);
    sprintf(cstr,"%7s",sval);
  }else{
    value = angle_abs * cal_in;
    dtostrf(value, 4, 2, sval);
    sprintf(cstr,"%7s",sval);
  }
  
  display.setCursor(1,26);
  display.setTextSize(3);
  display.print(cstr);
  digitalWrite(DEBUG_PIN, HIGH);
  display.DISPLAY(doEncoder);
  digitalWrite(DEBUG_PIN, LOW);  
}

void updateDisplayCal(void){
  display.clearDisplay();
 //draw frame
  display.drawLine(0,0,0,display.height()-1,SSD1306_WHITE);
  display.drawLine(0,display.height()-1,display.width()-1,display.height()-1,SSD1306_WHITE);
  display.drawLine(display.width()-1,display.height()-1,display.width()-1,0,SSD1306_WHITE);
  display.drawLine(display.width()-1,0,0,0,SSD1306_WHITE);
  display.drawLine(0,18,display.width()-1,18,SSD1306_WHITE);
    
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setCursor(2,2);
  display.setTextColor(WHITE);        // Draw white text
  display.println(F("Cal:1000mm"));
  
  float value;
  char sval[8];

  value = angle_abs;
  dtostrf(value, 4, 0, sval);
  sprintf(cstr,"%7s",sval);
  
  display.setCursor(1,26);
  display.setTextSize(3);
  display.print(cstr);
  digitalWrite(DEBUG_PIN, HIGH);
  display.DISPLAY(doEncoder);
  digitalWrite(DEBUG_PIN, LOW);
}

void updateDisplay(){
    if(mode == CAL_MODE){
      updateDisplayCal();
    }else{
      updateDisplayNormal();
    }
}
