#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int BUTTON_ENCODE = 7;
const int BUTTON_CLEAR = 8;
const int LED_PIN = 13;

// Button input (ms) [Non-standard time, for optimal UX]
const int DOT_MAX = 100;
const int DASH_MIN = 300;
const int SAME_LETTER = 500;
const int NEXT_LETTER = 1000;
const int NEXT_WORD = 5000;
const int END_MESSAGE = 10000;

// Morse output (ms) [Standard time for LED flash]
const int UNIT = 200;
const int MORSE_DOT = UNIT;
const int MORSE_DASH = UNIT * 3;
const int MORSE_SYMBOL_GAP = UNIT;
const int MORSE_LETTER_GAP = UNIT * 3;
const int MORSE_WORD_GAP = UNIT * 7;

const char* morseTable[] = {
  ".-",   // A
  "-...", // B
  "-.-.", // C
  "-..",  // D
  ".",    // E
  "..-.", // F
  "--.",  // G
  "....", // H
  "..",   // I
  ".---", // J
  "-.-",  // K
  ".-..", // L
  "--",   // M
  "-.",   // N
  "---",  // O
  ".--.", // P
  "--.-", // Q
  ".-.",  // R
  "...",  // S
  "-",    // T
  "..-",  // U
  "...-", // V
  ".--",  // W
  "-..-", // X
  "-.--", // Y
  "--.."  // Z
};

// State variables
int charCount = 0;
int cursorRow = 0;
int cursorCol = 0;
bool screenFull = false;
bool messageDone = false;
bool messageEnded = false;
bool firstChar = true;

unsigned long buttonPressTime = 0;
unsigned long buttonReleaseTime = 0;
bool buttonWasPressed = false;
bool waitingForLetter = false;
unsigned long clearPressTime = 0;
bool clearWasPressed = false;

String currentMorse = "";

// Decode Morse to character
char decodeMorse(String morse) {
  for (int i = 0; i < 26; i++) {
    if (morse == morseTable[i]) {
      return 'A' + i;
    }
  }
  return '\0';
}

// Add character to LCD with row wrapping
void addCharToDisplay(char c) {
  if (firstChar) {
    lcd.clear();
    lcd.setCursor(0, 0);
    firstChar = false;
  }

  if (charCount >= 32) {
    screenFull = true;
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
    return;
  }

  lcd.setCursor(cursorCol, cursorRow);
  lcd.print(c);
  charCount++;
  cursorCol++;

  if (cursorCol >= 16) {
    cursorCol = 0;
    cursorRow = 1;
  }
}

// Full Erase/Clear function [long press]
void clearDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  charCount = 0;
  cursorRow = 0;
  cursorCol = 0;
  screenFull = false;
  messageDone = false;
  messageEnded = false;
  firstChar = true;
  currentMorse = "";
  waitingForLetter = false;
  digitalWrite(LED_PIN, LOW);
}

// Backspace function [short press]
void backspace() {
  if (charCount <= 0) return;
  
  cursorCol--;
  if (cursorCol < 0) {
    cursorCol = 15;
    cursorRow = 0;
  }
  
  lcd.setCursor(cursorCol, cursorRow);
  
  lcd.print(" ");
  lcd.setCursor(cursorCol, cursorRow);
  charCount--;
  screenFull = false;
}

// Commit current Morse sequence as a letter
void commitLetter() {
  if (currentMorse.length() > 0) {
    char decoded = decodeMorse(currentMorse);
    if (decoded != '\0') {
      addCharToDisplay(decoded);
    }
    currentMorse = "";
  }
}

// LED flash (encoding)
void flashSymbol(char symbol) {
  digitalWrite(LED_PIN, HIGH);
  if (symbol == '.') {
    delay(MORSE_DOT);
  } else if (symbol == '-') {
    delay(MORSE_DASH);
  }
  digitalWrite(LED_PIN, LOW);
  delay(MORSE_SYMBOL_GAP);
}

// LED flash (Serial Monitor)
void flashMorse(String word) {
  word.toUpperCase();
  for (int i = 0; i < word.length(); i++) {
    char c = word.charAt(i);
    if (c == ' ') {
      delay(MORSE_WORD_GAP);
      continue;
    }
    if (c < 'A' || c > 'Z') continue;
    String morse = morseTable[c - 'A'];
    for (int j = 0; j < morse.length(); j++) {
      flashSymbol(morse.charAt(j));
    }
    delay(MORSE_LETTER_GAP);
  }
}

// Serial Monitor input
void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;
    input.toUpperCase();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(input);
    delay(3000);

    lcd.clear();

    flashMorse(input);
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("...");
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_ENCODE, INPUT);
  pinMode(BUTTON_CLEAR, INPUT);
}

void loop() {
  // Serial Monitor input
  handleSerialInput();

  // Clear button mechanisms
  bool clearPressed = digitalRead(BUTTON_CLEAR) == HIGH;
  
  if (clearPressed && !clearWasPressed) {
    clearPressTime = millis();
    clearWasPressed = true;
  }
  
  if (!clearPressed && clearWasPressed) {
    unsigned long clearHold = millis() - clearPressTime;
    clearWasPressed = false;
    
    if (clearHold <= DASH_MIN) {
      // Backspace [short press]
      backspace();
      if (messageEnded) {
        messageEnded = false;
        messageDone = false;
        waitingForLetter = false;
      }
    } else if (clearHold >= DASH_MIN) {
      // Clear [long press]
      clearDisplay();
      lcd.setCursor(0, 0);
      lcd.print("...");
    }
  }

  // Button press [Encoding]
  bool buttonPressed = digitalRead(BUTTON_ENCODE) == HIGH;
  if (messageEnded) {
    buttonPressed = false;
  }
  
  // Button just pressed
  if (buttonPressed && !buttonWasPressed) {
    buttonPressTime = millis();
    buttonWasPressed = true;
    waitingForLetter = false;
    messageDone = false;
    digitalWrite(LED_PIN, HIGH);
  }

  // Button just released
  if (!buttonPressed && buttonWasPressed) {
    unsigned long holdDuration = millis() - buttonPressTime;
    buttonReleaseTime = millis();
    buttonWasPressed = false;
    waitingForLetter = true;
    digitalWrite(LED_PIN, LOW);

    if (holdDuration <= DOT_MAX) {
      currentMorse += ".";
    } else if (holdDuration >= DASH_MIN) {
      currentMorse += "-";
    }
  }

  // Gap detection
  if (waitingForLetter && !buttonWasPressed) {
    unsigned long gap = millis() - buttonReleaseTime;
    
    if (gap >= END_MESSAGE && messageDone && !messageEnded) {
      backspace();
      addCharToDisplay(';');
      messageEnded = true;
	}

    else if (gap >= NEXT_WORD && !messageDone) {
      commitLetter();
      addCharToDisplay('_');
      messageDone = true;
    }
    else if (gap >= NEXT_LETTER && !messageDone) {
      commitLetter();
    }
  }
}