#include <Arduino.h>
// #include <pins_arduino.h>
#include <avr/interrupt.h>

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

// I2C-Interrupt-Handler (wird bei Anfrage vom Master aufgerufen)
ISR(TWI0_TWIS_vect)
{
    uint8_t status = TWI0.SSTATUS;
    static uint8_t akkuIndex = 0;

    // 1. **Address Match Interrupt (APIF)**: Master hat uns angesprochen.
    if (status & TWI_APIF_bm)
    {
        // **Master liest (DIR = 1)**: Slave muss Daten senden.
        if (status & TWI_DIR_bm)
        {
            if (status & TWI_AP_bm)
            {
                // Adress-Paket (START/REPEATED START)
                akkuIndex = 0; // Bei neuer Anfrage Startindex zurücksetzen
            }

            if (akkuIndex < 8)
            {
                // Sende den nächsten Spannungswert (dezimal * 100)
                TWI0.SDATA = static_cast<uint8_t>(akkus[akkuIndex]->voltage * 100);
                akkuIndex++;

                // Setze ACK (TWI_ACKACT_bm = 0) und Flags löschen
                TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm;
            }
            else
            {
                // Alle Daten gesendet, setze NACK (TWI_ACKACT_bm = 1)
                TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm | TWI_ACKACT_bm;
            }

            // **Master schreibt (DIR = 0)**: Slave empfängt Daten
        }
        else
        {
            // Daten lesen (falls Master schreibt)
            uint8_t receivedData = TWI0.SDATA;

            // Hier könntest du die empfangenen Daten verarbeiten
            // Beispiel: verschiedene Kommandos vom Master

            // ACK senden und Flags löschen
            TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm;
        }
    }

    // Data Interrupt Flag (DIF) - weitere Bytes übertragen
    if (status & TWI_DIF_bm)
    {
        if (status & TWI_DIR_bm)
        {
            // Master liest
            if (akkuIndex < 8)
            {
                TWI0.SDATA = static_cast<uint8_t>(akkus[akkuIndex]->voltage * 100);
                akkuIndex++;
                TWI0.SSTATUS = TWI_DIF_bm; // DIF löschen
            }
            else
            {
                // NACK senden wenn alle Daten übertragen
                TWI0.SSTATUS = TWI_DIF_bm | TWI_ACKACT_bm;
            }
        }
        else
        {
            // Master schreibt
            uint8_t receivedData = TWI0.SDATA;
            // Verarbeitung der empfangenen Daten hier
            TWI0.SSTATUS = TWI_DIF_bm; // DIF löschen
        }
    }
}

unsigned long lastMeasure = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("ATtiny I2C Slave gestartet");

    // Hardware I2C-Setup für Slave (PA1=SDA, PA2=SCL)
    PORTMUX.CTRLB |= PORTMUX_TWI0_bm;

    // TWI (I2C) Slave initialisieren
    TWI0.SADDR = SLAVE_ADDRESS << 1; // Adresse setzen
    TWI0.SCTRLA = TWI_ENABLE_bm; // TWI-Slave aktivieren
    TWI0.SCTRLB = TWI_SMEN_bm | TWI_DIEN_bm | TWI_APIEN_bm; // Smart-Mode, Daten-Int, Adress-Int

    // Interrupts global aktivieren
    sei();

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

        // Debug-Ausgabe über Serial
        // Serial.print("Spannungen: ");
        // for (int i = 0; i < 8; i++) {
        //     Serial.print("Akku");
        //     Serial.print(i);
        //     Serial.print(": ");
        //     Serial.print(akkus[i]->voltage);
        //     Serial.print("V ");
        // }
        // Serial.println();
    }
}


void loop()
{
    readAllBatteryVoltages();
}
