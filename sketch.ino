#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

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
int direction = 3;

unsigned long lastMoveTime = 0;
const unsigned long moveInterval = 100;
int score = 0;
int highScore = 0;
bool isPaused = false;

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello, Raspberry Pi Pico W!");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
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
  printWelcomeMessage();
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime > moveInterval && !isPaused) {
    lastMoveTime = currentTime;
    int vert = analogRead(joystickVert);
    int horz = analogRead(joystickHorz);
    int sel = digitalRead(joystickSel);

    if (vert > center + threshold && direction != 1) {
      direction = 0;
    } else if (vert < center - threshold && direction != 0) {
      direction = 1;
    } else if (horz > center + threshold && direction != 2) {
      direction = 3;
    } else if (horz < center - threshold && direction != 3) {
      direction = 2;
    }

    if (sel == LOW) {
      isPaused = !isPaused;
      delay(300); // debounce
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
      score += 10;
      if (score > highScore) {
        highScore = score;
      }
    }

    displayGame();
  }
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
    }
  }
  return false;
}

void gameOver() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  restartGame();
}

void generateFood() {
  bool validPosition = false;
  while (!validPosition) {
    validPosition = true;
    foodX = random(0, SCREEN_WIDTH / GRID_SIZE);
    foodY = random(0, SCREEN_HEIGHT / GRID_SIZE);
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        validPosition = false;
        break;
      }
    }
  }
}

void displayGame() {
  display.clearDisplay();
  display.fillRect(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }
  displayScore();
  display.display();
}

void printWelcomeMessage() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Welcome to Snake!");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
}

void restartGame() {
  snakeLength = 5;
  direction = 3;
  score = 0;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = snakeLength - i - 1;
    snakeY[i] = 0;
  }
  generateFood();
}

void displayScore() {
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 10);
  display.print("High Score: ");
  display.print(highScore);
  display.display();
}

void changeGameSpeed(int newSpeed) {
  unsigned long newMoveInterval = newSpeed;
  if (newMoveInterval < 20) {
    newMoveInterval = 20;
  }
  moveInterval = newMoveInterval;
}

void increaseDifficulty() {
  int difficultyLevel = score / 50;
  changeGameSpeed(100 - (difficultyLevel * 10));
}

void pauseGame() {
  if (isPaused) {
    display.setCursor(0, 20);
    display.print("Game Paused");
    display.display();
  }
}
