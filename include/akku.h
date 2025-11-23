//
// Created by Nick on 22.11.2025.
//

#ifndef LIION_CHARGER_AKKU_H
#define LIION_CHARGER_AKKU_H

class Akku {
    int pin;
    bool adc0;

public:
    Akku(int pin, bool adc0);

    double voltage = 0;

    [[nodiscard]] int getPin() const {
        return pin;
    }
    [[nodiscard]] bool isAdc0() const {
        return adc0;
    }
};
#endif //LIION_CHARGER_AKKU_H
