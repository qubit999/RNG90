#ifndef RNG90_H
#define RNG90_H

#include <Arduino.h>
#include <Wire.h>

class RNG90 {
public:
  // I2C address of the RNG90 device
  static constexpr uint8_t I2C_ADDR = 0x40;

  enum Status : uint8_t {
    SUCCESS        = 0x00,
    PARSE_ERROR    = 0x03,
    SELFTEST_ERROR = 0x07,
    HEALTH_ERROR   = 0x08,
    EXEC_ERROR     = 0x0F,
    WAKE_SUCCESS   = 0x11,
    COMM_ERROR     = 0xFF
  };

  explicit RNG90(TwoWire &wirePort = Wire);
  void begin(uint8_t sdaPin = 255, uint8_t sclPin = 255);

  Status wake();
  Status sleep();
  Status info(uint8_t rev[4]);
  Status random(uint8_t buf[32]);
  Status readSerial(uint8_t serial[16]);
  Status selfTest(uint8_t mode, uint8_t &result);
  Status lastError() const { return _lastError; }

private:
  TwoWire &_wire;
  Status   _lastError;

  Status crcCheck(const uint8_t *buf, size_t len);
  bool    waitReady(uint32_t timeout = 100);
  Status sendCommand(uint8_t opcode, uint8_t p1, uint16_t p2,
                     const uint8_t *data, size_t dataLen);
  Status readResponse(uint8_t *data, size_t expectedLen);

  static uint16_t crc16(const uint8_t *buf, size_t len);
};

#endif // RNG90_H
