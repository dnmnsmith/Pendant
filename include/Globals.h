#pragma once
#include <Arduino.h>

#include "LcdDisplay.h"
#include "EepromValues.h"
#include "LEDs.h"
#include "OLED.h"
#include "I2CInput.h"
#include "BluetoothSerial.h"
#include "GrblState.h"
#include "GrblSettings.h"
#include "JogController.h"
#include "KeyHandler.h"

#include <Adafruit_MCP23008.h>

extern SemaphoreHandle_t mutexI2C;

extern LcdDisplay lcdDisplay;
extern EepromValues eepromValues;
extern LEDs leds;
extern OLED stateOled;
extern OLED tloOled;
extern OLED feedOled;
extern OLED feedPercentOled;
extern I2CInput i2cinput;

extern BluetoothSerial SerialBT;
extern Adafruit_MCP23008 mcp;

extern GrblState grblState;
extern GrblSettings grblSettings;
extern JogController jogController;
extern KeyHandler keyHandler;

