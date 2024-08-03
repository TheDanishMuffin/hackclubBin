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
int direction = 3;

int score = 0;
bool isPaused = false;
bool adjustingMultiplier = false;
int gameSpeed = 100; 
const int speedOptions[] = {150, 100, 50}; 
int selectedSpeedIndex = 1; 

enum GameMode { NORMAL, GROWTH_MULTIPLIER };
GameMode selectedGameMode = NORMAL;
int growthMultiplier = 1;

enum ObstacleMode { NO_OBSTACLES, STATIC_OBSTACLES, DYNAMIC_OBSTACLES };
ObstacleMode selectedObstacleMode = NO_OBSTACLES;

#define MAX_OBSTACLES 5
int obstacleX[MAX_OBSTACLES];
int obstacleY[MAX_OBSTACLES];
int obstacleCount = 0;
int dynamicObstacleSpeed = 1;

enum BackgroundColor { BLACK, WHITE, INVERSE };
BackgroundColor selectedBackgroundColor = BLACK;

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
  selectBackgroundColor();
  startGame();
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
      selectBackgroundColor();
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

void generateObstacles() {
  if (selectedObstacleMode == NO_OBSTACLES) {
    obstacleCount = 0;
    return;
  }

  obstacleCount = random(1, MAX_OBSTACLES);
  for (int i = 0; i < obstacleCount; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      obstacleX[i] = random(0, SCREEN_WIDTH / GRID_SIZE - 1);
      obstacleY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE - 1);
      for (int j = 0; j < snakeLength; j++) {
        if ((snakeX[j] == obstacleX[i] || snakeX[j] == obstacleX[i] + 1) && (snakeY[j] == obstacleY[i] || snakeY[j] == obstacleY[i] + 1)) {
          validPosition = false;
          break;
        }
      }
      for (int j = 0; j < i; j++) {
        if (obstacleX[j] == obstacleX[i] && obstacleY[j] == obstacleY[i]) {
          validPosition = false;
          break;
        }
      }
    }
  }
}

void updateDynamicObstacles() {
  for (int i = 0; i < obstacleCount; i++) {
    obstacleX[i] += dynamicObstacleSpeed;
    if (obstacleX[i] >= SCREEN_WIDTH / GRID_SIZE - 1 || obstacleX[i] < 0) {
      dynamicObstacleSpeed = -dynamicObstacleSpeed;
    }
  }
}

void displayGame() {
  display.clearDisplay();
  
  // Set background color
  switch (selectedBackgroundColor) {
    case BLACK:
      display.fillScreen(SSD1306_BLACK);
      display.setTextColor(SSD1306_WHITE);
      break;
    case WHITE:
      display.fillScreen(SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      break;
    case INVERSE:
      display.fillScreen(SSD1306_INVERSE);
      display.setTextColor(SSD1306_WHITE);
      break;
  }
  
  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }

  display.fillRect(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);

  if (selectedObstacleMode != NO_OBSTACLES) {
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      display.fillRect(ox * GRID_SIZE, oy * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
    }
  }

  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  display.display();
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
  display.print("Adjust Multiplier");
  display.setCursor(0, 10);
  display.print("Current: ");
  display.print(growthMultiplier);
  display.setCursor(0, 20);
  display.print("Press SEL to Confirm");
  display.display();

  while (adjustingMultiplier) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      growthMultiplier++;
      display.setCursor(0, 10);
      display.print("Current: ");
      display.print(growthMultiplier);
      display.display();
      delay(300);
    } else if (vert < center - threshold) {
      if (growthMultiplier > 1) {
        growthMultiplier--;
      }
      display.setCursor(0, 10);
      display.print("Current: ");
      display.print(growthMultiplier);
      display.display();
      delay(300);
    }
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      adjustingMultiplier = false;
    }
  }
}

void selectGameMode() {
  const char* gameModeOptions[] = { "Normal", "Growth Multiplier" };
  int gameModeIndex = 0;

  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Select Game Mode");
    for (int i = 0; i < 2; i++) {
      display.setCursor(0, 10 + i * 10);
      if (i == gameModeIndex) {
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.print(gameModeOptions[i]);
    }
    display.display();

    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      gameModeIndex++;
      if (gameModeIndex >= 2) {
        gameModeIndex = 0;
      }
      delay(300);
    } else if (vert < center - threshold) {
      gameModeIndex--;
      if (gameModeIndex < 0) {
        gameModeIndex = 1;
      }
      delay(300);
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectedGameMode = static_cast<GameMode>(gameModeIndex);
      break;
    }
  }
}

void selectGameSpeed() {
  int speedIndex = selectedSpeedIndex;

  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Select Game Speed");
    for (int i = 0; i < 3; i++) {
      display.setCursor(0, 10 + i * 10);
      if (i == speedIndex) {
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.print(speedOptions[i]);
    }
    display.display();

    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      speedIndex++;
      if (speedIndex >= 3) {
        speedIndex = 0;
      }
      delay(300);
    } else if (vert < center - threshold) {
      speedIndex--;
      if (speedIndex < 0) {
        speedIndex = 2;
      }
      delay(300);
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectedSpeedIndex = speedIndex;
      gameSpeed = speedOptions[speedIndex];
      break;
    }
  }
}

void selectObstacleMode() {
  const char* obstacleModeOptions[] = { "No Obstacles", "Static Obstacles", "Dynamic Obstacles" };
  int obstacleModeIndex = 0;

  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Select Obstacle Mode");
    for (int i = 0; i < 3; i++) {
      display.setCursor(0, 10 + i * 10);
      if (i == obstacleModeIndex) {
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.print(obstacleModeOptions[i]);
    }
    display.display();

    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      obstacleModeIndex++;
      if (obstacleModeIndex >= 3) {
        obstacleModeIndex = 0;
      }
      delay(300);
    } else if (vert < center - threshold) {
      obstacleModeIndex--;
      if (obstacleModeIndex < 0) {
        obstacleModeIndex = 2;
      }
      delay(300);
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectedObstacleMode = static_cast<ObstacleMode>(obstacleModeIndex);
      break;
    }
  }
}

void selectBackgroundColor() {
  const char* backgroundColorOptions[] = { "Black", "White", "Inverse" };
  int backgroundColorIndex = 0;

  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Select Background Color");
    for (int i = 0; i < 3; i++) {
      display.setCursor(0, 10 + i * 10);
      if (i == backgroundColorIndex) {
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.print(backgroundColorOptions[i]);
    }
    display.display();

    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      backgroundColorIndex++;
      if (backgroundColorIndex >= 3) {
        backgroundColorIndex = 0;
      }
      delay(300);
    } else if (vert < center - threshold) {
      backgroundColorIndex--;
      if (backgroundColorIndex < 0) {
        backgroundColorIndex = 2;
      }
      delay(300);
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectedBackgroundColor = static_cast<BackgroundColor>(backgroundColorIndex);
      break;
    }
  }
}
