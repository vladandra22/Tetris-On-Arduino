#include <Arduino.h>
#include <Wire.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define CENTRU 510

void moveTetromino();
bool isTetromino(int i, int j);
bool isTetrominoCenter(int i, int j);
bool isTetrominoPart(int i, int j);
bool isNotValidMove(int i, int j);
void moveLeft();
void moveRight();
void moveDown();
void rotateLeft(int currentPiece);
void updateGame();
int addScore();
void selectRandomTetromino();
void displayGameOnLedMatrix();
void forceTetrominoDown();
void urmatoareaNotaCantecel();
void restartGame();


// Pini joystick
const int SW_PIN = 2;
const int WRX_PIN = A0;
const int WRY_PIN = A1;

// Pini MAX7219 (matrice LED)
const int DIN_PIN = 12;
const int CLK_PIN = 11;
const int CS_PIN = 10;
const int NUM_MAX7219 = 1;

// Pin Buzzer
const int BUZZER_PIN = 13;
// Pin seed analog pentru randomizare
const int SEED_PIN = A8;

// Timpul intre doua comenzi de Joystick.
unsigned long lastJoystickInput = 0;
const unsigned long intervalJoystick = 120;

// Delay-ul intre comenzile de force down.
unsigned long lastForceDownInput = 0;
const unsigned long intervalForceInput = 500;

// Timpul intre update-urile statusului jocului.
unsigned long lastGameUpdate = 0;
const unsigned long intervalUpdate = 800;

// Timpul apasarii butonului de reset.
unsigned long pressDuration = 3000; 
unsigned long buttonPressTime = 0;

// Adresa de scriere in memoria EEPROM a highscore-ului.
int address1 = 0;
int retrievedValue1;

// Score, highscore, daca s-a incheiat jocul si daca s-a afisat mesajul de 'Game Over'.
// (ultima variabila o folosim ca sa nu afisam mesajul la nesfarsit, deoarece
// statusul de 'game over' se verifica continuu).
int lastScore = -1;
int score = 0;
int highscore = 0;
int gameOver = false;
bool displayGameOver = false;

// Variabilele acestea pot fi modificate atat de intreruperi, 
// cat si de restul codului, asadar le scriem cu 'volatile'
// pentru a asigura ca nu avem acces la memorie optimizat necorespunzator.
volatile int playSong = true;
volatile int currentNote = 0;

// Matricea de joc. Avem un spatiu de 4 patratele in sus pentru
// ca piesele sa cada progresiv in joc.
int gameGrid[12][8]; 

// Tetromino ales curent si de cate ori apare acesta consecutiv in joc.
int pieceId = -1;
int freqPiece = 0;

// Vrem ca dupa un timp dupa ce avem 'Game Over' sa stingem led-urile si ecranul.
const unsigned long ledOnDuration = 30000;
unsigned long ledOnStartTime = 0;

// Forme pentru piesele Tetromino
static int tetrominoes[][4][8] = {
  // ---- = I Tetromino
  {
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 2, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0}
  },
  // Patrat = O Tetromino
  {
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  },
  // -|- T tetromino
  {
    {0, 0, 1, 2, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  },
  // |
  // |_ L Tetromino
  {
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 2, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  },
  // |_
  //  | = S Tetromino
    {
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 2, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  }
};

// Verificam ce tip de piese avem.
bool isTetromino(int i, int j) {
  return gameGrid[i][j] == 1 || gameGrid[i][j] == 2;
}

bool isTetrominoCenter(int i, int j) {
  return gameGrid[i][j] == 2;
}

bool isTetrominoPart(int i, int j) {
  return gameGrid[i][j] == 1;
}

bool isNotValidMove(int i, int j) {
  return gameGrid[i][j] == 3;
}