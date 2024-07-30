#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int joystickVert = 26;
const int joystickHorz = 27;
const int joystickSel = 28;

const int center = 511;
const int threshold = 200;

#define GRID_SIZE 4
#define MAX_SNAKE_LENGTH 128
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 5;
int foodX, foodY;
int direction = 3;  // 0 = up, 1 = dwn, 2 = LLeft, 3 = right

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello, Raspberry Pi Pico W!");

  if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial1.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  pinMode(joystickVert, INPUT);
  pinMode(joystickHorz, INPUT);
  pinMode(joystickSel, INPUT_PULLUP);

  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = snakeLength - i - 1; 
    snakeY[i] = 0;                   
  }

  generateFood();
}

void loop() {
  int vert = analogRead(joystickVert);
  int horz = analogRead(joystickHorz);
  int sel = digitalRead(joystickSel);

  if (vert > center + threshold && direction != 1) {
    direction = 0;  // Up
  } else if (vert < center - threshold && direction != 0) {
    direction = 1;  // Down
  } else if (horz > center + threshold && direction != 2) {
    direction = 3;  // Right
  } else if (horz < center - threshold && direction != 3) {
    direction = 2;  // Left
  }
  
  updateSnake();

  if (checkCollision()) {
    gameOver();
  }

  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if (snakeLength < MAX_SNAKE_LENGTH) {
      snakeLength++;
    }
    generateFood();
  }

  displayGame();

  delay(100);
}

void updateSnake() {
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  if (direction == 0) {
    snakeY[0]--;
  } else if (direction == 1) {
    snakeY[0]++;
  } else if (direction == 2) {
    snakeX[0]--;
  } else if (direction == 3) {
    snakeX[0]++;
  }

  if (snakeX[0] < 0) snakeX[0] = SCREEN_WIDTH / GRID_SIZE - 1;
  if (snakeX[0] >= SCREEN_WIDTH / GRID_SIZE) snakeX[0] = 0;
  if (snakeY[0] < 0) snakeY[0] = SCREEN_HEIGHT / GRID_SIZE - 1;
  if (snakeY[0] >= SCREEN_HEIGHT / GRID_SIZE) snakeY[0] = 0;
}

bool checkCollision() {
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
