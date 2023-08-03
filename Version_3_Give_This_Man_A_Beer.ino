/*
Not tested yet. Pinouts have to be revised
*/

#include <Arduino.h>
//#include <Stepper.h>
#include <FastLED.h>

#define NUM_USER_BUTTONS 6

// define strip LED pins (needs to be connectond on PWM pin)

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

int playerReadPin[NUM_USER_BUTTONS] = {1, 2, 3, 4, 5, 6};
int acceptedPlayers[NUM_USER_BUTTONS] = {0, 0, 0, 0, 0, 0};
const int resetPin = 11;
const int startButtonPin = 8;

/* Uncomment Stepper.h (and find right library for stepper bc there's a ton)

// stepper pins. Might need to be changed depending on stepper pinouts

int stepPin1 = 28;
int stepPin2 = 26;
int stepPin3 = 24;
int stepPin4 = 22;

Stepper motor(512, stepPin1, stepPin2, stepPin3, stepPin4);

// limit switch pins
const int limitSwitchPin = 40;
*/

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

// Game states. Pinouts (if they are pinouts) need to be changed
const int bootingUp = 0;
const int waitForStart = 5;
const int gameCountdown = 10;
const int disqualifying = 15;
const int gamePlay = 20;
const int gameOver = 25;
const int fillingCup = 30;
const int gameAcceptPlayers = 35;

// current state

int state = bootingUp;

void setup() { // sets up inputs

  for (int i = 0; i < NUM_USER_BUTTONS; i++)
  {
    pinMode(playerReadPin[i], INPUT);
  }

  pinMode(startButtonPin, INPUT);
  pinMode(resetPin, INPUT);
  
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

  // for the stepper (uncomment when you find the right library)

  //pinMode(limitSwitchPin, INPUT);

  //pinMode(stepPin1, OUTPUT);
  //pinMode(stepPin2, OUTPUT);
  //pinMode(stepPin3, OUTPUT);
  //pinMode(stepPin4, OUTPUT);

  while(!Serial);

  Serial.begin(9600);
  //motor.setSpeed(20);

}

/* if we use RGB LED strip, use this following part:

void setColourRGB(unsigned int red, unsigned int blue, unsigned int green) {
  analogWrite(RED, red);
  analogWrite(BLUE, blue);
  analogWrite(GREEN, green);
}

void cycleStripRGB() {
  unsigned int colourOfRGB[3] = {255, 0, 0};

  // choosing colours to increment and decrement
  for(int decrementColour = 0; decrementColour < 3; decrementColour += 1)
  {
    int incrementColour = decrementColour == 2 ? 0 : decrementColour + 1;

    // cross-fade the two colours

    for(int i = 0; i < 255; i += 1)
    {
      colourOfRGB[decrementColour] -= 1;
      colourOfRGB[incrementColour] += 1;
      setColourRGB(colourOfRGB[0],colourOfRGB[1],colourOfRGB[3]);
      delay(5);
    }
  }
}

void startColourRGB() {
  unsigned int colourOfRGB[3] = {255, 0, 0};
  int brightness = 255;
  int fadeAmount = 255;

  analogWrite(RED, brightness);
  analogWrite(BLUE, brightness);
  analogWrite(GREEN, brightness);
  brightness = brightness - fadeAmount;
  delay(300);
}

 if we use single LEDs, use this following part:

void fadeWAIT_LED() { // function to either fade LEDs or switch their colours
  int brightness = 0;
  int fadeAmount = 5;

  analogWrite(WAIT_LED, brightness);
  brightness = brightness + fadeAmount;
  if(brightness <= 0 || brightness >= 255)
  {
    fadeAmount = -fadeAmount;
  }
  delay(30);
}

void flashGO_LED() { // function to make LEDs flash
  int brightness = 0;
  int fadeAmount = 5;

  analogWrite(GO_LED, brightness);
  brightness = brightness + fadeAmount;
  if(brightness <= 0 || brightness >= 255)
  {
    fadeAmount = -fadeAmount;
  }
  delay(5);
}
*/

// if we use addressable LED strip

void FillLEDsFromPaletteColors(uint8_t colorIndex){
  uint8_t brightness = 255;

  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void changePalettePeriodically(){
  uint8_t secondHand = (millis()/1000) % 60;
  static uint8_t lastSecond = 99;

  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
    if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
    if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }      
    if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
    if( secondHand == 25)  { setUpRandomPalette();              currentBlending = LINEARBLEND; }
    if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND;     }    
    if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
    if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
    if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
    if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
    if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
  }
}

void setUpRandomPalette(){
  for( int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV( random8(), 255, random8());
  }
}

void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;   
}

void SetupPurpleAndGreenPalette()
{
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

void checkForLoser() {

  if (numFinish < NUM_USER_BUTTONS)
  {
    readInputPins();
    for (int i = 0; i < NUM_USER_BUTTONS; i++)
    {
      if (playerReadPin[i] > 0 && /* playerTimes[i] == 0 && */ !disqualifiedPlayers[i]) //check logic behind this one
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

void fillCup() { // function to turn stepper to loser's cup
  //has to be integrated into main logic function void loop()
}

void loop() {
  // main logic of the game. Still needs to be worked on.

  changePalettePeriodically();
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1;

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000/UPDATES_PER_SECOND);

  switch(state)
  {
    case bootingUp:
    //cycleStripRGB();
    break;

    case waitForStart:
      if(checkStart())
      {
        stateTransition(gameAcceptPlayers);
      }
    break;

    case gameAcceptPlayers:
      acceptPlayers();
    break;

    case gameCountdown:
      if(millis() > startTime) // doesnt make sense. Player button needs to pressed to get a time in millis()
      {
        stateTransition(gamePlay);
        // fade led or switch led to wait led (gotta write a function for that) 
      }
      else
      {
        disqualifyPlayers(); // this function doesn't bring us back to another state.
        // fade led or switch led to wait led (gotta write a function for that)
      }
    break;

    case gamePlay:
      if(millis() > (gameOverTime + 3000 * numFinish)) // 3000 supposed to be in milliseconds here, might have to modify it
      {
        stateTransition(gameOver);
      }
    break;

    case gameOver:
      if(millis() > (gameOverTime + 2000 * numFinish)) // 2000 supposed to be in milliseconds here, might have to modify it
      {
        stateTransition(waitForStart);
      }
      if(checkStart())
      {
        stateTransition(gameCountdown);
      }
      /*
      if((millis() % 2000 == 0) // 2000 supposed to be in milliseconds here, might have to modify it
      {
        //showUserTimes();
      }
      */
    break;

    /*
    case fillCup:
      // call on function to fill cup
    break;
    */
  }
}


void stateTransition(int newState) {
  switch(newState)
  {
    case waitForStart:
      //press start. Doesnt do anything
    break;

    case gameCountdown:
      clearGame(); // need to implement 
      setRandomDelay();
      startTime = millis() + randomDelay;
    break;

    case gamePlay:
      //digitalWrite(GO_LED, HIGH);
      checkForLoser();
    break;

    case gameOver:
      //digitalWrite(GO_LED, LOW);
      gameOverTime = millis();
    break;
  }
}
