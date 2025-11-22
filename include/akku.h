//
// Created by Nick on 22.11.2025.
//

#ifndef LIION_CHARGER_AKKU_H
#define LIION_CHARGER_AKKU_H

class Akku {
    int pin;

public:
    Akku(int pin);

    double voltage = 0;

    [[nodiscard]] int getPin() const {
        return pin;
    }
};
#endif //LIION_CHARGER_AKKU_H
