# FilamentMeasuringTool
Filament Measuring Tool for 3D printing using Arduino.
Measure 1.75mm 3D printing Filament.

 ![MainPage](https://github.com/bmoniey/FilamentMeasuringTool/blob/main/gallery/fmt20_assy.png?raw=true)

## Design Targets

* Accuracy: 1mm over 1 meter
* Cost: <$20
* Easy to use. 

## ECAD

![ECAD](https://github.com/bmoniey/FilamentMeasuringTool/blob/main/design/ecad/fmt.svg)


## Library Dependencies

Copy these libraries to your Arduino Libraries folder for the main or protected branch.

* https://github.com/adafruit/Adafruit_SSD1306.git
* https://github.com/adafruit/Adafruit-GFX-Library.git
* https://github.com/Seeed-Studio/Seeed_Arduino_AS5600.git


## Links

* FMT on Grab CAD (step,stl there):https://grabcad.com/library/filament-measuring-tool-1
* Pylen:https://github.com/bmoniey/pylen
* Filament Welding tool:https://grabcad.com/library/3dprint-filament-welder-1
* bmoniey's Website:https://gear-blocks.com

## Bill of Material

- 1 Arduino Nano (ASIN B0713XK923)
- 1 AS5600 kit (PCB,magnet horizontal,5mmx2mm Amazon ASIN:B094F8H591)
- 1 OLED,128x64,I2C Display (ASIN B076PNP2VD)
- 4 M5 x 20 Socket Head Cap Screw
- 3 Ball Bearing,608,ABEC-9, 8x22x7 
- 4 Stand-off M3x10
- 4 Stand-off M2x5
- 1 PCB 50x70 (ASIN B081Q5JZ43)
- 2 header,male, horizontal,2.54mm 
- 2 Push-button, momentary
- 8 Dupont wire jumpers 20cm
- 1 housing_top, 3d printed
- 1 housing_bot, 3d printed
- 1 housing_front, 3d printed
- 1 Wheel, 3d printed
- 1 Snap-ring, 3d printed

## Usage

### Power

Use a USB C to Mini cable for power. Any USB C wall adapter or USB C port should work.
	
### Normal Mode (out of reset)

Will display units and the length with appropriate precision.

- Reset Button: Press to zero length readout.
- Units Button: Press to change between mm and inch
- Press reset and then press and hold units to enter calibration mode

### Calibration Mode:

Expects to record raw counts for 1000mm of filament. The display will show the raw position in counts.

Steps:
1. Zero the position at your measurement starting point with reset button
2. Draw out exactly 1000mm or 39.37in of filament (using a tape measure or other reference)
3. Press the units button (value must be non-zero. Negative values OK too!) to save new calibration
4. Press reset and then press and hold units to re-enter normal mode

- Reset Button: Press to zero length.  
- Units Button: Stores the calibration and returns to Normal Mode

### Notes:
1. Using while Cura is open is not recommended. The software tries to access the serial port of the Arduino and funny things can start to happen. Simple close Cura when using the device.

## Version

### 2.34.1 (10/11/2021)
- This update is the result of a merge from the protected branch
- Successful change to Adafruit_SSD1306 library for change from private to protected data. Thanks to the open source community and LadyAda for working on the pull request.
- code clean-up using the clang-format tool in windows
- inverted this revision list so latest changes go first!

### 2.23.1 (10/3/2021)
- working with Adafruit on change from private to protected
- added back in the Adafruit_SSD1306_D.cpp,h files which implement the display(dfunct_t dfunct) method which is less complicated and will be able to benefit from any other modifications made to the base library. Hopefully the protected will become mainline which will allow this fork to die and no special library checkout will be needed.

### 2.18.1 (10/2/2021)
- revision now in format major.git-revision.build numbers
- added note to display to help with tracking branches
- start-up delay from 1s to 2s to give user a change to read it!
- added comments in the fmt.ino file to indicate what branch the project goes with

### 2.1.1
- removed unused variables and comments
- created defines for magic numbers
- improved handling of button presses
- improved entry and return from calibration mode

### 2.1.0 
- store and read calibration from eeprom
- added calibration mode
- added updates to push-buttons to handle calibration mode

### 2.0.0 (9/25/2021)
- Initial Release of FMT20

### 1.0.0
- Failed attempt using an analog rotary encoder
