#pragma once
#include "RNG90.h"
#include <Arduino.h>
#include <cstring>

class RandomStringGenerator {
private:
    uint8_t randomData[32];
    size_t dataIndex;
    RNG90& rng;
    const char* CHARSET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?/~`";
public:
    RandomStringGenerator(RNG90& rngDevice);
    void refreshRandomData();
    char getRandomCharUnbiased(uint8_t* randomData, size_t& index, const char* charset);
    String generateString(size_t length, const char* charset);
    String generateRandomString(size_t length);
};