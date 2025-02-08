//Some codes were authored by me and I didn't remember all licensing info for codes I pulled. Probably MIT and 
//others
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "OTA.h"

#define KEYPAD_TX 0  // IO0 on green wire 
#define KEYPAD_RX 3  // IO3 on white wire
#define alarm_input 2 // IO2
#define bit_time 434  //434.7 us for 2300 baud
#define esp8266_id "ademco_alarm_keypad"

const char* ssid = "xxxxx";
const char* password = "xxxxx";

// Baud rate settings
const int baudRate = 2300;
const int bitDuration = 1000000/baudRate; // Bit duration in microseconds
bool tx_ok=false;

const uint32_t keyEncoding[10] = {
    0b111111101111111011111110, // 0
    0b101111001011110010111100, // 1
    0b110111001101110011011100, // 2
    0b100111101001111010011110, // 3
    0b111011001110110011101100, // 4
    0b101011101010111010101110, // 5
    0b110011101100111011001110, // 6
    0b100011001000110010001100, // 7
    0b111101001111010011110100, // 8
    0b101101101011011010110110  // 9
};

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP8266 Button Test</title>
        <style>
            button {
                font-size: 32px; /* Twice the original size */
                padding: 20px 40px; /* Increase padding for a larger button */
                margin: 5px; /* Add some space between buttons */
            }
        </style>
        <script>
            function sendMessage(msg) {
                var xhttp = new XMLHttpRequest();
                xhttp.open("GET", "/click?msg=" + msg, true);
                xhttp.send();
            }
        </script>
    </head>
    <body>
        <h2>ESP8266 Ademco 6148 Keypad Emulator</h2>
        <button onclick="sendMessage('Button1')">OFF</button>
        <button onclick="sendMessage('Button2')">AWAY</button>
        <button onclick="sendMessage('Button3')">STAY</button>
        <button onclick="sendMessage('Button4')">CHIME</button>
        <button onclick="sendMessage('Button5')">5</button>
        <button onclick="sendMessage('Button6')">6</button>
        <button onclick="sendMessage('Button7')">7</button>
        <button onclick="sendMessage('Button8')">8</button>
        <button onclick="sendMessage('Button9')">9</button>
        <button onclick="sendMessage('Button0')">0</button>
    </body>
    </html>
  )rawliteral");
}

void handleButtonClick() {
  char mychar;
  if (server.hasArg("msg")) {
    String msg = server.arg("msg");
    Serial.println();
    Serial.println(msg); //msg is ButtonX
    mychar = msg[6];
    switch (mychar) {
      case ('1'): //OFF
        sendKeypress("x"); //replace x with your key
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("1");
        break;

      case ('2'): //AWAY
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("2");
        break;

      case ('3'): //STAY
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("3");
        break;

      case ('4'): //CHIME
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("x");
        sendKeypress("9");
        break;

      case ('5'):
        sendKeypress("5");
        break;

      case ('6'):
        sendKeypress("6");
        break;

      case ('7'):
        sendKeypress("7");
        break;

      case ('8'):
        sendKeypress("8");
        break;

      case ('9'):
        sendKeypress("9");
        break;

      case ('0'):
        sendKeypress("0");
        break;

      default:
        break;
    }          
    server.send(200, "text/plain", "Received: " + msg);
    } else {
        server.send(400, "text/plain", "Bad Request");
      }
}

// Idle state set to 0
void setIdleState() {
    digitalWrite(KEYPAD_TX, LOW);  // Force idle state low
}

void setup() {
    Serial.begin(115200);
    pinMode(KEYPAD_TX, OUTPUT);
    pinMode(KEYPAD_RX, INPUT_PULLUP);
    pinMode(alarm_input, INPUT_PULLUP);
    setIdleState();  // Ensure idle state is 0    
    WiFi.begin(ssid, password);
    Serial.println();
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println();
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname(esp8266_id);
    setupOTA();

    server.on("/", handleRoot);
    server.on("/click", handleButtonClick);

    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Ademco 6148 Emulator Initialized!");    
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}

// Function to send a simulated keypress to the panel
void sendKeypress(String keys) {
  char key = keys[0];
  uint32_t bitPattern = keyEncoding[key - '0'];
  tx_ok = false; //reseting flag
  while (!tx_ok) { //Re-transmit until tx_ok
    int response = transmitBitPattern(bitPattern);
    if (response != 1) {
      Serial.print("'");
      Serial.print(key);
      Serial.print("' sent, result: ");
      Serial.println(response);
    }
//    delay(1000); //slow down for debugging
  }
}

int transmitBitPattern(uint32_t pattern) {
  bool tx_window;
 
//  Serial.println("Entering tx function");    
  tx_window = digitalRead(KEYPAD_RX);
  if (tx_window) {
    delay(4); // Wait for 4ms after detecting edge
    tx_window = digitalRead(KEYPAD_RX); // Check if tx window is still open after 4ms
    if (tx_window) { // if tx window is still open after 4ms, start tx
//    Serial.print(pattern, BIN); // Check bit patterns
      for (int i = 23; i >= 0; i--) {
        if (digitalRead(KEYPAD_RX) == LOW) { // Check if tx window is closed during transmission
          digitalWrite(KEYPAD_TX, LOW); // Set output to 0 if window is closed
          Serial.print(" abort ");
          return 0; // Abort tx and re-transmit
        }        
        digitalWrite(KEYPAD_TX, (pattern >> i) & 0x01); //Load pattern and bit banging
        delayMicroseconds(bitDuration);
      }
    } else { // tx window is closed after 4ms, so exit and start re-transmit
        return 1;
      }        
  } else {
      return 1; //tx window is closed
    }

  digitalWrite(KEYPAD_TX, LOW); // Setting output=0 after transmission
  tx_ok = true;
  Serial.print(" ok ");
  return 2;
}