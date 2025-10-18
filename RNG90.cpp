// File: RNG90.cpp
#include "RNG90.h"

RNG90::RNG90(TwoWire &wirePort)
  : _wire(wirePort), _lastError(COMM_ERROR) {}

void RNG90::begin(uint8_t sdaPin, uint8_t sclPin) {
    if (sdaPin != 255 && sclPin != 255) {
      Wire.setSDA(sdaPin);
      Wire.setSCL(sclPin);
    }
  _wire.begin();
  _wire.setClock(400000);
}

RNG90::Status RNG90::wake() {
  _wire.beginTransmission(I2C_ADDR);
  uint8_t code = _wire.endTransmission();
  
  delay(2);  
  
  uint32_t start = millis();
  while (millis() - start < 5) {
    _wire.beginTransmission(I2C_ADDR);
    if (_wire.endTransmission() == 0) break;
    delay(1);
  }

  if (_wire.requestFrom(I2C_ADDR, (uint8_t)4) == 4) {
    for (uint8_t i = 0; i < 4; ++i) {
      (void)_wire.read();
    }
  }

  _lastError = static_cast<Status>(code);
  return (code == 2) ? WAKE_SUCCESS : COMM_ERROR;
}


// Sleep: write word address = 0x01
RNG90::Status RNG90::sleep() {
  _wire.beginTransmission(I2C_ADDR);
  _wire.write(0x01);
  uint8_t code = _wire.endTransmission();
  _lastError = static_cast<Status>(code);
  return (code == 0) ? SUCCESS : COMM_ERROR;
}

RNG90::Status RNG90::sendCommand(uint8_t opcode, uint8_t p1, uint16_t p2,
                                 const uint8_t *data, size_t dataLen) {
  size_t packetLen = 1 + 1 + 2 + dataLen;
  size_t totalLen = 1 + packetLen + 2;

  if (totalLen > 87) return PARSE_ERROR;
  
  uint8_t buf[90];
  size_t idx = 0;
  
  buf[idx++] = totalLen;
  buf[idx++] = opcode;
  buf[idx++] = p1;
  buf[idx++] = lowByte(p2);
  buf[idx++] = highByte(p2);
  
  if (dataLen > 0 && data != nullptr) {
    memcpy(buf + idx, data, dataLen);
    idx += dataLen;
  }
  
  uint16_t crc = crc16(buf, idx);
  buf[idx++] = lowByte(crc);
  buf[idx++] = highByte(crc);
    
  _wire.beginTransmission(I2C_ADDR);
  _wire.write(0x03);
  
  for (size_t i = 0; i < idx; i++) {
    _wire.write(buf[i]);
  }
  
  uint8_t result = _wire.endTransmission();
  
  _lastError = static_cast<Status>(result);
  return (result == 0) ? SUCCESS : COMM_ERROR;
}

bool RNG90::waitReady(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    _wire.beginTransmission(I2C_ADDR);
    if (_wire.endTransmission() == 0) {
      return true;
    }
    delay(10); 
  }
  return false;
}

RNG90::Status RNG90::readResponse(uint8_t *data, size_t expectedLen) {
  if (!waitReady(5000)) return EXEC_ERROR;
  
  _wire.beginTransmission(I2C_ADDR);
  _wire.write(0x00); 
  _wire.endTransmission();
  
  size_t groupSize = 1 + expectedLen + 2; 
  
  if (_wire.requestFrom(I2C_ADDR, (uint8_t)groupSize) != groupSize) {
    Serial.println("Failed to read expected bytes");
    return COMM_ERROR;
  }
  
  uint8_t count = _wire.read();
  
  if (count == 4) {
    uint8_t status = _wire.read();
    uint8_t crcLo = _wire.read();
    uint8_t crcHi = _wire.read();
    
    Serial.print("Error status: 0x"); Serial.println(status, HEX);
    
    uint8_t tmp[2] = {count, status};
    uint16_t expectedCrc = crc16(tmp, 2);
    uint16_t receivedCrc = (uint16_t)crcHi << 8 | crcLo;
    
    if (expectedCrc != receivedCrc) {
      Serial.println("CRC mismatch on error response");
      return COMM_ERROR;
    }
    
    _lastError = static_cast<Status>(status);
    return _lastError;
  }
  
  if (count != groupSize) {
    Serial.print("Count mismatch. Expected: "); Serial.print(groupSize);
    Serial.print(", Got: "); Serial.println(count);
    return PARSE_ERROR;
  }
  
  for (size_t i = 0; i < expectedLen; i++) {
    data[i] = _wire.read();
  }
  
  uint8_t crcLo = _wire.read();
  uint8_t crcHi = _wire.read();
  uint16_t receivedCrc = (uint16_t)crcHi << 8 | crcLo;
  
  uint8_t tmp[expectedLen + 1];
  tmp[0] = count;
  memcpy(tmp + 1, data, expectedLen);
  uint16_t expectedCrc = crc16(tmp, expectedLen + 1);
    
  if (expectedCrc != receivedCrc) {
    Serial.println("CRC mismatch on data response");
    return COMM_ERROR;
  }
  
  return SUCCESS;
}

uint16_t RNG90::crc16(const uint8_t *data, size_t length) {
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;
    
    for (size_t counter = 0; counter < length; counter++) {
        for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
            data_bit = ((data[counter] & shift_register) != 0) ? 1 : 0;
            crc_bit = (uint8_t)(crc_register >> 15);
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }
    
    return crc_register;
}

RNG90::Status RNG90::info(uint8_t rev[4]) {
  // 0x30 for Info command
  Status st = sendCommand(0x30, 0x00, 0x0000, nullptr, 0);
  if (st != SUCCESS) return st;
  
  uint8_t buf[4];
  st = readResponse(buf, sizeof(buf));
  if (st != SUCCESS) return st;
  
  memcpy(rev, buf, 4);
  return SUCCESS;
}

RNG90::Status RNG90::readSerial(uint8_t serial[16]) {
  // Correct opcode: 0x02, Param1: 0x01 for serial read
  Status st = sendCommand(0x02, 0x01, 0x0000, nullptr, 0);
  if (st != SUCCESS) return st;
  
  uint8_t buf[16];
  st = readResponse(buf, sizeof(buf));
  if (st != SUCCESS) return st;
  
  memcpy(serial, buf, 16);
  return SUCCESS;
}

RNG90::Status RNG90::random(uint8_t bufOut[32]) {
  // 0x16 for Random command
  uint8_t dummy[20] = {0};
  Status st = sendCommand(0x16, 0x00, 0x0000, dummy, sizeof(dummy));
  if (st != SUCCESS) return st;
  
  uint8_t buf[32];
  st = readResponse(buf, sizeof(buf));
  if (st != SUCCESS) return st;
  
  memcpy(bufOut, buf, 32);
  return SUCCESS;
}

// SelfTest: run on-demand self-tests, mode=0x00/0x01/0x20/0x21
RNG90::Status RNG90::selfTest(uint8_t mode, uint8_t &result) {
  Status s = sendCommand(0x77, mode, 0x0000, nullptr, 0);
  return (s != SUCCESS) ? s : readResponse(&result, 1);
}
