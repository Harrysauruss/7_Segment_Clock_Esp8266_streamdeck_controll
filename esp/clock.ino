#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <TimeLib.h>
#include <DNSServer.h>

// Network credentials
const char* ssid = "";    // Your WiFi name
const char* password = ""; // Your WiFi password

// LED Configuration
#define LED_PIN D6
#define LED_COUNT 86
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 50

// Global variables
CRGB leds[LED_COUNT];
ESP8266WebServer server(80);
int currentHour = 12;
int currentMinute = 0;
CRGB displayColor = CRGB::White;
bool autoUpdateEnabled = false;
int secondCounter = 0;

// Define segment patterns for 0-9
const bool digits[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}  // 9
};

void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    
    // Set first LED to red to indicate WiFi connecting
    leds[0] = CRGB::Red;
    FastLED.show();

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting to WiFi");

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        leds[0] = CRGB::Green;
        FastLED.show();
    } else {
        Serial.println("\nFailed to connect. Starting AP Mode...");
        leds[0] = CRGB::Blue;
        FastLED.show();
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP("CLOCK-SETUP", "12345678");
        
        Serial.println("AP Started");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
    }

    // Setup web server
    server.on("/", HTTP_GET, handleRoot);
    server.on("/setTime", HTTP_GET, handleSetTime);
    server.begin();
    Serial.println("HTTP server started");
}
void loop() {
    server.handleClient();
    EVERY_N_SECONDS(1) {
        secondCounter++;
        if (secondCounter >= 60) {
            secondCounter = 0;
            currentMinute++;
            if (currentMinute >= 60) {
                currentMinute = 0;
                currentHour++;
                if (currentHour >= 24) {
                    currentHour = 0;
                }
            }
        }
    }
    updateDisplay();
    FastLED.show();
}

void handleRoot() {
    String html = F("<!DOCTYPE html>"
                   "<html>"
                   "<head>"
                   "<meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>LED Clock Setup</title>"
                   "<style>"
                   "body { margin: 20px; font-family: sans-serif; }"
                   "input { display: block; margin: 10px 0; }"
                   "button { margin-top: 10px; }"
                   "</style>"
                   "</head>"
                   "<body>"
                   "<h1>LED Clock Setup</h1>"
                   "<form action='/setTime' method='get'>"
                   "<label>Hour (0-23):"
                   "<input type='number' name='hour' min='0' max='23' required value='12'>"
                   "</label>"
                   "<label>Minute (0-59):"
                   "<input type='number' name='minute' min='0' max='59' required value='0'>"
                   "</label>"
                   "<label>Color:"
                   "<input type='color' name='color' value='#ffffff'>"
                   "</label>"
                   "<label>Brightness:"
                   "<input type='range' name='brightness' min='0' max='255' value='50'>"
                   "</label>"
                   "<button type='submit'>Update Clock</button>"
                   "</form>"
                   "</body>"
                   "</html>");

    server.send(200, "text/html", html);
}

void handleSetTime() {
    Serial.println("Received setTime request");
    
    if (server.hasArg("hour") && server.hasArg("minute")) {
        currentHour = server.arg("hour").toInt();
        currentMinute = server.arg("minute").toInt();
        
        Serial.print("Setting time to: ");
        Serial.print(currentHour);
        Serial.print(":");
        Serial.println(currentMinute);
        
        if (server.hasArg("color")) {
            String colorStr = server.arg("color");
            Serial.print("Color: ");
            Serial.println(colorStr);
            
            long number = strtol(colorStr.substring(1).c_str(), NULL, 16);
            uint8_t r = (number >> 16) & 0xFF;
            uint8_t g = (number >> 8) & 0xFF;
            uint8_t b = number & 0xFF;
            displayColor = CRGB(r, g, b);
        }
        
        if (server.hasArg("brightness")) {
            int brightness = server.arg("brightness").toInt();
            Serial.print("Brightness: ");
            Serial.println(brightness);
            FastLED.setBrightness(brightness);
        }
        
        updateDisplay();
        FastLED.show();
        
        server.sendHeader("Location", "/");
        server.send(303);
    } else {
        server.send(400, "text/plain", "Error: Need hour and minute");
    }
}

void updateDisplay() {
    FastLED.clear();
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);
    displayDigit(0, currentHour / 10);
    displayDigit(1, currentHour % 10);
    
    displayDigit(2, currentMinute / 10);
    displayDigit(3, currentMinute % 10);
    
    int dotPos1 = 42;
    int dotPos2 = 43;
    leds[dotPos1] = displayColor;
    leds[dotPos2] = displayColor;
}

void displayDigit(int position, int number) {
    int startLed = position * 21;
    
    for (int segment = 0; segment < 7; segment++) {
        if (digits[number][segment]) {
            for (int led = 0; led < 3; led++) {
                int ledIndex = startLed + (segment * 3) + led;
                if(ledIndex < LED_COUNT) {
                    leds[ledIndex] = displayColor;
                }
            }
        }
    }
}