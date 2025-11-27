#include <Arduino.h>
#include <Wire.h>
#include "akku.h"

Akku* akkus[8] = {
    new Akku(PIN_PB5, true),
    new Akku(PIN_PB4, true),
    new Akku(PIN_PB1, true),
    new Akku(PIN_PB0, true),
    new Akku(PIN_PC0, false),
    new Akku(PIN_PC1, false),
    new Akku(PIN_PC2, false),
    new Akku(PIN_PC3, false)
};

// I2C-Adresse des Slaves (z.B. 0x50)
#define SLAVE_ADDRESS 0x50

// Pin-Belegung für Hardware I2C (Slave-Modus)
// PA1 = SDA (Pin 17)
// PA2 = SCL (Pin 18)

void requestEvent() {
    // Alle 8 Akkuspannungen als Bytes senden
    for (int i = 0; i < 8; i++) {
        uint8_t voltageData = static_cast<uint8_t>(akkus[i]->voltage * 100);
        Wire.write(voltageData);
    }
}

void receiveEvent(int howMany) {
    Serial.print("Empfangen: ");
    Serial.print(howMany);
    Serial.println(" Bytes");

    while (Wire.available()) {
        uint8_t data = Wire.read();
        Serial.print("Data: 0x");
        Serial.println(data, HEX);
    }
}
unsigned long lastMeasure = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("ATtiny I2C Slave gestartet");

    // Hardware I2C-Setup für Slave (PA1=SDA, PA2=SCL)
    PORTMUX.CTRLB |= PORTMUX_TWI0_bm;

    Wire.begin(SLAVE_ADDRESS);        // Als Slave initialisieren
    Wire.onRequest(requestEvent);     // Callback für Master-Anfragen
    Wire.onReceive(receiveEvent);

    init_ADC0();
    init_ADC1();

    for (const Akku* single : akkus)
    {
        pinMode(single->getPin(), INPUT);
    }

    Serial.println("I2C Slave bereit auf Adresse 0x50");
}

double teiler = 5000.0f / (220.0f + 5000.0f);

double getVolt(int pin, bool adc0)
{
    volatile float analog_value;
    if (adc0)
    {
        analog_value = analogRead(pin);
    }
    else
    {
        analog_value = analogRead1(pin);
    }
    return ((analog_value * 5.0f) / 1023.0f) / teiler;
}

void readAllBatteryVoltages()
{
    if (millis() - lastMeasure >= 1000)
    {
        for (Akku* single : akkus)
        {
            single->voltage = getVolt(single->getPin(), single->isAdc0());
        }
        lastMeasure = millis();
    }
}


void loop()
{
    readAllBatteryVoltages();
}
