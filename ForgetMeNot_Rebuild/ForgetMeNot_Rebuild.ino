/*
   Forget-me-not

   by Jeph Stahl
   Additional Development by Move38

*/

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
#define MAX_SHOW_TIME 4000    // 4 seconds on
#define MIN_SHOW_TIME 2000    // 2 seconds on
#define MIN_DARK_TIME 1000    // 1 second  off
#define MAX_DARK_TIME 2000    // 2 seconds off
#define CURVE_BEGIN_LEVEL 1
#define CURVE_END_LEVEL 10
int showTime = MAX_SHOW_TIME;
int darkTime = MIN_DARK_TIME;

Timer answerTimer; // runs the timing of displaying the answer
#define ANSWER_DURATION 2000  // 2 seconds

#define MAX_LEVEL 59
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
byte puzzleInfo[6] = {0, 0, 0, 0, 0, 0};
bool bSendPuzzle = false;
Timer datagramTimer;
#define DATAGRAM_TIMEOUT 1000
byte answerFace = FACE_COUNT;
byte centerFace = FACE_COUNT;

/*
   Puzzle Levels
*/
byte puzzleArray[60] =     {0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 2, 1, 0, 2, 3, 3, 2, 0, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
byte difficultyArray[60] = {1, 1, 1, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 2, 1, 1, 2, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

#define COLOR_1 makeColorHSB(220,200,255)  // LIGHTPINK
#define COLOR_2 makeColorHSB(255,200,255)  // SALMON
#define COLOR_3 makeColorHSB(220,200,255)  // PINK
#define COLOR_4 makeColorHSB(180,200,255)  // MAUVE
#define COLOR_5 makeColorHSB(150,200,255)  // INDIGO
#define COLOR_6 makeColorHSB(120, 50,255)  // PERIWINKLE

Color petalColors[6] = {COLOR_1, COLOR_2, COLOR_3, COLOR_4, COLOR_5, COLOR_6};


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

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {
  // put your main code here, to run repeatedly:

  //  if (slowTimer.isExpired()) {
  //    slowTimer.set(FRAME_DURATION);

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

  // debug display
  displayDebug();

  // communication
  FOREACH_FACE(f) {
    setValueSentOnFace(faceComms[f], f);
  }

  // dump button presses
  buttonSingleClicked();
  buttonLongPressed();
  //  }
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

          byte neighborVal = getLastValueReceivedOnFace(f); // value received from neighbor

          if (neighborVal != PUZZLE_RECEIVED) { // not yet received the datagram

            faceComms[f] = PUZZLE_AVAIL;

            // send the datagram
            puzzleInfo[5] = f;  // communicate which face this is

            if (f == answerFace) {
              puzzleInfo[3] = 1;
              sendDatagramOnFace( &puzzleInfo, sizeof(puzzleInfo), f);
            } else {
              puzzleInfo[3] = 0;
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
        byte neighborVal = getLastValueReceivedOnFace(f); // value received from neighbor
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
    }

    // determine centerFace
    centerFace = getCenterFace();

    if ( centerFace == FACE_COUNT ) { // NO CENTER FACE, DON'T DO ANYTHING
      return;
    }

    if (isDatagramReadyOnFace(centerFace)) {//is there a packet?

      if (getDatagramLengthOnFace(centerFace) == 6) {//is it the right length?

        byte *data = (byte *) getDatagramOnFace(centerFace);//grab the data

        for (byte i = 0; i < 6; i++) {
          puzzleInfo[i] = data[i];
        }

        markDatagramReadOnFace(centerFace);

        faceComms[centerFace] = PUZZLE_RECEIVED;

        // Parse the data
        currentLevel = puzzleInfo[4]; // set our current puzzle level
        // create puzzle state for stage one and stage two
        //        stageOneData = determineStages(puzzleInfo[0], puzzleInfo[2], puzzleInfo[3], 1);
        //        stageTwoData = determineStages(puzzleInfo[0], puzzleInfo[2], puzzleInfo[3], 2);
      }
    }

    // listen for start puzzle
    if ( getLastValueReceivedOnFace(centerFace) == PUZZLE_START ) {
      if (puzzleTimer.isExpired()) {
        puzzleTimer.set(getPuzzleDuration(currentLevel));
        faceComms[centerFace] = PUZZLE_START_RECEIVED;
        gameState = GAMEPLAY;
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
        byte neighborVal = getLastValueReceivedOnFace(f);
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
      answerTimer.set(ANSWER_DURATION);
      puzzleState = CORRECT;
      // we passed this level let's increment
      currentLevel++;
    }

    // check to see if all received the incorrect message
    if (areAllFaces(PUZZLE_WRONG_RECEIVED)) {
      setAllFaces(INERT);
      gameState = ANSWER;
      answerTimer.set(ANSWER_DURATION);
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
    if (getLastValueReceivedOnFace(centerFace) == PUZZLE_CORRECT) {
      faceComms[centerFace] = PUZZLE_CORRECT_RECEIVED;
    }
    else if (getLastValueReceivedOnFace(centerFace) == PUZZLE_WRONG) {
      faceComms[centerFace] = PUZZLE_WRONG_RECEIVED;
    }


    // the center has validated that the answer was received on all faces, now go to answer loop
    if (faceComms[centerFace] == PUZZLE_CORRECT_RECEIVED) {
      if (getLastValueReceivedOnFace(centerFace) == INERT) {
        faceComms[centerFace] = INERT;
        gameState = ANSWER;
        answerTimer.set(ANSWER_DURATION);
        puzzleState = CORRECT;
      }
    }
    else if (faceComms[centerFace] == PUZZLE_WRONG_RECEIVED) {
      if (getLastValueReceivedOnFace(centerFace) == INERT) {
        faceComms[centerFace] = INERT;
        gameState = ANSWER;
        answerTimer.set(ANSWER_DURATION);
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
        for (byte i = 0; i < 6; i++) { // initialize the puzzle
          puzzleInfo[i] = 0;
        }
      }
      else if (puzzleState == WRONG) {
        // go to scoreboard if incorrect
        gameState = SCOREBOARD;
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
  for (byte i = 0; i < 6; i++) { // initialize the puzzle
    puzzleInfo[i] = 0;
  }
  setAllFaces(INERT);
  puzzleState = WAIT;
  puzzleTimer.set(0);
  answerTimer.set(0);
  pieceType = PETAL;
  gameState = SETUP;
}

/*
   Check for Reset
*/
void checkForReset(bool triggered) {
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
        if (getLastValueReceivedOnFace(f) == USER_RESET) {
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

    if (getLastValueReceivedOnFace(centerFace) == USER_RESET) {
      faceComms[centerFace] = USER_RESET_RECEIVED;
    }

    if ( faceComms[centerFace] == USER_RESET_RECEIVED ) {
      if (getLastValueReceivedOnFace(centerFace) == INERT) {
        gameState = RESET;
      }
    }

  } // end pieceType == PETAL
}


/*
   returns true if this Blink is positioned and ready to be a center
*/
bool isCenterPossible() {

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
  puzzleInfo[0] = puzzleArray[level];

  //  choose a puzzle palette
  puzzleInfo[1] = 0;//TODO: multiple palettes

  //  lookup puzzle difficulty
  puzzleInfo[2] = difficultyArray[level];

  //  set whether this is the answer face or not
  //  puzzleInfo[3] = 0;

  //  current level
  puzzleInfo[4] = level;

  //  what face am I
  //  puzzleInfo[5] = 0;//this changes when I send it, default to 0 is fine

}

/*
   Look at neighbors and determine the center face
*/
byte getCenterFace() {
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
    if (isValueReceivedOnFaceExpired(f)) {
      return false;
    }
    else {
      if (getLastValueReceivedOnFace(f) != val) {
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

/*
   Display Center
*/
void displayCenter() {

}

/*
   Display Petals
*/
void displayPetals() {

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
        if (f < currentLevel % 6) {
          setColorOnFace(ORANGE, f);
        }
      }
    }

  }
}
