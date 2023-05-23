#include <Arduino.h>
#include "pins.h"
#include "jsettime.h"
#include "readvolt.h"
#include "util.h"
#include "timerdelay.h"
#include "btcontroller.h"

TimerDelay btSendDelay(false, 2000);
BluetoothController btController;

void dropLoads();

// Voltage thresholds to turn on and off relay R1 and R2
float voltageTurnOnR1 = 25.5;
float voltageTurnOffAll = 24.9;
float voltageTurnOnR2 = 27.0;
float voltageTurnOffR2 = 26.5;

// Output variables
bool r1_on = false;
bool r1_and_r2_on = false;

// States
enum State
{
    OFF, // == Start
    R1_ON,
    R1_AND_R2_ON
};

// Helper for print labels instead integer when state change
const char * const stateName[] PROGMEM = {"OFF", "R1_ON", "R1_AND_R2_ON"};

// Current state
//State current_state = OFF;

// The three states are tracked and maintained using the two output variables
State current_state()
{
    if(r1_and_r2_on) { return R1_AND_R2_ON; }
    else if(r1_on) { return R1_ON; }
    else return OFF;
}

void sendVoltage(float voltage);

void setup()
{
    setupPins(); // set up Input/Output
    Serial.begin(115200);
    while (!Serial) {
    } // Needed for native USB port only

    Serial.println(F("Starting the Voltage Controlled Switch...\n"));
    dropLoads(); // turn off all loads

    int year = 2023;
    int month = 4; // [0,11], January = 0
    int day = 24;
    int hour = 0; // [0,24]
    int minute = 0; // [0,59]
    int second = 0; // [0.59]

    jsettime(year, month, day, hour, minute, second );

    // Initialize bluetooth and its delay timer
    btController.init();
    btSendDelay.start();
    btController.sendInfo("Initializing voltage_controlled_relay...");
}

void loop()
{
    State old_state = current_state();
    float v = readVoltage();

    btController.send("BAT_VOLTS", String(v));

    if( OFF == old_state)
    {
        if( voltageTurnOnR2 < v ) { r1_and_r2_on = true; }
        else if( voltageTurnOnR1 < v ) { r1_on = true; }
    }
    else if( R1_ON == old_state)
    {
        if( voltageTurnOffAll > v ) { r1_on = r1_and_r2_on = false; }
        else if( voltageTurnOnR2 < v ) { r1_and_r2_on = true; }
    }
    else if( R1_AND_R2_ON == old_state)
    {
        if( voltageTurnOffAll > v ) { r1_on = r1_and_r2_on = false; }
        else if( voltageTurnOffR2 > v ) { r1_and_r2_on = false; }
    }

    Serialprintln( "Voltage %fV - state %s --> %s",
      v, stateName[old_state], stateName[current_state()]);

    // Set outputs
    digitalWrite(LOAD_1_LED, r1_on || r1_and_r2_on);
    digitalWrite(LOAD_1_RELAY, r1_on || r1_and_r2_on);
    digitalWrite(LOAD_2_LED, r1_and_r2_on);
    digitalWrite(LOAD_2_RELAY, r1_and_r2_on);

    // Sync the bluetooth controller
    BluetoothData btData = btController.sync();

    if(btData.available()) {
        // Handle data sent over bluetooth
        Serial.print("CMD -> '");
        Serial.print(btData.getCommand());
        Serial.print("\tData -> '");
        Serial.print(btData.getData());
        Serial.println("'");
    }

    delay(500); // Sleep
}

void setOn(int pin)
{
    digitalWrite(pin, HIGH);
}

void setOff(int pin)
{
    digitalWrite(pin, LOW);
}

// Turn ON/OFF the loads
void dropLoads()
{
    setOff(LOAD_1_RELAY);
    setOff(LOAD_1_LED);

    setOff(LOAD_2_RELAY);
    setOff(LOAD_2_LED);

    setOff(LOAD_3_RELAY);
    setOff(LOAD_3_LED);

    setOff(LOAD_4_RELAY);
    setOff(LOAD_4_LED);

    setOff(LOAD_5_RELAY);
    setOff(LOAD_5_LED);

    setOff(LOAD_6_RELAY);
    setOff(LOAD_6_LED);
}
