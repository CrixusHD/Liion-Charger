#include <Wire.h>

#include "akku.h"


Akku *akkus[8] = {
    new Akku(PIN_PB5),
    new Akku(PIN_PB4),
    new Akku(PIN_PB1),
    new Akku(PIN_PB0),
    new Akku(PIN_PC0),
    new Akku(PIN_PC1),
    new Akku(PIN_PC2),
    new Akku(PIN_PC3)
};

// I2C-Adresse des Slaves (z.B. 0x50)
#define SLAVE_ADDRESS 0x50

// Pin-Belegung (Diese werden durch PORTMUX gesetzt, aber gut zur Info)
// PA1 = SDA (Pin 17)
// PA2 = SCL (Pin 18)


// I2C-Interrupt-Handler (wird bei Anfrage vom Master aufgerufen)
ISR(TWI0_TWIS_vect) { // Korrekter Vektorname für TWI Slave Interrupt
    uint8_t status = TWI0.SSTATUS;
    static uint8_t akkuIndex = 0; // Index muss außerhalb der ISR gespeichert werden

    // 1. **Address Match Interrupt (APIF)**: Master hat uns angesprochen.
    if (status & TWI_APIF_bm) {

        // **Master liest (DIR = 1)**: Slave muss Daten senden.
        if (status & TWI_DIR_bm) {

            // Schleife zum Senden aller Akku-Spannungen (Daten)
            static uint8_t akkuIndex = 0;

            if (status & TWI_AP_bm) { // Adress-Paket (START/REPEATED START)
                akkuIndex = 0; // Bei neuer Anfrage Startindex zurücksetzen
            }

            if (akkuIndex < 8) {
                 // Sende den nächsten Spannungswert (dezimal * 100)
                TWI0.SDATA = static_cast<uint8_t>(akkus[akkuIndex]->voltage * 100);
                akkuIndex++;

                // Setze ACK (TWI_ACKACT_bm = 0) und APIF löschen
                TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm;
            } else {
                // Alle Daten gesendet, setze NACK (TWI_ACKACT_bm = 1), um Master zu stoppen.
                TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm | TWI_ACKACT_bm;
            }

        // **Master schreibt (DIR = 0)**: Slave empfängt Daten (nicht implementiert).
        } else {
            // Hier müssten Sie Logik für den Empfang von Daten einfügen.
            // Zuerst APIF und DIF löschen, um den nächsten Byte-Transfer zu erlauben
            TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm;
        }
    }

}


void setup() {
    // I2C-Setup für Slave
    PORTMUX.CTRLB |= PORTMUX_TWI0_bm; // I2C auf PA0/PA1 (SDA/SCL)

    // 2. TWI (I2C) Slave initialisieren
    TWI0.SADDR = SLAVE_ADDRESS << 1;   // Adresse setzen
    TWI0.SCTRLA = TWI_ENABLE_bm;       // TWI-Slave aktivieren
    TWI0.SCTRLB = TWI_SMEN_bm | TWI_DIEN_bm | TWI_APIEN_bm; // Smart-Mode, Daten-Int, Adress-Int

    for (const Akku *single: akkus) {
        pinMode(single->getPin(), INPUT);
    }
}

double teiler = 5000.0f / (220.0f + 5000.0f);

double getVolt(int pin) {
    volatile float analog_value = analogRead(pin);
    return ((analog_value * 5.0f) / 1023.0f) / teiler;
}

unsigned long lastMeasure = 0;

void readAllBatteryVoltages() {
    if (millis() - lastMeasure >= 1000) {
        for (Akku *single: akkus) {
            single->voltage = getVolt(single->getPin());
        }
        lastMeasure = millis();
    }
}


void loop() {
    readAllBatteryVoltages();
}
