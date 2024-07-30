#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <LiquidCrystal.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

const int joystickVert = 26;
const int joystickHorz = 27;
const int joystickSel = 28;

const int buzzerPin = 10;

int score = 0;
int highScore = 0;

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello, Raspberry Pi Pico W!");

  if(!display.begin(0x3C, OLED_RESET)) {
    Serial1.println(F("SH1107 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  lcd.begin(16, 2);
  lcd.print("Welcome!");

  pinMode(joystickVert, INPUT);
  pinMode(joystickHorz, INPUT);
  pinMode(joystickSel, INPUT_PULLUP);

  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);
