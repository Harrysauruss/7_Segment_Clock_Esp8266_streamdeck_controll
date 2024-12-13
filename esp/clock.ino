#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// LED Configuration
#define LED_PIN 5
#define LED_COUNT 86
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Network configuration
const byte DNS_PORT = 53;
const char* AP_NAME = "LED-Clock-Setup";
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

// Global variables
CRGB leds[LED_COUNT];
int currentHour = 12;
int currentMinute = 45;
CRGB displayColor = CRGB::Red;
bool dotsOn = true;
uint8_t brightness = 255;

// EEPROM configuration
struct Settings {
    bool configured;
    char ssid[32];
    char password[64];
    uint8_t brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} settings;

// Transition variables
bool isTransitioning = false;
unsigned long transitionStartTime = 0;
unsigned long transitionDuration = 0;
int targetHour = 0;
int targetMinute = 0;
int startHour = 0;
int startMinute = 0;

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

// Segment to LED mapping array
const int segmentStart[4][7] = {
    {0, 3, 6, 9, 12, 15, 18},    // Digit 0
    {21, 24, 27, 30, 33, 36, 39}, // Digit 1
    {44, 47, 50, 53, 56, 59, 62}, // Digit 2
    {65, 68, 71, 74, 77, 80, 83}  // Digit 3
};

// HTML templates
const char* setupPage = R"(
<!DOCTYPE html>
<html>
<head>
    <title>LED Clock Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        .form-group { margin-bottom: 15px; }
        input { width: 100%; padding: 5px; margin-top: 5px; }
        button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; }
    </style>
</head>
<body>
    <h2>LED Clock WiFi Setup</h2>
    <form action="/save-wifi" method="POST">
        <div class="form-group">
            <label>SSID:</label>
            <input type="text" name="ssid" required>
        </div>
        <div class="form-group">
            <label>Password:</label>
            <input type="password" name="password" required>
        </div>
        <button type="submit">Save</button>
    </form>
</body>
</html>
)";

const char* controlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>LED Clock Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        .control-group { margin-bottom: 20px; padding: 15px; border: 1px solid #ddd; }
        input { margin: 5px; }
        button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; margin: 5px; }
    </style>
</head>
<body>
    <h2>LED Clock Control</h2>
    
    <div class="control-group">
        <h3>Brightness & Color</h3>
        <input type="range" id="brightness" min="0" max="255" value="255">
        <input type="color" id="color" value="#FF0000">
        <button onclick="applySettings()">Apply</button>
    </div>

    <div class="control-group">
        <h3>Set Time</h3>
        <input type="time" id="timeInput">
        <button onclick="setTime()">Set</button>
    </div>

    <div class="control-group">
        <h3>Transition</h3>
        <input type="time" id="targetTime">
        <input type="number" id="duration" placeholder="Duration (ms)" value="5000">
        <button onclick="startTransition()">Start</button>
    </div>

    <script>
        function updateBrightness() {
            const brightness = document.getElementById("brightness").value;
            fetch("/api/brightness", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({brightness: parseInt(brightness)})
            });
        }

        function updateColor() {
            const colorHex = document.getElementById("color").value;
            const r = parseInt(colorHex.substr(1,2), 16);
            const g = parseInt(colorHex.substr(3,2), 16);
            const b = parseInt(colorHex.substr(5,2), 16);
            fetch("/api/color", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({r, g, b})
            });
        }

        function setTime() {
            const time = document.getElementById("timeInput").value;
            const [hours, minutes] = time.split(":");
            fetch("/api/time", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({
                    hour: parseInt(hours),
                    minute: parseInt(minutes)
                })
            });
        }

        function startTransition() {
            const time = document.getElementById("targetTime").value;
            const [hours, minutes] = time.split(":");
            const duration = document.getElementById("duration").value;
            fetch("/api/transition", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({
                    targetHour: parseInt(hours),
                    targetMinute: parseInt(minutes),
                    duration: parseInt(duration)
                })
            });
        }

        function applySettings() {
            updateBrightness();
            updateColor();
        }

        // Add event listeners for real-time updates
        document.getElementById("brightness").addEventListener("change", updateBrightness);
        document.getElementById("color").addEventListener("change", updateColor);
    </script>
</body>
</html>
)rawliteral";


void setup() {
    Serial.begin(115200);
    
    // Initialize EEPROM
    EEPROM.begin(sizeof(Settings));
    loadSettings();

    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(settings.brightness);
    displayColor = CRGB(settings.red, settings.green, settings.blue);
    
    // Check if WiFi is configured
    if (!settings.configured) {
        setupAccessPoint();
    } else {
        connectToWiFi();
    }

    // Setup web server routes
    setupWebServer();
    
    FastLED.clear();
    updateDisplay();
    FastLED.show();
}

void loadSettings() {
    EEPROM.get(0, settings);
    if (settings.brightness == 0) {
        settings.brightness = 255;
        settings.red = 255;
        settings.green = 0;
        settings.blue = 0;
    }
}

void saveSettings() {
    EEPROM.put(0, settings);
    EEPROM.commit();
}

void setupAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_NAME);
    
    dnsServer.start(DNS_PORT, "*", apIP);
}

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.ssid, settings.password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        setupAccessPoint();
    }
}

void setupWebServer() {
    if (!settings.configured) {
        webServer.on("/", HTTP_GET, []() {
            webServer.send(200, "text/html", setupPage);
        });
        
        webServer.on("/save-wifi", HTTP_POST, handleWiFiSetup);
    } else {
        webServer.on("/", HTTP_GET, []() {
            webServer.send(200, "text/html", controlPage);
        });
        
        // Web interface routes
        webServer.on("/update-settings", HTTP_POST, handleSettings);
        webServer.on("/set-time", HTTP_POST, handleTimeSet);
        webServer.on("/transition", HTTP_POST, handleTransition);

        // API routes
        webServer.on("/api/status", HTTP_GET, handleGetStatus);
        webServer.on("/api/settings", HTTP_GET, handleGetSettings);
        webServer.on("/api/settings", HTTP_POST, handleApiSettings);
        webServer.on("/api/time", HTTP_GET, handleGetTime);
        webServer.on("/api/time", HTTP_POST, handleApiSetTime);
        webServer.on("/api/transition", HTTP_POST, handleApiTransition);
        webServer.on("/api/brightness", HTTP_POST, handleApiBrightness);
        webServer.on("/api/color", HTTP_POST, handleApiColor);
    }
    
    webServer.begin();
}

void handleWiFiSetup() {
    if (webServer.hasArg("ssid") && webServer.hasArg("password")) {
        strncpy(settings.ssid, webServer.arg("ssid").c_str(), sizeof(settings.ssid));
        strncpy(settings.password, webServer.arg("password").c_str(), sizeof(settings.password));
        settings.configured = true;
        saveSettings();
        
        webServer.send(200, "text/plain", "WiFi settings saved. Rebooting...");
        delay(2000);
        ESP.restart();
    }
}

void handleGetStatus() {
    StaticJsonDocument<200> doc;
    doc["configured"] = settings.configured;
    doc["brightness"] = settings.brightness;
    doc["color"]["r"] = settings.red;
    doc["color"]["g"] = settings.green;
    doc["color"]["b"] = settings.blue;
    doc["currentHour"] = currentHour;
    doc["currentMinute"] = currentMinute;
    doc["isTransitioning"] = isTransitioning;

    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void handleGetSettings() {
    StaticJsonDocument<200> doc;
    doc["brightness"] = settings.brightness;
    doc["color"]["r"] = settings.red;
    doc["color"]["g"] = settings.green;
    doc["color"]["b"] = settings.blue;

    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void handleApiSettings() {
    if (webServer.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
        
        if (!error) {
            if (doc.containsKey("brightness")) {
                settings.brightness = doc["brightness"];
                FastLED.setBrightness(settings.brightness);
            }
            
            if (doc.containsKey("color")) {
                settings.red = doc["color"]["r"];
                settings.green = doc["color"]["g"];
                settings.blue = doc["color"]["b"];
                displayColor = CRGB(settings.red, settings.green, settings.blue);
            }
            
            saveSettings();
            updateDisplay();
            FastLED.show();
            webServer.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }
    }
}

void handleApiSetTime() {
    if (webServer.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
        
        if (!error && doc.containsKey("hour") && doc.containsKey("minute")) {
            int hour = doc["hour"];
            int minute = doc["minute"];
            
            if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60) {
                currentHour = hour;
                currentMinute = minute;
                updateDisplay();
                FastLED.show();
                webServer.send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                webServer.send(400, "application/json", "{\"error\":\"Invalid time values\"}");
            }
        }
    }
}

void handleApiTransition() {
    if (webServer.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
        
        if (!error && doc.containsKey("targetHour") && doc.containsKey("targetMinute") && doc.containsKey("duration")) {
            int targetHour = doc["targetHour"];
            int targetMinute = doc["targetMinute"];
            unsigned long duration = doc["duration"];
            
            if (targetHour >= 0 && targetHour < 24 && targetMinute >= 0 && targetMinute < 60) {
                transitionToTime(targetHour, targetMinute, duration);
                webServer.send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                webServer.send(400, "application/json", "{\"error\":\"Invalid time values\"}");
            }
        }
    }
}

void handleApiBrightness() {
    if (webServer.hasArg("plain")) {
        StaticJsonDocument<50> doc;
        DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
        
        if (!error && doc.containsKey("brightness")) {
            settings.brightness = doc["brightness"];
            FastLED.setBrightness(settings.brightness);
            saveSettings();
            updateDisplay();
            FastLED.show();
            webServer.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            webServer.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
        }
    }
}

void handleApiColor() {
    if (webServer.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
        
        if (!error && doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
            settings.red = doc["r"];
            settings.green = doc["g"];
            settings.blue = doc["b"];
            displayColor = CRGB(settings.red, settings.green, settings.blue);
            saveSettings();
            updateDisplay();
            FastLED.show();
            webServer.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            webServer.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
        }
    }
}

void handleGetTime() {
    StaticJsonDocument<100> doc;
    doc["hour"] = currentHour;
    doc["minute"] = currentMinute;

    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void handleSettings() {
    handleApiSettings();
}

void handleTimeSet() {
    handleApiSetTime();
}

void handleTransition() {
    handleApiTransition();
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
    
    // Calculate progress (0.0 to 1.0)
    float progress = (float)elapsedTime / transitionDuration;
    
    // Convert start and target times to minutes for easier interpolation
    int startTotalMinutes = startHour * 60 + startMinute;
    int targetTotalMinutes = targetHour * 60 + targetMinute;
    
    // Handle wraparound (e.g., transitioning from 23:59 to 00:01)
    if (targetTotalMinutes < startTotalMinutes) {
        targetTotalMinutes += 24 * 60;
    }
    
    // Calculate current total minutes
    int currentTotalMinutes = startTotalMinutes + ((targetTotalMinutes - startTotalMinutes) * progress);
    
    // Handle wraparound for final values
    if (currentTotalMinutes >= 24 * 60) {
        currentTotalMinutes -= 24 * 60;
    }
    
    // Convert back to hours and minutes
    currentHour = currentTotalMinutes / 60;
    currentMinute = currentTotalMinutes % 60;
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

void loop() {
    if (!settings.configured) {
        dnsServer.processNextRequest();
    }
    webServer.handleClient();
    
    EVERY_N_MILLISECONDS(16) {  // ~60fps update rate
        if (isTransitioning) {
            updateTransition();
            updateDisplay();
            FastLED.show();
        }
    }
    
    EVERY_N_SECONDS(1) {
        if (!isTransitioning) {
            dotsOn = !dotsOn;
            updateDisplay();
            FastLED.show();
        }
    }
}