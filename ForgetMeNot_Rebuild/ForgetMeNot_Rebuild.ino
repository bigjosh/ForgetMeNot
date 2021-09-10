/*
   Forget-me-not

   by Jeph Stahl
   Additional Development by Move38

*/

char junk;

enum pieceTypes {
  CENTER,
  PETAL
};

byte pieceType = PETAL; // default to PETAL
bool bPossibleCenter = false;

enum gameStates {
  SETUP,
  GAMEPLAY,
  ANSWER,
  SCOREBOARD,
  RESET
};

byte gameState = RESET;

enum puzzleStates {
  SHOW,
  HIDE,
  WAIT,
  CORRECT,
  WRONG
};

byte puzzleState = WAIT;

Timer puzzleTimer;  // runs the timing of the puzzle

//SHOW/DARK time variables TODO: TUNE THESE
#define MAX_SHOW_TIME 5000    // 5 seconds on
#define MIN_SHOW_TIME 3000    // 3 seconds on
#define MIN_DARK_TIME 1500    // 1.5 second  off
#define MAX_DARK_TIME 3000    // 3 seconds off
#define CURVE_BEGIN_LEVEL 1
#define CURVE_END_LEVEL 10
int showTime = MAX_SHOW_TIME;
int darkTime = MIN_DARK_TIME;

Timer answerTimer; // runs the timing of displaying the answer
#define ANSWER_REVEAL_DURATION 3000  // 3 seconds

/*
   For Scoreboard
   ----------------------------------------
*/
bool isScoreboard = false;

#define PIP_IN_ROUND 18
#define NUM_PETALS 6
#define NUM_PIP_IN_PETAL (PIP_IN_ROUND / NUM_PETALS)
#define PIP_DURATION_IN_ROUND 60
#define PIP_DURATION_IN_SCORE 300
uint16_t roundDuration = PIP_DURATION_IN_ROUND * PIP_IN_ROUND;
byte currentRound;
byte numberOfRounds;
byte numberOfPips;

uint32_t timeOfGameEnding = 0;
uint32_t timeSinceScoreboardBegan = 0;

byte petalID;
// ----------------------------------------

#define MAX_LEVEL 72
byte currentLevel = 0;

/*
  Puzzle Info
  ---------------
  0: type
  1: palette
  2: difficulty
  3: isAnswer
  4: level
  5: petalID
*/

class puzzleInfo_t {

  public: 
  byte type;
  byte palette;
  byte difficulty;
  byte isAnswer;
  byte level;
  byte petalID;
};

puzzleInfo_t puzzleInfo;// = {0, 0, 0, 0, 0, 0};

static puzzleInfo_t emptyPuzzleInfo;// = {0, 0, 0, 0, 0, 0};

void clearPuzzleInfo() {
  puzzleInfo = emptyPuzzleInfo;
}

char junk2;

bool bSendPuzzle = false;
Timer datagramTimer;
#define DATAGRAM_TIMEOUT 1000
byte answerFace = FACE_COUNT;
byte centerFace = FACE_COUNT;

enum puzzleType {
  COLOR_PETALS,       // Each petal is a single color
  LOCATION_PETALS,    // Each petal is lit a single direction
  DUO_PETALS,         // Each petal is a pair of colors
  ROTATION_PETALS     // Each petal animates rotation CW or CCW
};

/*
   Forget-me-not **Puzzle Levels**
   (^.^)  @)--^--  (^_^)          |                      STAGE 1                       |                      STAGE 2                       |                      STAGE 3                       |                      STAGE 4                       |
                                  |````````````````````````````````````````````````````|****************************************************|####################################################|$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$|
*/
byte puzzleArray[MAX_LEVEL] =     {0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 2, 1, 0, 2, 3, 3, 2, 0, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
byte difficultyArray[MAX_LEVEL] = {1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 2, 1, 1, 2, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

#define COLOR_1 makeColorHSB( 30,200,255)  // SALMON
#define COLOR_2 makeColorHSB(255,200,255)  // PINK
#define COLOR_3 makeColorHSB(220,200,255)  // LIGHT PINK
#define COLOR_4 makeColorHSB(180,200,255)  // VIOLET
#define COLOR_5 makeColorHSB(150,200,255)  // INDIGO
#define COLOR_6 makeColorHSB(120, 50,255)  // PERIWINKLE/WHITE

Color petalColors[6] = {COLOR_1, COLOR_2, COLOR_3, COLOR_4, COLOR_5, COLOR_6};

byte rotationBri[6] = {0, 0, 0, 0, 0, 0};
byte rotationFace = 0;
Timer rotationTimer;
#define ROTATION_RATE 100

uint32_t timeOfBloom = 0;
bool wasCenterPossible = false;

byte stageOneData = 0;
byte stageTwoData = 0;

/*
   Communications
*/
enum comms {
  INERT,
  PUZZLE_AVAIL,
  PUZZLE_RECEIVED,
  PUZZLE_START,
  PUZZLE_START_RECEIVED,
  USER_SELECT,
  USER_SELECT_RECEIVED,
  USER_RESET,
  USER_RESET_RECEIVED,
  PUZZLE_CORRECT,
  PUZZLE_CORRECT_RECEIVED,
  PUZZLE_WRONG,
  PUZZLE_WRONG_RECEIVED,
  PUZZLE_WIN,
  PUZZLE_WIN_RECEIVED
};

byte faceComms[6] = {INERT, INERT, INERT, INERT, INERT, INERT};

Timer slowTimer;
#define FRAME_DURATION 200

// Bring f into range 0-facecount

byte normalizeFace( byte f) {
  while (f>FACE_COUNT) {
    f-=FACE_COUNT;  
  };
  return f;
}

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void scoreboardLoop();
void resetLoop();
void displayCenter();
void displayPetal();
void checkForReset(byte);
void startPuzzle(byte);
byte getCommsData(byte);
bool areAllFaces(byte);
void setAllFaces(byte);
uint16_t getPuzzleDuration(byte);
byte isCenterPossible();
byte getCenterFace();
byte determineStages(byte,byte,bool,byte);
uint16_t getDarkDuration(byte);
  
void loop() {
  // put your main code here, to run repeatedly:

  //    if (slowTimer.isExpired()) {
  //      slowTimer.set(FRAME_DURATION);

  switch (gameState) {

    case SETUP:
      setupLoop();
      break;
    case GAMEPLAY:
      gameplayLoop();
      break;
    case ANSWER:
      answerLoop();
      break;
    case SCOREBOARD:
      scoreboardLoop();
      break;
    case RESET:
      resetLoop();
      break;
    default:
      break;
  }

  switch (pieceType) {
    case CENTER:
      displayCenter();
      break;
    case PETAL:
      displayPetal();
      break;
  }

  // debug display
  //  displayDebug();

  // communication
  FOREACH_FACE(f) {
    byte data = ( faceComms[f] << 1 ) + ( pieceType );
    setValueSentOnFace(data, f);
  }

  // dump button presses
  buttonSingleClicked();
  buttonLongPressed();
  //    }
}


/*
   Setup Loop
*/
void setupLoop() {
  // listen for reset
  checkForReset(buttonLongPressed());

  if ( pieceType == CENTER ) {
    // listen for click to send puzzle
    if (buttonSingleClicked()) {
      startPuzzle(currentLevel);
    }
    // listen for all received puzzle
    // create a puzzle
    // and share that puzzle
    if (bSendPuzzle && !datagramTimer.isExpired()) {

      FOREACH_FACE(f) {

        if (!isValueReceivedOnFaceExpired(f)) { // neighbor present

          byte neighborVal = getCommsData(getLastValueReceivedOnFace(f)); // value received from neighbor

          if (neighborVal != PUZZLE_RECEIVED) { // not yet received the datagram

            faceComms[f] = PUZZLE_AVAIL;

            // send the datagram
            puzzleInfo.petalID = f;  // communicate which face this is

            if (f == answerFace) {
              puzzleInfo.isAnswer = 1;
              sendDatagramOnFace( &puzzleInfo, sizeof(puzzleInfo), f);
            } else {
              puzzleInfo.isAnswer = 0;
              sendDatagramOnFace( &puzzleInfo, sizeof(puzzleInfo), f);
            }

          }
        }
      }

      // check to see if we still need to send the puzzle
      if (areAllFaces(PUZZLE_RECEIVED)) {
        bSendPuzzle = false;
        setAllFaces(PUZZLE_START);
        puzzleTimer.set(getPuzzleDuration(currentLevel));
        gameState = GAMEPLAY;
      }

    } // end sending the puzzle


    // stop sending the puzzle start message after it has been received
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { // neighbor present
        byte neighborVal = getCommsData(getLastValueReceivedOnFace(f)); // value received from neighbor
        if (neighborVal == PUZZLE_START_RECEIVED) { // received the start
          faceComms[f] = INERT;
        }
      }
    }

  } // end pieceType == CENTER
  else if ( pieceType == PETAL ) {
    // listen for click to become the center (and send puzzle... this is the start of a new game)
    // listen for puzzle datagram, and enter gameplay
    if (isCenterPossible()) {
      if (buttonSingleClicked()) {
        pieceType = CENTER;
        startPuzzle(currentLevel);
      }
      if (!wasCenterPossible) {
        wasCenterPossible = true;
        timeOfBloom = millis();
      }
    }
    else {
      wasCenterPossible = false;
    }

    // determine centerFace
    centerFace = getCenterFace();

    if ( centerFace == FACE_COUNT ) { // NO CENTER FACE, DON'T DO ANYTHING
      return;
    }

    if (isDatagramReadyOnFace(centerFace)) {//is there a packet?

      if (getDatagramLengthOnFace(centerFace) == sizeof(puzzleInfo)) {//is it the right length?

        puzzleInfo = *( (puzzleInfo_t *) getDatagramOnFace(centerFace));//grab the data

        markDatagramReadOnFace(centerFace);

        faceComms[centerFace] = PUZZLE_RECEIVED;

        // Parse the data
        currentLevel = puzzleInfo.level; // set our current puzzle level
        // create puzzle state for stage one and stage two
        stageOneData = determineStages(puzzleInfo.type, puzzleInfo.difficulty, puzzleInfo.isAnswer, 1);
        stageTwoData = determineStages(puzzleInfo.type, puzzleInfo.difficulty, puzzleInfo.isAnswer, 2);
      }
    }

    // listen for start puzzle
    if ( getCommsData(getLastValueReceivedOnFace(centerFace)) == PUZZLE_START ) {
      if (puzzleTimer.isExpired()) {
        puzzleTimer.set(getPuzzleDuration(currentLevel));
        faceComms[centerFace] = PUZZLE_START_RECEIVED;
        gameState = GAMEPLAY;
        puzzleState = SHOW;
      }
    }
  } // end pieceType == PETAL
}


/*
   Gameplay Loop
*/
void gameplayLoop() {
  // listen for reset
  checkForReset(buttonLongPressed());

  if ( pieceType == CENTER ) {
    // listen for neighbor clicked
    // share result of the user selection with the group
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborVal = getCommsData(getLastValueReceivedOnFace(f));
        if (neighborVal == USER_SELECT) {
          faceComms[f] = USER_SELECT_RECEIVED;
          // this face was clicked on, was it correct?
          if (f == answerFace) {
            // let others know we are correct
            setAllFaces(PUZZLE_CORRECT);
          }
          else {
            // let others know we lost
            setAllFaces(PUZZLE_WRONG);
          }
        }
      }
    }

    // check to see if all received the correct message
    if (areAllFaces(PUZZLE_CORRECT_RECEIVED)) {
      setAllFaces(INERT);
      gameState = ANSWER;
      answerTimer.set(ANSWER_REVEAL_DURATION);
      puzzleState = CORRECT;
      // we passed this level let's increment
      currentLevel++;
    }

    // check to see if all received the incorrect message
    if (areAllFaces(PUZZLE_WRONG_RECEIVED)) {
      setAllFaces(INERT);
      gameState = ANSWER;
      answerTimer.set(ANSWER_REVEAL_DURATION);
      puzzleState = WRONG;
    }

  } // end pieceType == CENTER
  else if ( pieceType == PETAL ) {
    // show
    if ( puzzleTimer.getRemaining() > getDarkDuration(currentLevel) ) {
      puzzleState = SHOW;
    }
    // hide
    else if ( puzzleTimer.getRemaining() <= getDarkDuration(currentLevel) && !puzzleTimer.isExpired() ) {
      puzzleState = HIDE;
    }
    // wait
    else {
      puzzleState = WAIT;
    }
    // listen for user input
    // share user input with center
    // listen for result of input

    // determine centerFace
    centerFace = getCenterFace();

    if ( centerFace == FACE_COUNT ) {
      return;
      // not a legal petal... let's show this error state
    }

    bool acceptInput = true;

    // don't allow input when not in the WAIT state
    if (puzzleState != WAIT) {
      acceptInput = false;
    }

    if (acceptInput) {
      // listen for user input from a petal
      if (buttonSingleClicked()) {
        // am I correct or incorrect?
        faceComms[centerFace] = USER_SELECT;
      }
    }

    // listen for cue to go to answer state from center
    if (getCommsData(getLastValueReceivedOnFace(centerFace)) == PUZZLE_CORRECT) {
      faceComms[centerFace] = PUZZLE_CORRECT_RECEIVED;
    }
    else if (getCommsData(getLastValueReceivedOnFace(centerFace)) == PUZZLE_WRONG) {
      faceComms[centerFace] = PUZZLE_WRONG_RECEIVED;
    }


    // the center has validated that the answer was received on all faces, now go to answer loop
    if (faceComms[centerFace] == PUZZLE_CORRECT_RECEIVED) {
      if (getCommsData(getLastValueReceivedOnFace(centerFace)) == INERT) {
        faceComms[centerFace] = INERT;
        gameState = ANSWER;
        answerTimer.set(ANSWER_REVEAL_DURATION);
        puzzleState = CORRECT;
      }
    }
    else if (faceComms[centerFace] == PUZZLE_WRONG_RECEIVED) {
      if (getCommsData(getLastValueReceivedOnFace(centerFace)) == INERT) {
        faceComms[centerFace] = INERT;
        gameState = ANSWER;
        answerTimer.set(ANSWER_REVEAL_DURATION);
        puzzleState = WRONG;
      }
    }

  } // end pieceType == PETAL
}


/*
   Answer Loop
*/
void answerLoop() {
  // listen for reset
  checkForReset(buttonLongPressed());

  if ( pieceType == CENTER ) {
    // wait
    if ( answerTimer.isExpired() ) {

      if (puzzleState == CORRECT) {
        // go to setup if correct
        gameState = SETUP;
        puzzleState = WAIT;
        answerFace = FACE_COUNT;
        
        // initialize the puzzle
        clearPuzzleInfo();
       
      }
      else if (puzzleState == WRONG) {
        // go to scoreboard if incorrect
        gameState = SCOREBOARD;
        timeOfGameEnding = millis();
      }

    }

  } // end pieceType == PETAL
  else if ( pieceType == PETAL ) {
    // wait
    if ( answerTimer.isExpired() ) {

      if (puzzleState == CORRECT) {
        // go to setup if correct
        gameState = SETUP;
        puzzleState = WAIT;
      }
      else if (puzzleState == WRONG) {
        // go to scoreboard if incorrect
        gameState = SCOREBOARD;
        timeOfGameEnding = millis();
      }

    }

    // determine centerFace
    centerFace = getCenterFace();

  } // end pieceType == PETAL
}


/*
   Scoreboard Loop
*/
void scoreboardLoop() {
  // listen for reset
  checkForReset(buttonLongPressed() || buttonSingleClicked());

  if ( pieceType == CENTER ) {
  }
  else if ( pieceType == PETAL ) {
    // update/show score
  }
}


/*
   Reset Loop
*/
void resetLoop() {
  // doesn't mater if we are center or not, we all return to our initial states
  // initialize everything
  currentLevel = 0;

  clearPuzzleInfo();

  setAllFaces(INERT);
  puzzleState = WAIT;
  puzzleTimer.set(0);
  answerTimer.set(0);
  pieceType = PETAL;
  wasCenterPossible = false;
  stageOneData = 0;
  stageTwoData = 0;
  gameState = SETUP;
}

/*
   Check for Reset
*/
void checkForReset(byte triggered) {
  if ( pieceType == CENTER ) {

    if (!isCenterPossible()) { // only allow reset while configured completely
      return;
    }

    // listen for click
    if (triggered) {
      setAllFaces(USER_RESET);
    }

    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getCommsData(getLastValueReceivedOnFace(f)) == USER_RESET) {
          setAllFaces(USER_RESET);
        }
      }
    }

    // check to see if all received the reset message
    if (areAllFaces(USER_RESET_RECEIVED)) {
      setAllFaces(INERT);
      gameState = RESET;
    }
    //
    // go to setup
  } // end pieceType == PETAL
  else if ( pieceType == PETAL ) {
    // listen for click
    // go to setup

    // determine centerFace
    centerFace = getCenterFace();

    if (centerFace == FACE_COUNT) { // this piece is not attached correctly
      return;
    }

    if (triggered) {
      faceComms[centerFace] = USER_RESET;
    }

    if (getCommsData(getLastValueReceivedOnFace(centerFace)) == USER_RESET) {
      faceComms[centerFace] = USER_RESET_RECEIVED;
    }

    if ( faceComms[centerFace] == USER_RESET_RECEIVED ) {
      if (getCommsData(getLastValueReceivedOnFace(centerFace)) == INERT) {
        gameState = RESET;
      }
    }

  } // end pieceType == PETAL
}


/*
   returns true if this Blink is positioned and ready to be a center
*/
byte isCenterPossible() {

  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      return false; // missing neighbor, this is not a possible center
    }
  }

  return true;  // we looped through all neighbors without returning, great, we are a possible center
}

/*
   returns a duration in milliseconds for a given level
*/
uint16_t getPuzzleDuration(byte level) {


  return getShowDuration(level) + getDarkDuration(level);
}

/*
   returns a duration in milliseconds that the puzzle will show for
*/
uint16_t getShowDuration(byte level) {
  if (level < CURVE_BEGIN_LEVEL) {
    return MAX_SHOW_TIME; // max time is easier i.e. lower level
  }
  else if (level > CURVE_END_LEVEL) {
    return MIN_SHOW_TIME; // min time is harder i.e. higher level
  }
  else {
    // everything in between
    return MAX_SHOW_TIME - (level - CURVE_BEGIN_LEVEL) * (MAX_SHOW_TIME - MIN_SHOW_TIME) / ( CURVE_END_LEVEL - CURVE_BEGIN_LEVEL);
  }

}

/*
   returns a duration in milliseconds that the puzzle will show for
*/
uint16_t getDarkDuration(byte level) {

  if (level < CURVE_BEGIN_LEVEL) {
    return MIN_DARK_TIME; // max time is easier i.e. lower level
  }
  else if (level > CURVE_END_LEVEL) {
    return MAX_DARK_TIME; // min time is harder i.e. higher level
  }
  else {
    // everything in between
    return MIN_DARK_TIME + (level - CURVE_BEGIN_LEVEL) * (MAX_DARK_TIME - MIN_DARK_TIME) / ( CURVE_END_LEVEL - CURVE_BEGIN_LEVEL);
  }
}

/*
    Initializes everything needed to start a puzzle
*/
void startPuzzle(byte level) {
  createPuzzle(level);
  bSendPuzzle = true;
  datagramTimer.set(DATAGRAM_TIMEOUT);
}


/*
   Creates the puzzle for this level
*/
void createPuzzle(byte level) {

  answerFace = random(5);//which face will have the correct answer?

  //  lookup puzzle type
  puzzleInfo.type = puzzleArray[level];

  //  choose a puzzle palette
  puzzleInfo.palette = 0;//TODO: multiple palettes

  //  lookup puzzle difficulty
  puzzleInfo.difficulty = difficultyArray[level];

  //  set whether this is the answer face or not
  //  puzzleInfo[3] = 0;

  //  current level
  puzzleInfo.level = level;

  //  what face am I
  //  puzzleInfo[5] = 0;//this changes when I send it, default to 0 is fine

}

/*
    Determine the 1st and 2nd stage
    returns a value that can be used by the display function to display
    the correct puzzle pieces.
    NOTE: this is from the original repo and admittedly pretty spaghetti
*/
byte determineStages(byte puzzType, byte puzzDiff, bool amAnswer, byte stage) {
  if (stage == 1) {//determine the first stage - pretty much always a number 0-5, but in duoPetal it's a little more complicated
    if (puzzType == DUO_PETALS) {//special duo petal time!
      //choose a random interior color
      byte interior = random(5);
      //so based on the difficulty, we then choose another color
      byte distance = 5 - puzzDiff;

      byte exterior = 0;
      bool goRight = random(1);
      if (goRight) {
        exterior = normalizeFace(interior + distance);
      } else {
        exterior = normalizeFace(interior + 6 - distance);
      }
      return ((interior * 10) + exterior);
    } else if (puzzType == ROTATION_PETALS) {
      return (random(1));

    } else {//every other puzzle just chooses a random number 0-5
      return (random(5));
    }

  } else {//only change answer if amAnswer
    if (amAnswer) {//I gotta return a different value

      if (puzzType == DUO_PETALS) { //this is a duo petal, so we gotta reverse it
        byte newExterior = stageOneData / 10;
        byte newInterior = stageOneData % 10;
        return ((newInterior * 10) + newExterior);

      } else if (puzzType == ROTATION_PETALS) {//just swap from 0 to 1 and vice versa
        if (stageOneData == 0) {
          return (1);
        } else {
          return (0);
        }
      } else {//all other puzzles, just decide how far to rotate in the spectrum
        /*
           NOTE: difficulty distance should be thought of from the midpoint
           i.e. the easiest difference in color will be 180º away in the spectrum
           and the easiest difference in direction will be 180º opposite
        */
        byte distance = 5 - puzzDiff;
        bool goRight = random(1);
        if (goRight) {
          return normalizeFace(stageOneData + distance);
        } else {
          return normalizeFace(stageOneData + 6 - distance);
        }
      }
    } else {//if you are not the answer, just return the stage one data
      return (stageOneData);
    }
  }
}


/*
   Look at neighbors and determine the center face
*/
// TODO: Look into if this is too heavy and causing delay
//byte getCenterFace() {
//  return 3;
//}
byte getCenterFace() {

  // check to see if alone
  if (isAlone()) {
    centerFace = FACE_COUNT;
  }

  // no need to do this search unless we were alone
  // Note: this greatly speeds up the compute time
  // BUT, it also means that if pieces are rearranged
  // and never alone, they can have the incorrect center face
  if (centerFace != FACE_COUNT) {
    return centerFace;
  }

  // needs 3 adjacent faces
  // middle one is the center face
  byte numNeighbors = 0;
  bool neighborsPresent[6];

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { // neighbor present
      neighborsPresent[f] = true;
      numNeighbors++;
    }
    else {
      neighborsPresent[f] = false;
    }
  }

  if (numNeighbors != 3) {
    return FACE_COUNT;
  }
  else {
    // 3 neighbors verified, return the index of the middle one
    if (neighborsPresent[0] && neighborsPresent[1] && neighborsPresent[2]) {
      return 1;
    }
    else if (neighborsPresent[1] && neighborsPresent[2] && neighborsPresent[3]) {
      return 2;
    }
    else if (neighborsPresent[2] && neighborsPresent[3] && neighborsPresent[4]) {
      return 3;
    }
    else if (neighborsPresent[3] && neighborsPresent[4] && neighborsPresent[5]) {
      return 4;
    }
    else if (neighborsPresent[4] && neighborsPresent[5] && neighborsPresent[0]) {
      return 5;
    }
    else if (neighborsPresent[5] && neighborsPresent[0] && neighborsPresent[1]) {
      return 0;
    }
  }

  return FACE_COUNT;
}

/*
   All Face Comms
*/
void setAllFaces(byte val) {
  FOREACH_FACE(f) {
    faceComms[f] = val;
  }
}

/*
   Check if all faces are saying the same message
*/
bool areAllFaces(byte val) {
  FOREACH_FACE(f) {
    if (getCommsData(isValueReceivedOnFaceExpired(f))) {
      return false;
    }
    else {
      if (getCommsData(getLastValueReceivedOnFace(f)) != val) {
        return false;
      }
    }
  }
  return true;
}

/*
   Display Section
   ------------------------
   here we will display all of the beautiful flowers :)
*/
#define YELLOW_HUE 42
#define GREEN_HUE 77
/*
   Display Center
*/
void displayCenter() {

  if (gameState == SETUP) { // Center display during setup

    setColor(YELLOW);
    setColorOnFace(WHITE, random(5));
  }
  else if (gameState == GAMEPLAY) { // Center display during gameplay
    setColor(YELLOW);
  }
  else if (gameState == ANSWER) { // Center display during answer

    if ( puzzleState == CORRECT ) {
//      setColor(GREEN);
        byte hue = YELLOW_HUE + ( (GREEN_HUE - YELLOW_HUE) * answerTimer.getRemaining() ) / ANSWER_REVEAL_DURATION;
        byte sat = (255 * answerTimer.getRemaining()) / ANSWER_REVEAL_DURATION;

        setColor(makeColorHSB(hue, 255, 255));
        setColorOnFace(makeColorHSB(hue, sat, 255), random(5));
    }
    else if (puzzleState == WRONG ) {
      setColor(YELLOW);
    }
  }
  else if (gameState == SCOREBOARD) { // Center display during scoreboard
    setColor(YELLOW);
  }
  else if (gameState == RESET) { // Center display during reset
    //setColor(BLUE);
  }

  /*
    Display the missing pieces from the center
  */
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      setColorOnFace(dim(RED, sin8_C(millis() / 3)), f);
    }
  }

}

/*
   Display Petal
*/
void displayPetal() {

  if (gameState == SETUP) { // Petal display during setup

    if (centerFace != FACE_COUNT) {
      setColor(OFF);
      setColorOnFace(GREEN, centerFace);
    }
    else {
      setColor(GREEN);
    }

    if ( isCenterPossible() ) {
      uint32_t timeSinceBloom = millis() - timeOfBloom;

      if (timeSinceBloom > 2000) {
        setColor(YELLOW);
        setColorOnFace(WHITE, random(5));
      }
      else {
        byte hue = GREEN_HUE - ( (GREEN_HUE - YELLOW_HUE) * timeSinceBloom ) / 2000;
        byte bri = 100 + (155 * timeSinceBloom) / 2000;

        setColor(makeColorHSB(hue, 255, bri));
        setColorOnFace(dim(WHITE, bri), random(5));
      }
    }
  }
  else if (gameState == GAMEPLAY) { // Petal display during gameplay

    if ( puzzleState == SHOW ) {
      displayStage( stageOneData );
    }
    else if ( puzzleState == HIDE ) {
      setColor(OFF);
      setColorOnFace(dim(GREEN, 128), centerFace);
    }
    else if ( puzzleState == WAIT ) {
      displayStage( stageTwoData );
    }

  }
  else if (gameState == ANSWER) { // Petal display during answer
    if ( puzzleState == CORRECT ) {
      byte bri = (255 * answerTimer.getRemaining() / ANSWER_REVEAL_DURATION);
      setColor(dim(GREEN, bri));
    }
    else if (puzzleState == WRONG ) {
      
      if (!answerTimer.isExpired()) {
        // display the correct piece and fade out to reveal the gameboard
        byte bri = (255 * answerTimer.getRemaining() / ANSWER_REVEAL_DURATION);
        if (puzzleInfo.isAnswer) { // i was the correct answer
          setColor(dim(GREEN, bri));  // show Green for correct
        }
        else {
          setColor(dim(RED, bri)); // show Red for wrong
        }
      }
    }
  }
  else if (gameState == SCOREBOARD) { // Petal display during scoreboard

    petalID = puzzleInfo.petalID;
    setColor(OFF);
    displayScoreboard();  // WAY OVER MEMORY

    // display face IDs
//    FOREACH_FACE(f) {
//      if (f <= petalID) {
//        setColorOnFace(MAGENTA, f);
//      }
//    }
  }
  else if (gameState == RESET) { // Petal display during reset
    //setColor(BLUE);
  }
}

/*
   Display Stage
*/
void displayStage( byte stageData ) {

  switch (puzzleInfo.type) {

    case COLOR_PETALS:
      setColor(petalColors[stageData]);
      break;

    case LOCATION_PETALS:
      displayLocationPetal(stageData);
      break;

    case DUO_PETALS:
      displayDuoPetal((stageData / 10), (stageData % 10)); // interior, exterior
      break;

    case ROTATION_PETALS:
      displayRotationPetal(stageData);
      break;

    default:
      setColor(RED);  // IF WE SEE THIS: ERROR! HOW'D WE GET HERE?!
      break;
  }
}

/*
   Location Petals
   dark with a single direction illuminated
*/
void displayLocationPetal(byte dir) {
  setColor(OFF);
  setColorOnFace(WHITE, dir);
}

/*
   Duo Petals
   inner and outer are 2 different colors
*/
void displayDuoPetal(byte interior, byte exterior) {
  setColor(petalColors[interior]);//setting the interior color

  setColorOnFace( petalColors[exterior], normalizeFace(centerFace + 2));
  setColorOnFace(petalColors[exterior], normalizeFace(centerFace + 3));
  setColorOnFace(petalColors[exterior], normalizeFace(centerFace + 4));
}

/*
   Rotation Animation
   rotates CW or CCW with trail
*/
void displayRotationPetal(bool isCW) {

  if (rotationTimer.isExpired()) {

    rotationTimer.set(ROTATION_RATE);

    if (isCW) { // CW Rotation
      rotationFace = normalizeFace(rotationFace + 1);
    } else {  // CCW Rotation
      rotationFace = normalizeFace(rotationFace + 5);
    }
    rotationBri[rotationFace] = 255;
  }

  FOREACH_FACE(f) {
    if ( rotationBri[f] >= 5 ) {
      rotationBri[f] -= 5;
    }
    setColorOnFace(dim(WHITE, rotationBri[f]), f);
  }
}

void displayScoreboard() {

  numberOfRounds = (currentLevel) / PIP_IN_ROUND;
  numberOfPips = (currentLevel) % PIP_IN_ROUND; // CAREFUL: 0 pips means a single pip (index of 0), 5 pips means all 6 lit (index of 5)

  timeSinceScoreboardBegan = millis() - timeOfGameEnding;

  currentRound = timeSinceScoreboardBegan / roundDuration;

  if ( currentRound >= numberOfRounds ) {
    currentRound = numberOfRounds; // cap the rounds at the score
  }

  displayBackground();
  displayForeground();
}

/*
   Display the build of the rounds completed
*/

void displayBackground() {

  uint32_t timeSinceRoundBegan = timeSinceScoreboardBegan - (currentRound * roundDuration);  // time passed in this round

  // display background color on face based on how much time has passed
  FOREACH_FACE(f) {
    byte faceOffset = normalizeFace(f + centerFace + 2);

    if ( f == centerFace ) { // draw face touching the center
      if (puzzleInfo.isAnswer) { //i was the correct answer
        setColorOnFace(dim(GREEN, sin8_C(millis() / 4)), f); //pulse leaf
      }
      else {
        setColorOnFace(GREEN, f); // show leaf
      }
    }

    // only display the 3 pips in our petal
    if (f >= 3) {
      continue; // for the time being, let's only display on 0,1,2
    }

    uint32_t faceTime = f * PIP_DURATION_IN_ROUND; // after this amount of time has passed, draw on this pip
    uint32_t timeToDisplayPrevPetals = PIP_DURATION_IN_ROUND * (petalID * NUM_PIP_IN_PETAL);
    if ( timeSinceRoundBegan > ( faceTime + timeToDisplayPrevPetals ) ) {

      //      setColorOnFace(RED, f);
      switch (currentRound) {
        case 0: setColorOnFace(dim(RED, 200), faceOffset); break;
        case 1: setColorOnFace(dim(ORANGE, 200), faceOffset); break;
        case 2: setColorOnFace(dim(YELLOW, 200), faceOffset); break;
        case 3: setColorOnFace(dim(GREEN, 200), faceOffset); break;
        case 4: setColorOnFace(dim(BLUE, 200), faceOffset); break;
      }
    }
    else {
      // display the previous round (shift the cases
      switch (currentRound) {
        case 0: setColorOnFace(OFF, faceOffset); break;
        case 1: setColorOnFace(dim(RED, 200), faceOffset); break;
        case 2: setColorOnFace(dim(ORANGE, 200), faceOffset); break;
        case 3: setColorOnFace(dim(YELLOW, 200), faceOffset); break;
        case 4: setColorOnFace(dim(GREEN, 200), faceOffset); break;
        case 5: setColorOnFace(dim(BLUE, 200), faceOffset); break;
      }
    }
  }

}

/*
   Display the final score on the current round
*/
void displayForeground() {

  //  if( currentRound == numberOfRounds ) { // DO NOT USE - this is still drawing the background, don't want to begin yet

  byte nextRound = numberOfRounds + 1;  // since we are drawing this in our timeline after we painted the background for our current round

  uint32_t timeSincePipStarted = timeSinceScoreboardBegan - (nextRound * roundDuration);  // time passed in this round

  byte currentPip = timeSincePipStarted / PIP_DURATION_IN_SCORE;

  if (currentPip >= numberOfPips) {
    currentPip = numberOfPips;
  }

  if (timeSinceScoreboardBegan >= nextRound * roundDuration ) { // begins drawing after all backgrounds have been drawn

    // great, lets draw the pip to its final destination
    FOREACH_FACE(f) {
      byte faceOffset = normalizeFace(f + centerFace + 2);

      // only display the 3 pips in our petal
      if (f >= 3) {
        continue; // for the time being, let's only display on 0,1,2
      }

      uint32_t faceTime = f * PIP_DURATION_IN_SCORE; // after this amount of time has passed, draw on this pip
      uint32_t timeToDisplayPrevPetals = PIP_DURATION_IN_SCORE * (petalID * NUM_PIP_IN_PETAL);

      byte faceInEntireDisplay = f + (petalID * NUM_PIP_IN_PETAL);

      if ( timeSincePipStarted > (faceTime + timeToDisplayPrevPetals) && faceInEntireDisplay <= numberOfPips) {
        // able to display pip

        // if the front pip, pulse
        if ( faceInEntireDisplay == currentPip) {
          // go down and up once every pip duration
          // set the brightness based on the time passed during this pip display duration
          byte bri = sin8_C(map(timeSincePipStarted % PIP_DURATION_IN_SCORE, 0, PIP_DURATION_IN_SCORE, 0, 255)); // time passed in this current pip converted to 0-255
          setColorOnFace(dim(WHITE, bri), faceOffset);
        }
        // else stay iluminated
        else {
          setColorOnFace(WHITE, faceOffset);
        }
      }
    }
  }
}


/*
   DEBUG Display
*/

void displayDebug() {

  switch (gameState) {
    case SETUP:
      setColor(BLUE);
      break;
    case GAMEPLAY:
      setColor(WHITE);
      break;
    case ANSWER:
      setColor(GREEN);
      break;
    case SCOREBOARD:
      setColor(MAGENTA);
      break;
    case RESET:
      setColor(RED);
      break;
    default:
      break;
  }

  /*
     DEBUG Comms Display
  */
  switch (puzzleState) {
    case SHOW:
      setColorOnFace(MAGENTA, 0);
      break;
    case HIDE:
      setColorOnFace(OFF, 0);
      break;
    case WAIT:
      setColorOnFace(YELLOW, 0);
      break;
    case CORRECT:
      setColorOnFace(GREEN, 0);
      break;
    case WRONG:
      setColorOnFace(RED, 0);
      break;
    default:
      break;
  }

  if (pieceType == CENTER) {
    if (answerFace < FACE_COUNT) {
      setColorOnFace(GREEN, answerFace);
    }

    /*
      Display the missing pieces from the center
    */
    FOREACH_FACE(f) {
      if (isValueReceivedOnFaceExpired(f)) {
        setColorOnFace(dim(RED, sin8_C(millis() / 3)), f);
      }
    }

    if (gameState == SETUP) {
      setColor(BLUE);
      FOREACH_FACE(f) {
        if (f < normalizeFace(currentLevel)) {
          setColorOnFace(ORANGE, f);
        }
      }
    }

  }
}

/*
   Parallel Comms Data
*/

byte getPieceType(byte data) {
  return (data & 1);
}

byte getCommsData(byte data) {
  return ((data >> 1) & 31);
}
