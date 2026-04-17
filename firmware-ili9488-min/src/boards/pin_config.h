#pragma once

#include <Arduino.h>

namespace board {

namespace display {
constexpr int kMosi = 11;
constexpr int kMiso = 13;
constexpr int kSclk = 12;
constexpr int kCs = 10;
constexpr int kDc = 9;
constexpr int kRst = 14;
constexpr int kBacklight = 21;
constexpr bool kBacklightOnGpio = false;
}  // namespace display

namespace storage {
constexpr int kSdCs = 8;
}  // namespace storage

namespace audio {
constexpr int kPwmOut = 42;
constexpr int kI2sBclk = 40;
constexpr int kI2sLrck = 41;
constexpr int kI2sDout = 42;
}  // namespace audio

namespace input {
constexpr int kUp = 4;
constexpr int kDown = 5;
constexpr int kLeft = 6;
constexpr int kRight = 7;

constexpr int kA = 15;
constexpr int kB = 16;
constexpr int kX = 17;
constexpr int kY = 18;

constexpr int kStart = 47;
constexpr int kSelect = 48;
constexpr int kL = 38;
constexpr int kR = 39;
}  // namespace input

}  // namespace board
