#include "RNG90.h"
#include "RNGString.h"
#include <Arduino.h>

RandomStringGenerator::RandomStringGenerator(RNG90& rngDevice) : rng(rngDevice), dataIndex(32) {}
    
void RandomStringGenerator::refreshRandomData() {
    rng.random(randomData);
    dataIndex = 0;
}

char RandomStringGenerator::getRandomCharUnbiased(uint8_t* randomData, size_t& index, const char* charset) {
    size_t charsetLen = strlen(charset);
    uint8_t maxValid = (256 / charsetLen) * charsetLen - 1;
    
    while (index < 32) { 
        uint8_t randomByte = randomData[index++];
        if (randomByte <= maxValid) {
            return charset[randomByte % charsetLen];
        }
    }
    return charset[0];
}
    
String RandomStringGenerator::generateString(size_t length, const char* charset) {
    String result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; i++) {
        if (dataIndex >= 32) {
            refreshRandomData();
        }
        result += getRandomCharUnbiased(randomData, dataIndex, charset);
    }
    return result;
}

String RandomStringGenerator::generateRandomString(size_t length) {
    return generateString(length, CHARSET);
}