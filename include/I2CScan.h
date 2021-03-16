#pragma once

#include <Arduino.h>
#include <Wire.h>


void i2cScan(const char *wireName, TwoWire * wire);
