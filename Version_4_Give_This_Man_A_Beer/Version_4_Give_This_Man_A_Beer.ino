/*
Not tested yet. Pinouts have to be revised

improvements: 
- light up loser cup somehow someway

*/

#include <Arduino.h>
//#include <Stepper.h>
#include <FastLED.h>

#define NUM_USER_BUTTONS 6

// define strip LED pins (needs to be connected on PWM pin)

#define LED_PIN 13
#define NUM_LEDS 56
#define BRIGHTNESS 64
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

// define other stuff for LEDs

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// input pins

int playerReadPin[NUM_USER_BUTTONS] = {2, 3, 4, 5, 6, 7};
int acceptedPlayers[NUM_USER_BUTTONS] = {0, 0, 0, 0, 0, 0};
// const int startButtonPin = 8;

// pump pins

const int RELAY_PIN = 40;

// Stepper pins

const int enPin = 13;
const int dirPin = 12 ;
const int stepPin = 11 ;

//constent used for calculation of the stepperMotion
const int stepsPerRev = 800;
int previousAngle = 0;
double gearRatio = 1.8;
int currentAngle = 0;
double stepperMotion = 0;
double stepperDiff = 0 ;
const int halfCircle = 180;
const int fullCircle = 360;

// MOST important variable it can range from 1 to 6 and represent the position of the losing button,
// so if the 3rd button was the slowest, it will be position 3
int lastPlayer = 0;

// variables used through code

long slowestTime = -1;
long slowestPlayer = -1;

// variables per game

bool disqualifiedPlayers[NUM_USER_BUTTONS];
long playerTimes[NUM_USER_BUTTONS] = {0,0,0,0,0,0};
int runnerUpsOrder[NUM_USER_BUTTONS - 1] = {0,0,0,0,0};
String playersOrder[NUM_USER_BUTTONS - 1] = {"","","","",""};
int runnerUpPosition = 0;
int numFinish = 0;

// time variables (all in milliseconds)

long startTime = 0;
long losingTime = 0;
long winningTime = 0;
long gameOverTime = 0;
int randomDelay = 0;

void setup() {

  for (int i = 0; i < NUM_USER_BUTTONS; i++)
  {
    pinMode(playerReadPin[i], INPUT);
  }

  pinMode(startButtonPin, INPUT);
  pinMode(resetPin, INPUT);

  // for the pump

  pinMode(RELAY_PIN, OUTPUT);
  
  // for the RGB strip

  delay(3000); //safety delay on boot

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM = {
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,

    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
      
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
  };

  // for the stepper

  //pinMode(limitSwitchPin, INPUT);

  pinMode(enPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(enPin, LOW);

  while(!Serial);

  Serial.begin(9600);
  //motor.setSpeed(20);

}

// if we use addressable LED strip

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t brightness = 255;

  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void changePalettePeriodically() {
  uint8_t secondHand = (millis()/1000) % 60;
  static uint8_t lastSecond = 99;

  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
    if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
    if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }      
    if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
    if( secondHand == 25)  { setUpRandomPalette();                     currentBlending = LINEARBLEND; }
    if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND;     }    
    if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
    if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
    if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
    if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
    if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
  }
}

void setUpRandomPalette() {
  for( int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV( random8(), 255, random8());
  }
}

void SetupBlackAndWhiteStripedPalette() {
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;   
}

void SetupPurpleAndGreenPalette() {
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;
    
  currentPalette = CRGBPalette16(
                                 green,  green,  black,  black,
                                 purple, purple, black,  black,
                                 green,  green,  black,  black,
                                 purple, purple, black,  black );
}

void setRandomDelay() { // sets random time delay for game
  randomSeed(millis() + digitalRead(startButtonPin)); // check logic here with states
  randomDelay = random(3000,10000);
}

bool checkStart() { // returns if start button input is HIGH
  return digitalRead(startButtonPin) == HIGH;
}

void readInputPins() { // reads HIGH || LOW from input pin
  for(int i = 0; i < NUM_USER_BUTTONS; i++){
    playerReadPin[i] = digitalRead(playerReadPin[i]);
  }
}

void clearGame() {
  for (int i = 0; i < NUM_USER_BUTTONS; i++){
    disqualifiedPlayers[i] = false;
    playerTimes[i] = 0;

    if(i < NUM_USER_BUTTONS - 1){
      runnerUpsOrder[i] = -1;
      playersOrder[i] = "";
    }
  }

  runnerUpPosition = 0;
  randomDelay = 0;
  numFinish = 0;
  startTime = 0;
  losingTime = 0;
  gameOverTime = 0;
  // debugSerialPrintln("Game variables cleared.")
}

void disqualifyPlayers() {
  if(numFinish < NUM_USER_BUTTONS)
  {
    readInputPins();

    for(int i = 0; i < NUM_USER_BUTTONS; i++)
    {
      if(playerReadPin[i] > 0 && !disqualifiedPlayers[i])
      {
       int pressedButton = i;
       disqualifiedPlayers[pressedButton] = true;
       // debugSerialPrintStringAndNumber("Player ", pressedButton, " disqualified.");
      }
    }
  }
}

void acceptPlayers() {
  bool gameNotStarted = true;
  while(gameNotStarted) // 
  {
    for(int i = 0; i < NUM_USER_BUTTONS; i++)
    {
      if(playerReadPin[i] == HIGH)
      {
        acceptedPlayers[i] = 1;
      }
    }
    if(startButtonPin == HIGH)
    {
      int sum = 0;
      for(int i = 0; i < NUM_USER_BUTTONS; i++)
      {
        if(acceptedPlayers[i] > 0)
        {
          sum++;
        }
      }
      if(sum > 1)
      {
        gameNotStarted = false;
      }
      else{//send error message
      }
    }
  }
  // change states
}

void checkForLoser() { //badly coded, needs to be changed to returmn array[6] in ordered array

  if (numFinish < NUM_USER_BUTTONS)
  {
    readInputPins();
    for (int i = 0; i < NUM_USER_BUTTONS; i++)
    {
      if (playerReadPin[i] > 0 && playerTimes[i] == 0 && !disqualifiedPlayers[i])
      {
        int pressedButton = i;
        playerTimes[pressedButton] = millis();

        if (numFinish == 0) // check logic here too
        {
          long playerTimes = millis();
          long loserReactionTime = losingTime - startTime;
          long winnerReactionTime = winningTime - startTime; // this line is only useful if we want to show player scores

          /*
          check if all time variables used above are defined appropriately. losingTime hasn't been used before.
          */
        }

        else
        {
          long winnerReactionTime = winningTime - startTime; // gotta be rewritten otherwise it is undefined
          long runnerUpTime = millis() - winnerReactionTime;
          runnerUpsOrder[runnerUpPosition] = runnerUpTime;
          runnerUpPosition++;
        }
        numFinish++;
      }
    }
  }
}

//calculates the shortest path based on the previous position
void stepperPosition(){
 stepperDiff = currentAngle - previousAngle;  //calculates the distance between the two position
  
  if( stepperDiff == 0 )
  {
  stepperMotion  = 0;
  }

  if( stepperDiff == halfCircle || stepperDiff == -halfCircle )
  {
  stepperMotion  = 180;
  }

  else if( stepperDiff > halfCircle )
  {
  digitalWrite(dirPin, LOW); //sets the spinning direction based on the state of the pin
  stepperMotion = -(stepperDiff-fullCircle);
  }

  else if( stepperDiff < (-halfCircle))
  {
  digitalWrite(dirPin,HIGH);
  stepperMotion = (stepperDiff+fullCircle);
  }

  else if(0<stepperDiff && stepperDiff < halfCircle)
  {
  digitalWrite(dirPin, HIGH);
  stepperMotion = stepperDiff;
  }

  else  if(0 > stepperDiff && stepperDiff > -halfCircle)
  {
  digitalWrite(dirPin,LOW);
  stepperMotion = (-stepperDiff);
  }
}

//spins the motor based on the previous calculation
void stepperRotation()
{
  for(double step = 0; step < (gearRatio*((stepperMotion*(stepsPerRev/360)))); step++) // does the calculation by transforming the angle into the steps required and will loop until that number is reached
    {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(1000);//time between each pulse and theres is 800 pulse per revolution so the lower the delay the faster the spin, do not modify
    digitalWrite(stepPin,LOW);
    delayMicroseconds(1000);
    }
  previousAngle = currentAngle;
}

void fillCup() { // function to turn stepper to loser's and disqualified players' cups

  currentAngle = ((LastPlayer)*60)-30; //takes the number of the last player so if the last player is at the 3th button it will give the angle for it 
  stepperPosition();
  stepperRotation();
  delay(200);
  digitalWrite(RELAY_PIN, HIGH); // turn on pump
  delay(8000); // turn on pump for 8 seconds
  digitalWrite(RELAY_PIN, LOW); // Turn off pump

}

void endGame() {
  // Display the final scores
  for (int i = 0; i < 6; i++) {
    Serial.print("Player ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(playerScores[i]);
  }
}

void loop() {
  // main logic of the game. Still needs to be worked on. Cases don't work without an LCD menu screen

  changePalettePeriodically();
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1;

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000/UPDATES_PER_SECOND);

  if(checkStart())
  {
    clearGame();
    acceptPlayers();
    setRandomDelay();
    startTime = millis() + randomDelay;
    checkForLoser();
    endGames();
    //fillCup();
  }
}
