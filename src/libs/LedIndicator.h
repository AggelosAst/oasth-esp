//
// Created by AggelosD on 4/6/2024.
//
#include "Arduino.h"

#ifndef OASTH_LEDINDICATOR_H
#define OASTH_LEDINDICATOR_H


class LedIndicator {
public:
    void static indicateActiveRoute();

    void static indicateBad();

    void static indicateGood();

    void static indicateNeutral();

    [[noreturn]] void static indicateBadConstant();

    static constexpr const int redPin = 23;
    static constexpr const int greenPin = 19;
    static constexpr const int bluePin = 18;
};


void LedIndicator::indicateActiveRoute() {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 255);
}

void LedIndicator::indicateBad() {
    analogWrite(redPin, 255);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
}


void LedIndicator::indicateGood() {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 255);
    analogWrite(bluePin, 0);
}

void LedIndicator::indicateNeutral() {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
}

[[noreturn]] void LedIndicator::indicateBadConstant() {
    while (true) {
        indicateBad();
        delay(300);
        indicateNeutral();
        delay(300);
    }
}


#endif //OASTH_LEDINDICATOR_H
