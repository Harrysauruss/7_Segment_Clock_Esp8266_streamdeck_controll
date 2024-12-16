#include <FastLED.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi Configuration
const char* ssid = "ssid";
const char* password = "password";

// Web Server on port 80
ESP8266WebServer server(80);

// LED Configuration
#define LED_PIN D6
#define LED_COUNT 86
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 255

// Global variables
CRGB leds[LED_COUNT];
int currentHour = 10;
int currentMinute = 10;
CRGB displayColor = CRGB::Red;
bool autoUpdateEnabled = false;
int secondCounter = 0;
bool dotsOn = true;

bool isTransitioning = false;
unsigned long transitionStartTime = 0;
unsigned long transitionDuration = 0;
int targetHour = 0;
int targetMinute = 0;
int startHour = 0;
int startMinute = 0;


// Smooth sine-based easing function
float easeInOutSine(float t) {
    return -(cos(PI * t) - 1) / 2;
}

// Define segment patterns for 0-9
// Segments are arranged as:
//  AAA
// F   B
// F   B
//  GGG
// E   C
// E   C
//  DDD
const bool digits[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // 0 (A,B,C,D,E,F)
    {0, 1, 1, 0, 0, 0, 0}, // 1 (B,C)
    {1, 1, 0, 1, 1, 0, 1}, // 2 (A,B,D,E,G)
    {1, 1, 1, 1, 0, 0, 1}, // 3 (A,B,C,D,G)
    {0, 1, 1, 0, 0, 1, 1}, // 4 (B,C,F,G)
    {1, 0, 1, 1, 0, 1, 1}, // 5 (A,C,D,F,G)
    {1, 0, 1, 1, 1, 1, 1}, // 6 (A,C,D,E,F,G)
    {1, 1, 1, 0, 0, 0, 0}, // 7 (A,B,C)
    {1, 1, 1, 1, 1, 1, 1}, // 8 (A,B,C,D,E,F,G)
    {1, 1, 1, 1, 0, 1, 1}  // 9 (A,B,C,D,F,G)
};

// Segment to LED mapping array
// Each segment uses 3 LEDs
const int segmentStart[4][7] = {
    // Digit 0 (leftmost)
    {0, 3, 6, 9, 12, 15, 18},
    // Digit 1
    {21, 24, 27, 30, 33, 36, 39},
    // Digit 2
    {44, 47, 50, 53, 56, 59, 62},
    // Digit 3 (rightmost)
    {65, 68, 71, 74, 77, 80, 83}
};

void handleColor() {
    String r = server.arg("r");
    String g = server.arg("g");
    String b = server.arg("b");

    Serial.println();
    Serial.print("Ip Called");

    if(r != "" && g != "" && b != "") {
        displayColor = CRGB(r.toInt(), g.toInt(), b.toInt());
        server.send(200, "text/plain", "Color updated");
    } else {
        server.send(400, "text/plain", "Missing RGB values");
    }
}

void handleTransition(){
  String h = server.arg("h");
  String m = server.arg("m");
  String t = server.arg("t");

  if(h != "" && m != "" && t != ""){
    transitionToTime(h.toInt(), m.toInt(), t.toInt());
    server.send(200, "text/plain", "transition started");
  } else{
    server.send(400, "text/plain", "Missing values");
  }
}

void handleSet(){
  String h = server.arg("h");
  String m = server.arg("m");

  if(h != "" && m != ""){
    currentHour = h.toInt();
    currentMinute = m.toInt();
    server.send(200, "text/plain", "set time");
  } else{
    server.send(400, "text/plain", "Missing values");
  }
}

void handleRoot() {
    String html = "<html><body>";
    html += "<h1>LED Clock Control</h1>";
    html += "<p>Current color: RGB(" + String(displayColor.r) + "," + String(displayColor.g) + "," + String(displayColor.b) + ")</p>";
    html += "<p>Set color using: /color?r=255&g=0&b=0</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    // Setup server endpoints
    server.on("/", handleRoot);
    server.on("/color", handleColor);
    server.on("/transition", handleTransition);
    server.on("/set", handleSet);
    server.begin();
    
    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
}

void transitionToTime(int hour, int minute, unsigned long durationMillis) {
    // Validate input
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return;
    }
    
    // Store transition parameters
    isTransitioning = true;
    transitionStartTime = millis();
    transitionDuration = durationMillis;
    targetHour = hour;
    targetMinute = minute;
    startHour = currentHour;
    startMinute = currentMinute;
}

void updateTransition() {
    if (!isTransitioning) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - transitionStartTime;
    
    if (elapsedTime >= transitionDuration) {
        // Transition complete
        currentHour = targetHour;
        currentMinute = targetMinute;
        isTransitioning = false;
        return;
    }
    
    // Calculate progress (0.0 to 1.0) with smooth sine easing
    float linearProgress = (float)elapsedTime / transitionDuration;
    float progress = easeInOutSine(linearProgress);
    
    // Convert start and target times to minutes for easier interpolation
    int startTotalMinutes = startHour * 60 + startMinute;
    int targetTotalMinutes = targetHour * 60 + targetMinute;
    
    // Handle wraparound (e.g., transitioning from 23:59 to 00:01)
    if (targetTotalMinutes < startTotalMinutes) {
        targetTotalMinutes += 24 * 60;
    }
    
    // Calculate current total minutes using eased progress
    int currentTotalMinutes = startTotalMinutes + ((targetTotalMinutes - startTotalMinutes) * progress);
    
    // Handle wraparound for final values
    if (currentTotalMinutes >= 24 * 60) {
        currentTotalMinutes -= 24 * 60;
    }
    
    // Convert back to hours and minutes
    currentHour = currentTotalMinutes / 60;
    currentMinute = currentTotalMinutes % 60;
}


void loop() {
    // Handle client requests
    server.handleClient();
    
    EVERY_N_MILLISECONDS(16) {  // ~60fps update rate
        if (isTransitioning) {
            updateTransition();
            updateDisplay();
            FastLED.show();
        }
    }
    
    EVERY_N_SECONDS(1) {
        if (!isTransitioning) {
            secondCounter++;
            dotsOn = !dotsOn;
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
}

void updateDisplay() {
    FastLED.clear();
    
    displayDigit(0, currentHour / 10);
    displayDigit(1, currentHour % 10);
    
    displayDigit(2, currentMinute / 10);
    displayDigit(3, currentMinute % 10);
    
    // Update dots based on dotsOn state
    int dotPos1 = 42;
    int dotPos2 = 43;
    if (dotsOn) {
        leds[dotPos1] = displayColor;
        leds[dotPos2] = displayColor;
    } else {
        leds[dotPos1] = CRGB::Black;
        leds[dotPos2] = CRGB::Black;
    }
}

void displayDigit(int position, int number) {
    // Check for valid input
    if (position < 0 || position > 3 || number < 0 || number > 9) {
        return;
    }
    
    // Light up appropriate segments
    for (int segment = 0; segment < 7; segment++) {
        if (digits[number][segment]) {
            int startLed = segmentStart[position][segment];
            // Light up 3 LEDs for each segment
            for (int led = 0; led < 3; led++) {
                int ledIndex = startLed + led;
                if (ledIndex < LED_COUNT) {
                    leds[ledIndex] = displayColor;
                }
            }
        }
    }
}