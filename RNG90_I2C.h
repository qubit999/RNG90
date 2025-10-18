// File: RNG90_I2C.h
#pragma once

// Uncomment this to use TinyWireM on ATTiny platforms
// #define USE_TINY_WIRE_M_

#ifdef USE_TINY_WIRE_M_
  #include <TinyWireM.h>
  using I2C = TinyWireM;        // ATTiny I2C
#else
  #include <Wire.h>
  using I2C = TwoWire;          // Standard Arduino I2C
#endif
