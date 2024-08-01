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
unsigned long moveInterval = 150;
int score = 0;
int highScore = 0;
bool isPaused = false;
bool gameOverFlag = false;
bool gameStarted = false;
int obstacleX[10], obstacleY[10];
int numObstacles = 5;
int movingObstacleX, movingObstacleY;
unsigned long lastObstacleMoveTime = 0;
unsigned long obstacleMoveInterval = 1000;

int powerUpX, powerUpY;
bool powerUpActive = false;
unsigned long lastPowerUpTime = 0;
unsigned long powerUpDuration = 10000;

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

  resetGame();
  showStartScreen();
}

void loop() {
  if (!gameOverFlag) {
    unsigned long currentTime = millis();
    if (currentTime - lastMoveTime > moveInterval && !isPaused) {
      lastMoveTime = currentTime;

      if (!gameStarted) {
        if (digitalRead(joystickSel) == LOW) {
          gameStarted = true;
          display.clearDisplay();
          display.display();
          delay(500);
        }
      } else {
        handleJoystickInput();
        if (!isPaused) {
          updateSnake();
          if (checkCollision() || checkObstacleCollision()) {
            gameOver();
          }
          if (snakeX[0] == foodX && snakeY[0] == foodY) {
            handleFoodConsumption();
          }
          moveObstacles(currentTime);
          if (powerUpActive && currentTime - lastPowerUpTime > powerUpDuration) {
            powerUpActive = false;
            increaseDifficulty();
          }
          if (snakeX[0] == powerUpX && snakeY[0] == powerUpY) {
            handlePowerUp();
          }
          displayGame();
        } else {
          pauseGame();
        }
      }
    }
  }
}

void handleJoystickInput() {
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
    delay(300);
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

  wrapAroundScreen();
}

void wrapAroundScreen() {
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

bool checkObstacleCollision() {
  for (int i = 0; i < numObstacles; i++) {
    if (snakeX[0] == obstacleX[i] && snakeY[0] == obstacleY[i]) {
      return true;
    }
  }
  if (snakeX[0] == movingObstacleX && snakeY[0] == movingObstacleY) {
    return true;
  }
  return false;
}

void gameOver() {
  gameOverFlag = true;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 20);
  display.print("High Score: ");
  display.print(highScore);
  display.display();
  delay(3000);
  resetGame();
}

void handleFoodConsumption() {
  if (snakeLength < MAX_SNAKE_LENGTH) {
    snakeLength++;
  }
  generateFood();
  score += 10;
  if (score > highScore) {
    highScore = score;
  }
  increaseDifficulty();
  if (random(0, 100) < 10) {
    generatePowerUp();
  }
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
    for (int i = 0; i < numObstacles; i++) {
      if (obstacleX[i] == foodX && obstacleY[i] == foodY) {
        validPosition = false;
        break;
      }
    }
    if (foodX == movingObstacleX && foodY == movingObstacleY) {
      validPosition = false;
    }
  }
}

void generateObstacles() {
  for (int i = 0; i < numObstacles; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      obstacleX[i] = random(0, SCREEN_WIDTH / GRID_SIZE);
      obstacleY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE);
      for (int j = 0; j < snakeLength; j++) {
        if (snakeX[j] == obstacleX[i] && snakeY[j] == obstacleY[i]) {
          validPosition = false;
          break;
        }
      }
      if (foodX == obstacleX[i] && foodY == obstacleY[i]) {
        validPosition = false;
      }
    }
  }
  generateMovingObstacle();
}

void generateMovingObstacle() {
  bool validPosition = false;
  while (!validPosition) {
    validPosition = true;
    movingObstacleX = random(0, SCREEN_WIDTH / GRID_SIZE);
    movingObstacleY = random(0, SCREEN_HEIGHT / GRID_SIZE);
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == movingObstacleX && snakeY[i] == movingObstacleY) {
        validPosition = false;
        break;
      }
    }
    for (int i = 0; i < numObstacles; i++) {
      if (obstacleX[i] == movingObstacleX && obstacleY[i] == movingObstacleY) {
        validPosition = false;
        break;
      }
    }
    if (foodX == movingObstacleX && foodY == movingObstacleY) {
      validPosition = false;
    }
  }
}

void generatePowerUp() {
  bool validPosition = false;
  while (!validPosition) {
    validPosition = true;
    powerUpX = random(0, SCREEN_WIDTH / GRID_SIZE);
    powerUpY = random(0, SCREEN_HEIGHT / GRID_SIZE);
    if (powerUpX != foodX || powerUpY != foodY) {
      for (int i = 0; i < snakeLength; i++) {
        if (snakeX[i] == powerUpX && snakeY[i] == powerUpY) {
          validPosition = false;
          break;
        }
      }
      for (int i = 0; i < numObstacles; i++) {
        if (obstacleX[i] == powerUpX && obstacleY[i] == powerUpY) {
          validPosition = false;
          break;
        }
      }
      if (foodX != powerUpX || foodY != powerUpY) {
        powerUpActive = true;
        lastPowerUpTime = millis();
      }
    }
  }
}

void handlePowerUp() {
  if (powerUpActive) {
    score += 50;
    powerUpActive = false;
    increaseDifficulty();
  }
}

void moveObstacles(unsigned long currentTime) {
  if (currentTime - lastObstacleMoveTime > obstacleMoveInterval) {
    lastObstacleMoveTime = currentTime;
    int moveDirection = random(0, 4);
    if (moveDirection == 0 && movingObstacleY > 0) {
      movingObstacleY--;
    } else if (moveDirection == 1 && movingObstacleY < SCREEN_HEIGHT / GRID_SIZE - 1) {
      movingObstacleY++;
    } else if (moveDirection == 2 && movingObstacleX > 0) {
      movingObstacleX--;
    } else if (moveDirection == 3 && movingObstacleX < SCREEN_WIDTH / GRID_SIZE - 1) {
      movingObstacleX++;
    }
  }
}

void displayGame() {
  display.clearDisplay();
  display.fillRect(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }
  for (int i = 0; i < numObstacles; i++) {
    display.drawRect(obstacleX[i] * GRID_SIZE, obstacleY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }
  display.drawRect(movingObstacleX * GRID_SIZE, movingObstacleY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  if (powerUpActive) {
    display.fillRect(powerUpX * GRID_SIZE, powerUpY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 10);
  display.print("High Score: ");
  display.print(highScore);
  display.display();
}

void pauseGame() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Paused");
  display.display();
}

void resetGame() {
  snakeLength = 5;
  score = 0;
  generateFood();
  generateObstacles();
  powerUpActive = false;
  gameOverFlag = false;
  isPaused = false;
  gameStarted = false;
  displayGame();
}

void showStartScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Snake Game");
  display.setCursor(0, 10);
  display.print("Press Button");
  display.display();
}

void increaseDifficulty() {
  if (score > 0 && score % 50 == 0) {
    if (moveInterval > 50) {
      moveInterval -= 10;
    }
    if (obstacleMoveInterval > 300) {
      obstacleMoveInterval -= 100;
    }
  }
}
