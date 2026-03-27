#include <WiFi.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// --- WOKWI HARDWARE DEFINITIONS (Must match diagram.json) ---

// WiFi Credentials
const char* ssid = "Jerry";
const char* password = "Jerry123$#";

// Display Pins (ST7789 example)
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4 // Connect to -1 if not using a reset pin
#define TFT_MOSI 23
#define TFT_SCLK 18

TFT_eSPI tft = TFT_eSPI();

// Touch Screen Pins (XPT2046 example)
#define TOUCH_CS   13
#define TOUCH_IRQ  -1 // Not used in this basic setup, set to -1

XPT2046_Touchscreen ts(TOUCH_CS);

// --- API DEFINITIONS ---
// These are defined in platformio.ini build_flags, but good to have here.
// Actual key must be substituted via Wokwi secrets or in platformio.ini for initial test.
#ifndef GEMINI_API_KEY
#define GEMINI_API_KEY "AIzaSyB9-7mJS5HEs4bqlJffbRTbdXLiRGkPVqE" // Replace with your actual API key or use build_flags
#endif
#ifndef GEMINI_API_URL
#define GEMINI_API_URL "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%s"
#endif

// --- STATE VARIABLES ---
String inputBuffer = "";
String chatHistory = "System: Welcome to Gemini Chatbot!\n";
enum State { INIT, WIFI_CONNECT, LOADING, READY, CHATTING, RESPONDING };
State currentState = INIT;

// --- FUNCTION PROTOTYPES ---
void setupWifi();
void drawLoadingScreen();
void drawKeyboard();
void drawChatInterface();
void handleTouch();
void sendPromptToGemini(String prompt);
String getGeminiResponse(String prompt);
void displayText(int x, int y, String text, uint16_t color);

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1); // Adjust rotation as needed for Wokwi display
  // For ST7789, if you have issues, try commenting out tft.init and using this:
  // tft.begin();
  // tft.setSwapBytes(true); // For some ST7789 displays

  tft.fillScreen(TFT_BLACK);

  ts.begin();
  // Calibrate touch if needed
  // ts.setRotation(tft.getRotation());

  currentState = WIFI_CONNECT;
}

void loop() {
  switch (currentState) {
    case WIFI_CONNECT:
      setupWifi();
      break;
    case LOADING:
      drawLoadingScreen();
      // After loading, transition to READY or CHATTING state
      currentState = READY; // Simplification: Move directly to READY for now
      break;
    case READY:
      tft.fillScreen(TFT_BLACK);
      drawKeyboard();
      drawChatInterface(); // Show initial chat history (or blank)
      currentState = CHATTING;
      break;
    case CHATTING:
      handleTouch();
      // Input is handled by touch, state only changes when sending a prompt
      break;
    case RESPONDING:
      // State change handled by sendPromptToGemini once done
      break;
  }
}

// --- IMPLEMENTATION STUBS ---

void setupWifi() {
  // TODO: Implement robust Wi-Fi connection logic here
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    currentState = LOADING; // Move to loading screen after successful connection
  } else {
    Serial.println("\nWiFi connection failed");
    displayText(10, 10, "WiFi Failed!", TFT_RED);
    delay(5000);
    // In a real device, we might retry or enter an error state
  }
}

void drawLoadingScreen() {
  tft.fillScreen(TFT_BLUE);
  displayText(tft.width() / 2 - 50, tft.height() / 2, "Loading...", TFT_WHITE);
  // In a real app, this is where we might initialize complex graphics/libraries
  delay(2000);
}


#define KEY_W 40
#define KEY_H 30
#define KEY_SPACING_X 5
#define KEY_SPACING_Y 5
#define KEYBOARD_START_Y (tft.height() - (KEY_H * 4 + KEY_SPACING_Y * 3) - 10) // 4 rows of keys

char keyboardKeys[] = {
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '\n',
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '<',
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '.', ' ',
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
};

int keyboardKeyLayout[][3] = {
  // char, x, y, width, height
  // Row 1
  {'Q', 0, 0, 1, 1},
  {'W', 1, 0, 1, 1},
  {'E', 2, 0, 1, 1},
  {'R', 3, 0, 1, 1},
  {'T', 4, 0, 1, 1},
  {'Y', 5, 0, 1, 1},
  {'U', 6, 0, 1, 1},
  {'I', 7, 0, 1, 1},
  {'O', 8, 0, 1, 1},
  {'P', 9, 0, 1, 1},
  {'\n', 10, 0, 1, 1}, // Enter key
  // Row 2
  {'A', 0, 1, 1, 1},
  {'S', 1, 1, 1, 1},
  {'D', 2, 1, 1, 1},
  {'F', 3, 1, 1, 1},
  {'G', 4, 1, 1, 1},
  {'H', 5, 1, 1, 1},
  {'J', 6, 1, 1, 1},
  {'K', 7, 1, 1, 1},
  {'L', 8, 1, 1, 1},
  {'<', 9, 1, 1, 1}, // Backspace
  // Row 3
  {'Z', 0, 2, 1, 1},
  {'X', 1, 2, 1, 1},
  {'C', 2, 2, 1, 1},
  {'V', 3, 2, 1, 1},
  {'B', 4, 2, 1, 1},
  {'N', 5, 2, 1, 1},
  {'M', 6, 2, 1, 1},
  {'.', 7, 2, 1, 1},
  {' ', 8, 2, 2, 1}, // Spacebar
  // Row 4 (Numbers)
  {'1', 0, 3, 1, 1},
  {'2', 1, 3, 1, 1},
  {'3', 2, 3, 1, 1},
  {'4', 3, 3, 1, 1},
  {'5', 4, 3, 1, 1},
  {'6', 5, 3, 1, 1},
  {'7', 6, 3, 1, 1},
  {'8', 7, 3, 1, 1},
  {'9', 8, 3, 1, 1},
  {'0', 9, 3, 1, 1}
};

void drawKeyboard() {
  // Input box area
  int input_y = KEYBOARD_START_Y - 50 - KEY_SPACING_Y; // Above the keyboard
  tft.drawRect(5, input_y, tft.width() - 10, 40, TFT_WHITE);
  tft.setCursor(10, input_y + 10);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print(inputBuffer);

  // Keyboard drawing
  int start_x = (tft.width() - (10 * KEY_W + 9 * KEY_SPACING_X)) / 2; // Center the keyboard
  if (start_x < 0) start_x = 0; // Ensure it doesn't go off screen
  
  for (int i = 0; i < sizeof(keyboardKeyLayout) / sizeof(keyboardKeyLayout[0]); ++i) {
    char keyChar = keyboardKeyLayout[i][0];
    int col = keyboardKeyLayout[i][1];
    int row = keyboardKeyLayout[i][2];
    int keyWidth = KEY_W * keyboardKeyLayout[i][3]; // Use width multiplier
    int keyHeight = KEY_H * keyboardKeyLayout[i][4]; // Use height multiplier

    int x = start_x + col * (KEY_W + KEY_SPACING_X);
    int y = KEYBOARD_START_Y + row * (KEY_H + KEY_SPACING_Y);

    tft.fillRect(x, y, keyWidth, keyHeight, TFT_DARKGREY);
    tft.drawRect(x, y, keyWidth, keyHeight, TFT_WHITE);
    tft.setCursor(x + keyWidth / 4, y + keyHeight / 4);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);

    if (keyChar == '\n') {
      tft.print("Send");
    } else if (keyChar == '<') {
      tft.print("Del");
    } else if (keyChar == ' ') {
      tft.print(" "); // Spacebar, print a space or nothing
    }else {
      tft.print(keyChar);
    }
  }
}



#define CHAT_HISTORY_HEIGHT_RATIO 0.7
#define CHAT_TEXT_START_X 5
#define CHAT_TEXT_START_Y 5

void drawChatInterface() {
  // Clear the chat history area
  int chatAreaHeight = tft.height() * CHAT_HISTORY_HEIGHT_RATIO;
  tft.fillRect(0, 0, tft.width(), chatAreaHeight, TFT_BLACK);

  tft.setCursor(CHAT_TEXT_START_X, CHAT_TEXT_START_Y);
  tft.setTextSize(1);
  
  // Basic scrolling: just print the last part that fits
  // This is a simplification; a proper scrolling implementation would involve more state management.
  int currentY = CHAT_TEXT_START_Y;
  int lineHeight = 8; // Approx height for setTextSize(1)
  int maxLines = chatAreaHeight / lineHeight;

  // Split chatHistory into lines and only show the ones that fit
  String tempChatHistory = chatHistory;
  int lastNewline = -1;
  int currentLine = 0;
  
  for(int i = 0; i < tempChatHistory.length(); ++i) {
    if (tempChatHistory.charAt(i) == '\n') {
      currentLine++;
      lastNewline = i;
    }
  }

  int startIndex = 0;
  if (currentLine > maxLines) {
    // Find the starting index to display only the last `maxLines`
    currentLine = 0;
    for(int i = 0; i < tempChatHistory.length(); ++i) {
      if (tempChatHistory.charAt(i) == '\n') {
        currentLine++;
        if (currentLine == (currentLine - maxLines + 1)) {
          startIndex = i + 1;
          break;
        }
      }
    }
  }
  
  String displayChat = tempChatHistory.substring(startIndex);

  // Now print the visible chat history, handling different colors for System, User, Gemini
  String line;
  for (int i = 0; i < displayChat.length(); ++i) {
    line += displayChat.charAt(i);
    if (displayChat.charAt(i) == '\n' || i == displayChat.length() - 1) {
      if (line.startsWith("System:")) {
        tft.setTextColor(TFT_YELLOW);
      } else if (line.startsWith("User:")) {
        tft.setTextColor(TFT_WHITE);
      } else if (line.startsWith("Gemini:")) {
        tft.setTextColor(TFT_GREEN);
      } else {
        tft.setTextColor(TFT_LIGHTGREY);
      }
      tft.print(line);
      currentY += lineHeight;
      line = "";
      tft.setCursor(CHAT_TEXT_START_X, currentY);
    }
  }
}


void displayText(int x, int y, String text, uint16_t color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.print(text.c_str());
}

void handleTouch() {
  if (ts.touched()) {
    // Retrieve raw touch point
    TS_Point p = ts.getPoint();

    // Convert raw touch to screen coordinates (adjust as per your screen and touch controller calibration)
    // These values are highly dependent on your specific screen and its calibration.
    // For Wokwi, you might need to experiment or check Wokwi examples for the exact mapping.
    // A common transformation might involve swapping X/Y and inverting one axis.
    int x = map(p.y, 240, 0, 0, tft.width()); // Example: assuming Y raw is X screen, inverted
    int y = map(p.x, 0, 240, 0, tft.height()); // Example: assuming X raw is Y screen

    // Filter out invalid touch readings (e.g., if no pressure is detected)
    if (p.z < 100 || p.z > 1000) return; // p.z indicates pressure
    
    // Check if touch is within keyboard area
    if (y > KEYBOARD_START_Y) {
      // Iterate through keys to find which one was pressed
      int start_x = (tft.width() - (10 * KEY_W + 9 * KEY_SPACING_X)) / 2;
      if (start_x < 0) start_x = 0;

      for (int i = 0; i < sizeof(keyboardKeyLayout) / sizeof(keyboardKeyLayout[0]); ++i) {
        char keyChar = keyboardKeyLayout[i][0];
        int col = keyboardKeyLayout[i][1];
        int row = keyboardKeyLayout[i][2];
        int keyWidth = KEY_W * keyboardKeyLayout[i][3];
        int keyHeight = KEY_H * keyboardKeyLayout[i][4];

        int keyX = start_x + col * (KEY_W + KEY_SPACING_X);
        int keyY = KEYBOARD_START_Y + row * (KEY_H + KEY_SPACING_Y);

        if (x >= keyX && x < (keyX + keyWidth) && y >= keyY && y < (keyY + keyHeight)) {
          // Key pressed!
          if (keyChar == '\n') { // Enter/Send key
            sendPromptToGemini(inputBuffer); 
          } else if (keyChar == '<') { // Backspace
            if (inputBuffer.length() > 0) {
              inputBuffer.remove(inputBuffer.length() - 1);
            }
          } else { // Regular character
            inputBuffer += keyChar;
          }
          // Redraw keyboard and input box to reflect changes
          tft.fillScreen(TFT_BLACK);
          drawChatInterface();
          drawKeyboard();
          break; // Exit loop after finding the key
        }
      }
    }
    
    // Debounce: wait for touch release
    while (ts.touched()) {
      delay(10);
    }
  }
}

void sendPromptToGemini(String prompt) {
  if (prompt.length() == 0) return;

  currentState = RESPONDING;
  chatHistory += "User: " + prompt + "\n";
  
  // Redraw to show user prompt immediately
  tft.fillScreen(TFT_BLACK);
  drawChatInterface();
  drawKeyboard(); // Redraw input box/keyboard area

  // --- Show Loading ---
  tft.fillRect(tft.width()/2 - 50, tft.height()/2, 100, 30, TFT_DARKGREY);
  displayText(tft.width()/2 - 40, tft.height()/2 + 5, "AI Thinking...", TFT_WHITE);
  
  Serial.println("Sending prompt to Gemini...");
  
  String response = getGeminiResponse(prompt);
  
  chatHistory += "Gemini: " + response + "\n";

  // Clear loading indicator and redraw everything
  tft.fillScreen(TFT_BLACK);
  drawChatInterface();
  drawKeyboard(); 
  
  inputBuffer = ""; // Clear input buffer after sending
  currentState = READY; // Back to ready state to accept new input
}

String getGeminiResponse(String prompt) {
  // TODO: Implement JSON construction and HTTP POST to Gemini API
  // This requires proper JSON structure for the request body.
  // Example: {"contents":[{"parts":[{"text":"<prompt>"}]}]}
  
  HTTPClient http;
  String serverPath = String(GEMINI_API_URL_BASE) + String(GEMINI_API_KEY);
  
  String jsonPayload = "{\"contents\":[{\"parts\":[{\"text\":\"";
  jsonPayload += prompt;
  jsonPayload += "\"}]}]}";

  Serial.printf("[HTTP] POST to %s\n", serverPath.c_str());
  http.begin(serverPath);
  
  // Set required headers for JSON POST
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonPayload);
  String payload = "Error: Could not connect or API call failed.";

  if (httpResponseCode == HTTP_CODE_OK) {
    payload = http.getString();
    http.end();
  } else {
    String errorMsg = http.errorToString(httpResponseCode);
    Serial.printf("[HTTP] POST failed, error: %s\n", errorMsg.c_str());
    http.end();
    return "Connection Error: " + errorMsg;
  }
  
  // --- Simplified JSON Parsing Stub ---
  // In a real scenario, we'd need a JSON parser like ArduinoJson.
  // For a Wokwi setup, we rely on the structure.
  // Assuming the response contains "text" field in the first candidate.
  
  String resultText = "API Response Parsing Error: See Serial Output.";
  
  int start_index = payload.indexOf("\"text\": \"") + 8;
  if (start_index > 8) {
    int end_index = payload.indexOf("\"", start_index);
    if (end_index != -1) {
      resultText = payload.substring(start_index, end_index);
      // Replace escaped quotes if any, though Gemini usually returns raw text in the sample structure
      resultText.replace("\\n", "\n"); 
    }
  }
  
  Serial.printf("API Response: %s\n", resultText.c_str());
  return resultText;
}