#define BLYNK_TEMPLATE_ID "TMPL6MCXDxrbt"
#define BLYNK_TEMPLATE_NAME "Water level Monitor"
#define BLYNK_AUTH_TOKEN "Z7EEkokzEkUnip752SHPEV0aF_pAKcY4"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>

// LCD setup for I2C address 0x27, 16x4 display
LiquidCrystal_I2C lcd(0x27, 16, 4);

// Ultrasonic Sensor Pins
const int trigPin = 5;
const int echoPin = 18;

// LED and Buzzer Pins
const int redLEDPin = 23;
const int greenLEDPin = 25;
const int orangeLEDPin = 26;
const int buzzerPin = 27;

// Water Flow Sensor Pin
const int flowSensorPin = 19;
volatile int pulseCount = 0;
float flowRate = 0.0;
float totalVolume = 0.0;
unsigned long lastMillis = 0;

// Water Level Thresholds (in cm)
const int safeLevel = 20;
const int warningLevel = 10;

// PIR Sensor Pin
const int pirPin = 4;
int motionState = 0;

// Temperature Sensor Pin (LM35)
const int tempPin = 34;

// Variables for distance calculation
long duration;
int distanceCm;

// Interrupt service routine for water flow sensor
void IRAM_ATTR countPulses() {
    pulseCount++;
}

void setup() {
    Blynk.begin(BLYNK_AUTH_TOKEN, "Homies", "asdfghjkl");

    lcd.init();
    lcd.backlight();

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    pinMode(redLEDPin, OUTPUT);
    pinMode(greenLEDPin, OUTPUT);
    pinMode(orangeLEDPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(pirPin, INPUT);
    pinMode(tempPin, INPUT);

    // Flow sensor setup
    pinMode(flowSensorPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulses, RISING);

    Serial.begin(115200);
}

// Function to read temperature from LM35
float getTemperature() {
    int analogValue = analogRead(tempPin);
    float millivolts = (analogValue / 4095.0) * 3300;
    return millivolts / 10;
}

void calculateFlowRate() {
    unsigned long currentMillis = millis();
    unsigned long timeInterval = currentMillis - lastMillis;

    if (timeInterval > 1000) {
        flowRate = (pulseCount / 7.5); // Calculate flow rate in liters/min (adjust factor based on sensor specs)
        totalVolume += (flowRate / 60); // Add to total volume in liters
        pulseCount = 0; // Reset pulse count
        lastMillis = currentMillis;
    }
}

void loop() {
    Blynk.run();

    // Measure distance using Ultrasonic Sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * 0.034 / 2;

    // Read motion state and temperature
    motionState = digitalRead(pirPin);
    float temperature = getTemperature();

    // Calculate water flow rate
    calculateFlowRate();

    // Display data on LCD
    lcd.setCursor(0, 0);
    lcd.print("Water Level: ");
    lcd.setCursor(12, 0);
    lcd.print(distanceCm);
    lcd.print("cm ");

    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Flow: ");
    lcd.print(flowRate);
    lcd.print(" L/m");

    lcd.setCursor(0, 3);
    lcd.print("Volume: ");
    lcd.print(totalVolume);
    lcd.print(" L  ");

    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");

    // LED and Buzzer Logic based on water level
    if (distanceCm > safeLevel) {
        digitalWrite(greenLEDPin, HIGH);
        digitalWrite(orangeLEDPin, LOW);
        digitalWrite(redLEDPin, LOW);
        digitalWrite(buzzerPin, LOW);

        Blynk.virtualWrite(V1, 255);
        Blynk.virtualWrite(V2, 0);
        Blynk.virtualWrite(V3, 0);
    } else if (distanceCm > warningLevel) {
        digitalWrite(greenLEDPin, LOW);
        digitalWrite(orangeLEDPin, HIGH);
        digitalWrite(redLEDPin, LOW);
        digitalWrite(buzzerPin, LOW);

        Blynk.virtualWrite(V1, 0);
        Blynk.virtualWrite(V2, 255);
        Blynk.virtualWrite(V3, 0);
    } else {
        digitalWrite(greenLEDPin, LOW);
        digitalWrite(orangeLEDPin, LOW);
        digitalWrite(redLEDPin, HIGH);
        digitalWrite(buzzerPin, HIGH);

        Blynk.virtualWrite(V1, 0);
        Blynk.virtualWrite(V2, 0);
        Blynk.virtualWrite(V3, 255);
    }

    // Send sensor data to Blynk
    Blynk.virtualWrite(V0, distanceCm); // Distance
    Blynk.virtualWrite(V4, temperature); // Temperature
    Blynk.virtualWrite(V5, motionState); // Motion state
    Blynk.virtualWrite(V6, flowRate); // Flow rate
    Blynk.virtualWrite(V7, totalVolume); // Total volume

    delay(1000);
}
