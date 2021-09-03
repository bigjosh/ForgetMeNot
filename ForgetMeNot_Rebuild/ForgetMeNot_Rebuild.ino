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

enum puzzleStates {
  SHOW,
  HIDE,
  WAIT,
  CORRECT,
  WRONG,
  WIN,
  LOSE
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
byte puzzleInfo[6];
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
  PUZZLE_RECEIVED,
  PUZZLE_START,
  PUZZLE_START_RECEIVED,
  USER_SELECT,
  USER_SELECT_RECEIVED,
  USER_RESET,
  USER_RESET_RECEIVED,
  PUZZLE_END,
  PUZZLE_WIN
};

#define RESET_TIMEOUT 250
Timer resetTimer;

void setup() {
  // put your setup code here, to run once:
  randomize();
  reset();
}

void loop() {
  // put your main code here, to run repeatedly:

  switch (pieceType) {

    case CENTER:
      centerLoop();
      centerDisplay();
      break;

    case PETAL:
      petalLoop();
      petalDisplay();
      break;

    default:
      break;
  }

  // dump button presses
  buttonSingleClicked();
  buttonLongPressed();
}


/*
   Center update loop
*/
void centerLoop() {

  // basic reset from the center
  if (buttonLongPressed()) {
    // reset petals
    setValueSentOnAllFaces(USER_RESET);
    resetTimer.set(RESET_TIMEOUT);
  }

  if (resetTimer.isExpired()) {
    reset();
  }

  // create a puzzle
  // and share that puzzle
  if (bSendPuzzle && !datagramTimer.isExpired()) {

    FOREACH_FACE(f) {

      if (!isValueReceivedOnFaceExpired(f)) { // neighbor present

        byte neighborVal = getLastValueReceivedOnFace(f); // value received from neighbor

        if (neighborVal != PUZZLE_RECEIVED) { // not yet received the datagram

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
    if (didAllPetalReceivePuzzle()) {
      bSendPuzzle = false;
      setValueSentOnAllFaces(PUZZLE_START);
      puzzleTimer.set(getPuzzleDuration(currentLevel));
    }

  } // end sending the puzzle


  // stop sending the puzzle start message after it has been received
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { // neighbor present
      byte neighborVal = getLastValueReceivedOnFace(f); // value received from neighbor
      if (neighborVal == PUZZLE_START_RECEIVED) { // received the start
        setValueSentOnFace(INERT, f);
      }
    }
  }

  // let's handle the puzzle logic
  if (!puzzleTimer.isExpired()) {

    //let's demonstrate the puzzle
    if ( puzzleTimer.getRemaining() > getDarkDuration(currentLevel) ) {
      // we are in the show period
      puzzleState = SHOW;
    }
    else {
      // we are in the dark period
      puzzleState = HIDE;
    }
  }
  else {  // puzzle timer done, wait for input

    if (puzzleState == CORRECT || puzzleState == WRONG) {
      // show the answer
    }
    else {
      puzzleState = WAIT;
    }

    // listen for a response from a petal
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborVal = getLastValueReceivedOnFace(f);
        if (neighborVal == USER_SELECT) {
          // this face was clicked on, was it correct?
          if (f == answerFace) {
            puzzleState = CORRECT;
            // show answer
            // then get ready for next puzzle
          }
          else {
            // let others know we lost
            puzzleState = LOSE;
            setValueSentOnAllFaces(PUZZLE_END);
          }
        }
      }
    }
  }

}

/*
   Center display loop
*/

void centerDisplay() {
  setColor(ORANGE); // show we are the center piece

  switch (puzzleState) {
    case SHOW:
      setColorOnFace(BLUE, 0);
      break;
    case HIDE:
      setColorOnFace(OFF, 0);
      break;
    case WAIT:
      setColorOnFace(GREEN, 0);
      break;
    case CORRECT:
      setColor(WHITE);
      break;
    case WRONG:
      setColor(RED);
      break;
    case WIN:
      setColor(MAGENTA);
      break;
    case LOSE:
      setColor(RED);
      setColorOnFace(OFF, 0);
      setColorOnFace(OFF, 2);
      setColorOnFace(OFF, 4);
      break;
  }

  // Show missing pieces from center
  FOREACH_FACE(f) {
    if(isValueReceivedOnFaceExpired(f)) {
      setColorOnFace(dim(RED, sin8_C(millis()/3)), f);
    }
  }
}

/*
   Petal update loop
*/

void petalLoop() {

  if (isCenterPossible()) {
    if (buttonSingleClicked()) {
      pieceType = CENTER;
      startPuzzle(currentLevel);
    }
  }

  // determine centerFace
  centerFace = getCenterFace();
  bool acceptInput = true;

  // listen for puzzleInfo
  if (centerFace == FACE_COUNT) {
    // not a legal petal... let's show this error state
    acceptInput = false;
  }
  else {
    if (isDatagramReadyOnFace(centerFace)) {//is there a packet?

      if (getDatagramLengthOnFace(centerFace) == 6) {//is it the right length?

        byte *data = (byte *) getDatagramOnFace(centerFace);//grab the data

        for (byte i = 0; i < 6; i++) {
          puzzleInfo[i] = data[i];
        }

        markDatagramReadOnFace(centerFace);

        setValueSentOnFace(PUZZLE_RECEIVED, centerFace);

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
        setValueSentOnFace(PUZZLE_START_RECEIVED, centerFace);
      }
    }

    if ( getLastValueReceivedOnFace(centerFace) == USER_RESET ) {
      reset();
      setValueSentOnFace( USER_RESET_RECEIVED, centerFace);
    }

    if ( getLastValueReceivedOnFace(centerFace) == PUZZLE_END ) {
      puzzleState = LOSE;
    }

  }
  //
  if (!puzzleTimer.isExpired()) {

    //let's demonstrate the puzzle
    if ( puzzleTimer.getRemaining() > getDarkDuration(currentLevel) ) {
      // we are in the show period
      puzzleState = SHOW;
    }
    else {
      // we are in the dark period
      puzzleState = HIDE;
    }
  }
  else {  // puzzle timer done, wait for input
    if (puzzleState == CORRECT || puzzleState == WRONG || puzzleState == WIN || puzzleState == LOSE) {
      // show the answer
    }
    else {
      puzzleState = WAIT;
    }

    if (acceptInput) {
      // listen for user input from a petal
      if (buttonSingleClicked()) {
        // am I correct or incorrect?
        setValueSentOnFace( USER_SELECT, centerFace);

        if (puzzleInfo[3]) {
          puzzleState = CORRECT;
        }
        else {
          puzzleState = WRONG;
        }
      }
    }
  }
  // listen for user selection
}

/*
   Petal display loop
*/
void petalDisplay() {

  setColor(GREEN);  // by default, let's color petals green

  switch (puzzleState) {
    case SHOW:
      setColorOnFace(BLUE, 0);
      break;
    case HIDE:
      setColorOnFace(OFF, 0);
      break;
    case WAIT:
      setColorOnFace(GREEN, 0);
      break;
    case CORRECT:
      setColor(WHITE);
      break;
    case WRONG:
      setColor(RED);
      break;
    case WIN:
      setColor(MAGENTA);
      break;
    case LOSE:
      setColor(RED);
      setColorOnFace(OFF, 0);
      setColorOnFace(OFF, 2);
      setColorOnFace(OFF, 4);
      break;
  }

  if (isCenterPossible()) {
    setColor(YELLOW); // show we are possible center pieces
  }

  if(centerFace != FACE_COUNT) {
    setColorOnFace(OFF, centerFace);  //show center with off
  }

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
   Returns true when all petals have received the puzzle
*/
bool didAllPetalReceivePuzzle() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { // neighbor present
      if (getLastValueReceivedOnFace(f) != PUZZLE_RECEIVED) {
        return false;
      }
    }
  }

  return true;
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
}

/*
  Initialize variables to reset the Blink... just like when we boot up :)
*/
void reset() {
  pieceType = PETAL;
  currentLevel  = 0;
  puzzleTimer.set(0);
  puzzleState = WAIT;
  setValueSentOnAllFaces(INERT);
  resetTimer.never();
}
