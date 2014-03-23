#include <Bridge.h>
#include <Console.h>
#include <Process.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include "config.h"

////////////////////////////////////////////////////////////////////////////////////
// Tinamous configuation is set in config.h, see config.h.example
// copy or rename config.h.example to config.h and fill in the appropriate 
// values (Tinamous.com account name, device username and password)
// and reload this sketch for the values to take effect.
////////////////////////////////////////////////////////////////////////////////////

// Keep button on pin 2 for interrupt (on the YUN)
const int buttonPin = 2;
const int ledPin =  6;
volatile int pressed = LOW;

// Onboard LED to show running
int onBoardLedPin = 13;
boolean onBoardLedState = LOW;

// Initialize at high level so that the next temperature read 
// is soon
int loopCounter = 1180;

// read the templerature every ... times around the loop
// 250ms Loop delay. 10 minutes = 10 Minutes * 60 Seconds * 2 counts per second. = 1200
const int readTemperatureEvery = 1200; // 1200

///////////////////////////////////////////////////
// Setup
///////////////////////////////////////////////////
void setup() {
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);      

  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);  
    
  // Led Low (off) whilst initializing.  
  digitalWrite(ledPin, LOW);  
  
  Bridge.begin();
  
  // Enable debugging on USB with the serial port
  // connect the Arduino IDE to the COMx to read the serial port.
  Serial.begin(9600);
       
  // Attach to Interrupt 1 on pin 2 to be notified when button
  // on pin 2 is pressed. NB: Pin 2 is interrupt 1 on Yun, may be interrupt 0 on others.
  attachInterrupt(1, buttonPressed, LOW);
  
  // Send a message to indicate the 
  sendStatusMessage("BigGreenButton Powered up and ready for action!");
}

///////////////////////////////////////////////////
// interrupt function when the button is pressed
///////////////////////////////////////////////////
void buttonPressed() {
  pressed = true;
}

///////////////////////////////////////////////////
// Loop
///////////////////////////////////////////////////
void loop() {
  
  if (pressed) {
    sendStatusMessage("Everything OK.");
    pressed = false;
  }
  
  delay(500);

  // Flash the onboard led to show activity
  digitalWrite(onBoardLedPin, onBoardLedState);
  onBoardLedState = !onBoardLedState;
  
  loopCounter++;
  
  if (loopCounter >= readTemperatureEvery) {
     readTemperatureSensor();
     loopCounter = 0;
  }
}

///////////////////////////////////////////////////
// Read the value from the temperature sensor
// and report it to Tinamous
///////////////////////////////////////////////////
void readTemperatureSensor() {
  int temperatureRaw = analogRead(A5);
  displayMessage("TemperatureRaw: " + temperatureRaw);
  
  float voltage = temperatureRaw *  (5000/ 1024); // Temperature in Kelvin
  Serial.print("Voltage: ");
  Serial.println(voltage);
  
  // Convert ADC value to Kelvin (5V ref, 1023 Max value).
  float temperatureKelvin = (((temperatureRaw/1024.0)*5.0)*100.0);
  float temperatureCelsius = temperatureKelvin-273.0;

  String temperatureString = String(temperatureCelsius);
  displayMessage("Temperature: " + temperatureString);
    
  sendMeasurement(temperatureCelsius, temperatureRaw);
}

///////////////////////////////////////////////////
// Sed the message to Tinamous
///////////////////////////////////////////////////
void sendStatusMessage(String message) {
  showSendingLed();
  String url = STATUS_API;   

  String statusMessage = "{'Message': '";
  statusMessage+=message;
  statusMessage+="', 'Lite':'true'}";
  
  displayMessage("Sending status... ");
  sendCurlMessage(url, statusMessage);
}

///////////////////////////////////////////////////
// Send measurements to Tinamous
///////////////////////////////////////////////////
void sendMeasurement(float temperatureCelsius, int temperatureRaw) {
  // Don't show the sending light for measurements.
  
  String url = MEASUREMENTS_API;
  
  // Construct the json message to send the temperature readings.
  String temperatureCelsiusString = String(temperatureCelsius);
  String temperatureRawString = String(temperatureRaw);
  String message = "{'Channel':'0', 'Field1':'" + temperatureCelsiusString + "', 'Field2':'" + temperatureRawString + "', 'Lite':'true'}";
  
  displayMessage("Sending measurement... ");
  
  sendCurlMessage(url, message);
}

///////////////////////////////////////////////////
// Light the "Sending" LED
///////////////////////////////////////////////////
void showSendingLed() {
  digitalWrite(ledPin, HIGH);
}

///////////////////////////////////////////////////
// Send http message using Curl via Process
///////////////////////////////////////////////////
void sendCurlMessage(String url, String message) {
  String usernamePassword = USERNAME_AND_PASSWORD;

  Process process;  
  process.begin("curl");
  process.addParameter("-u");
  process.addParameter(usernamePassword);
  process.addParameter("-k"); // insecure ssl
  process.addParameter("--request");
  process.addParameter("POST");
  process.addParameter("--data");
  process.addParameter(message);
  process.addParameter("--header");
  process.addParameter("Content-Type: application/json");
  process.addParameter("--header");
  process.addParameter("Accept: application/json");
  process.addParameter("-w %{http_code}");
  process.addParameter(url);
  process.run();
  
  handleProcessCurlResult(process);
}

///////////////////////////////////////////////////
// Handle the result from the Curl Process
// Checks for a "201" in the result string
// Flashes the Sending LED 5 times if it fails.
///////////////////////////////////////////////////
void handleProcessCurlResult(Process process) {

  // If there's incoming data from the net connection,
  // send it out the Serial:
  String result = String("");

  Serial.println("Curl:"); 
  while (process.available()>0) {
    char c = process.read();
    result+=c;
    Serial.print(c);
  }
  Serial.println("--------------");
    
  // Check for the 201 created response.
  if (result.indexOf("201") > 0) {
    digitalWrite(ledPin, LOW);
  } else {
    for (int i=0; i<5; i++) {
      // Set error state
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      delay(100);
    }
  }
}

///////////////////////////////////////////////////
// Display diagnostic message
///////////////////////////////////////////////////
void displayMessage(String message) {
  Serial.println(message);
}
