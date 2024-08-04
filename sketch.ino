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

int highScore = 0;

#define GRID_SIZE 4
#define MAX_SNAKE_LENGTH 128
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 5;
int foodX, foodY;
int direction = 3;

int score = 0;
bool isPaused = false;
bool adjustingMultiplier = false;
int gameSpeed = 100; 
const int speedOptions[] = {150, 100, 50}; 
int selectedSpeedIndex = 1; 

enum GameMode { NORMAL, GROWTH_MULTIPLIER, SURVIVAL };
GameMode selectedGameMode = NORMAL;
int growthMultiplier = 1;

enum ObstacleMode { NO_OBSTACLES, STATIC_OBSTACLES, DYNAMIC_OBSTACLES };
ObstacleMode selectedObstacleMode = NO_OBSTACLES;

#define MAX_OBSTACLES 5
int obstacleX[MAX_OBSTACLES];
int obstacleY[MAX_OBSTACLES];
int obstacleCount = 0;
int dynamicObstacleSpeed = 1;

#define MAX_POWERUPS 3
int powerUpX[MAX_POWERUPS];
int powerUpY[MAX_POWERUPS];
bool powerUpActive[MAX_POWERUPS];
enum PowerUpType { SPEED_BOOST, SLOW_DOWN, EXTRA_POINTS };
PowerUpType powerUpTypes[MAX_POWERUPS];
int powerUpDuration = 10000;
unsigned long powerUpStartTime[MAX_POWERUPS];

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

  selectGameMode();
  selectGameSpeed();
  selectObstacleMode();
  startGame();
  highScore = retrieveHighScore();
}

void loop() {
  if (digitalRead(joystickSel) == LOW) {
    while (digitalRead(joystickSel) == LOW); // Debounce button press
    if (!isPaused && !adjustingMultiplier) {
      isPaused = true;
      displayPauseScreen();
    } else if (isPaused && !adjustingMultiplier) {
      isPaused = false;
    } else if (!isPaused && adjustingMultiplier) {
      adjustingMultiplier = false;
      isPaused = true;
      displayPauseScreen();
    }
    if (isGameOver()) {
      if (score > highScore) {
        highScore = score;
        saveHighScore(highScore);
      }
      displayHighScore();
      waitForRestart();
    }
  }

  if (!isPaused && !adjustingMultiplier) {
    int vert = analogRead(joystickVert);
    int horz = analogRead(joystickHorz);

    if (vert > center + threshold && direction != 1) {
      direction = 0;
    } else if (vert < center - threshold && direction != 0) {
      direction = 1;
    } else if (horz > center + threshold && direction != 2) {
      direction = 2;
    } else if (horz < center - threshold && direction != 3) {
      direction = 3;
    }

    updateSnake();
    if (selectedObstacleMode == DYNAMIC_OBSTACLES) {
      updateDynamicObstacles();
    }

    if (checkCollision()) {
      gameOver();
    }

    if (snakeX[0] == foodX && snakeY[0] == foodY) {
      for (int i = 0; i < growthMultiplier; i++) {
        if (snakeLength < MAX_SNAKE_LENGTH) {
          snakeLength++;
        }
      }
      score += growthMultiplier;
      generateFood();
    }

    checkPowerUps();

    displayGame();

    delay(gameSpeed);
  } else if (isPaused && !adjustingMultiplier) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      isPaused = false;
    }
  } else if (adjustingMultiplier) {
    displayMultiplierAdjustment();
  }

  if (digitalRead(joystickSel) == LOW && isPaused) {
    while (digitalRead(joystickSel) == LOW); // Debounce button press
    adjustingMultiplier = true;
    isPaused = false;
    displayMultiplierAdjustment();
  }
}

void waitForRestart() {
  while (true) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW);
      startGame();
      break;
    }
  }
}

void generateBonusFood() {
  bool validPosition = false;
  while (!validPosition) {
    validPosition = true;
    bonusFoodX = random(0, SCREEN_WIDTH / GRID_SIZE);
    bonusFoodY = random(0, SCREEN_HEIGHT / GRID_SIZE);
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == bonusFoodX && snakeY[i] == bonusFoodY) {
        validPosition = false;
        break;
      }
    }
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      if ((bonusFoodX == ox || bonusFoodX == ox + 1) && (bonusFoodY == oy || bonusFoodY == oy + 1)) {
        validPosition = false;
        break;
      }
    }
  }
  bonusFoodActive = true;
}

void displayHighScore() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("High Score: ");
  display.print(highScore);
  display.setCursor(0, 10);
  display.print("Press SEL to Restart");
  display.display();
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

  if (snakeX[0] < 0 || snakeX[0] >= SCREEN_WIDTH / GRID_SIZE || snakeY[0] < 0 || snakeY[0] >= SCREEN_HEIGHT / GRID_SIZE) {
    gameOver();
  }
}

bool checkCollision() {
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
    }
  }

  if (selectedObstacleMode != NO_OBSTACLES) {
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      if ((snakeX[0] == ox || snakeX[0] == ox + 1) && (snakeY[0] == oy || snakeY[0] == oy + 1)) {
        return true;
      }
    }
  }

  return false;
}

void gameOver() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over :(");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 20);
  display.print("Press SEL to Restart");
  display.display();

  while (true) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectGameMode();
      selectGameSpeed();
      selectObstacleMode();
      startGame();
      break;
    }
  }
}

void startGame() {
  snakeLength = 5;
  direction = 3;
  score = 0;
  isPaused = false;

  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = snakeLength - i - 1;
    snakeY[i] = 0;
  }

  generateFood();
  generateObstacles();
  generatePowerUps();
  display.clearDisplay();
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
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      if ((foodX == ox || foodX == ox + 1) && (foodY == oy || foodY == oy + 1)) {
        validPosition = false;
        break;
      }
    }
  }
}

void generateBonusFood() {
  for (int i = 0; i < MAX_BONUS_FOOD; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      bonusFoodX[i] = random(0, SCREEN_WIDTH / GRID_SIZE);
      bonusFoodY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE);
      for (int j = 0; j < snakeLength; j++) {
        if (snakeX[j] == bonusFoodX[i] && snakeY[j] == bonusFoodY[i]) {
          validPosition = false;
          break;
        }
      }
      if (bonusFoodX[i] == foodX && bonusFoodY[i] == foodY) {
        validPosition = false;
      }
      for (int j = 0; j < obstacleCount; j++) {
        if (bonusFoodX[i] == obstacleX[j] && bonusFoodY[i] == obstacleY[j]) {
          validPosition = false;
          break;
        }
      }
    }
    bonusFoodActive[i] = true;
    bonusFoodStartTime[i] = millis();
  }
}


void generateObstacles() {
  if (selectedObstacleMode == STATIC_OBSTACLES || selectedObstacleMode == DYNAMIC_OBSTACLES) {
    obstacleCount = random(1, MAX_OBSTACLES + 1);
    for (int i = 0; i < obstacleCount; i++) {
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
        for (int j = 0; j < obstacleCount; j++) {
          if (i != j && obstacleX[i] == obstacleX[j] && obstacleY[i] == obstacleY[j]) {
            validPosition = false;
            break;
          }
        }
      }
    }
  }
}

void updateDynamicObstacles() {
  for (int i = 0; i < obstacleCount; i++) {
    obstacleX[i] += dynamicObstacleSpeed;
    if (obstacleX[i] >= SCREEN_WIDTH / GRID_SIZE) {
      obstacleX[i] = 0;
    }
  }
}

void generatePowerUps() {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      powerUpX[i] = random(0, SCREEN_WIDTH / GRID_SIZE);
      powerUpY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE);
      for (int j = 0; j < snakeLength; j++) {
        if (snakeX[j] == powerUpX[i] && snakeY[j] == powerUpY[i]) {
          validPosition = false;
          break;
        }
      }
      for (int j = 0; j < obstacleCount; j++) {
        if (powerUpX[i] == obstacleX[j] && powerUpY[i] == obstacleY[j]) {
          validPosition = false;
          break;
        }
      }
    }
    powerUpActive[i] = true;
    powerUpTypes[i] = static_cast<PowerUpType>(random(0, 3));
    powerUpStartTime[i] = millis();
  }
}

void checkPowerUps() {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (powerUpActive[i] && snakeX[0] == powerUpX[i] && snakeY[0] == powerUpY[i]) {
      powerUpActive[i] = false;
      switch (powerUpTypes[i]) {
        case SPEED_BOOST:
          gameSpeed /= 2;
          break;
        case SLOW_DOWN:
          gameSpeed *= 2;
          break;
        case EXTRA_POINTS:
          score += 10;
          break;
      }
      powerUpStartTime[i] = millis();
    }

    if (!powerUpActive[i] && millis() - powerUpStartTime[i] >= powerUpDuration) {
      switch (powerUpTypes[i]) {
        case SPEED_BOOST:
          gameSpeed *= 2;
          break;
        case SLOW_DOWN:
          gameSpeed /= 2;
          break;
        case EXTRA_POINTS:
          break;
      }
    }
  }
}

void selectGameMode() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Game Mode:");
  display.setCursor(0, 10);
  display.print("1. Normal");
  display.setCursor(0, 20);
  display.print("2. Growth Multiplier");
  display.setCursor(0, 30);
  display.print("3. Survival");
  display.display();

  while (true) {
    int vert = analogRead(joystickVert);
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      if (vert > center + threshold) {
        selectedGameMode = NORMAL;
      } else if (vert < center - threshold) {
        selectedGameMode = GROWTH_MULTIPLIER;
        adjustingMultiplier = true;
        displayMultiplierAdjustment();
      } else if (abs(vert - center) < threshold) {
        selectedGameMode = SURVIVAL;
      }
      break;
    }
  }

  display.clearDisplay();
}

void selectGameSpeed() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Game Speed:");
  display.setCursor(0, 10);
  display.print("1. Slow");
  display.setCursor(0, 20);
  display.print("2. Normal");
  display.setCursor(0, 30);
  display.print("3. Fast");
  display.display();

  while (true) {
    int vert = analogRead(joystickVert);
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      if (vert > center + threshold) {
        selectedSpeedIndex = 0;
      } else if (vert < center - threshold) {
        selectedSpeedIndex = 1;
      } else if (abs(vert - center) < threshold) {
        selectedSpeedIndex = 2;
      }
      gameSpeed = speedOptions[selectedSpeedIndex];
      break;
    }
  }

  display.clearDisplay();
}

void selectObstacleMode() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Obstacle Mode:");
  display.setCursor(0, 10);
  display.print("1. No Obstacles");
  display.setCursor(0, 20);
  display.print("2. Static Obstacles");
  display.setCursor(0, 30);
  display.print("3. Dynamic Obstacles");
  display.display();

  while (true) {
    int vert = analogRead(joystickVert);
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      if (vert > center + threshold) {
        selectedObstacleMode = NO_OBSTACLES;
      } else if (vert < center - threshold) {
        selectedObstacleMode = STATIC_OBSTACLES;
      } else if (abs(vert - center) < threshold) {
        selectedObstacleMode = DYNAMIC_OBSTACLES;
      }
      break;
    }
  }

  display.clearDisplay();
}

void displayMultiplierAdjustment() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Growth Multiplier:");
  display.setCursor(0, 10);
  display.print(growthMultiplier);
  display.display();

  while (adjustingMultiplier) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      growthMultiplier++;
      delay(200); 
    } else if (vert < center - threshold) {
      if (growthMultiplier > 1) {
        growth multiplier--;
        delay(200); 
      }
    }
    display.setCursor(0, 10);
    display.print(growthMultiplier);
    display.display();
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW);
      adjustingMultiplier = false;
    }
  }

  display.clearDisplay();
}

void displayPauseScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Paused");
  display.setCursor(0, 10);
  display.print("Press SEL to Resume");
  display.display();
}

void displayMultiplierAdjustment() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Adjust Growth Multiplier:");

  int selectedMultiplier = growthMultiplier;
  bool adjustmentMade = false;
  while (!adjustmentMade) {
    int vert = analogRead(joystickVert);
    if (vert < center - threshold) {
      selectedMultiplier = max(1, selectedMultiplier - 1);
    } else if (vert > center + threshold) {
      selectedMultiplier = min(10, selectedMultiplier + 1);
    }

    display.setCursor(0, 10);
    display.print("Multiplier: ");
    display.print(selectedMultiplier);

    display.display();

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); 
      growthMultiplier = selectedMultiplier;
      adjustmentMade = true;
    }
  }

  delay(1000); 
  adjustingMultiplier = false;
  isPaused = true;
  displayPauseScreen();
}

void selectGameMode() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Game Mode:");

  int selectedOption = 0;
  bool selectionMade = false;
