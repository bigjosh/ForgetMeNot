/*
   Forget-me-not

   by Jeph Stahl
   Additional Development by Move38

*/

enum gameStates {SETUP, CENTER, SENDING, WAITING, PLAYING_PUZZLE, PLAYING_PIECE, ERR};
byte gameState = SETUP;

bool firstPuzzle = false;

enum answerStates {INERT, CORRECT, WRONG, RESOLVE, VICTORY};
byte answerState = INERT;

byte centerFace = 0;

byte puzzlePacket[6] = {0, 0, 0, 0, 0, 0};  // TODO: is there a reason this isn's just the PuzzleInfo?

#define MAX_LEVEL 72
byte currentPuzzleLevel = 0;
Timer puzzleTimer;
bool puzzleStarted = false;
Timer answerTimer;

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

uint32_t timeOfGameEnding;
uint32_t timeSinceScoreboardBegan;

byte petalID;

#define ANSWER_REVEAL_DURATION 2000
// ----------------------------------------


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

bool canBloom = false;
Timer bloomTimer;
#define BLOOM_TIME 1000
#define GREEN_HUE 77
#define YELLOW_HUE 42

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

enum puzzleType {
  COLOR_PETALS,       // Each petal is a single color
  LOCATION_PETALS,    // Each petal is lit a single direction
  DUO_PETALS,         // Each petal is a pair of colors
  ROTATION_PETALS     // Each petal animates rotation CW or CCW
};


Timer datagramTimer;
#define DATAGRAM_TIMEOUT 250
byte puzzleInfo[6] = {0, 0, 0, 0, 0, 0};
byte stageOneData = 0;
byte stageTwoData = 0;
byte answerFace = 0;


Timer slowTimer;
#define FRAME_DELAY 10

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {

//  if (slowTimer.isExpired()) {
//    slowTimer.set(FRAME_DELAY);

    switch (gameState) {
      case SETUP:
        setupLoop();
        setupDisplay();
        break;
      case CENTER:
      case SENDING:
      case PLAYING_PUZZLE:
        centerLoop();
        centerDisplay();
        break;
      case WAITING:
      case PLAYING_PIECE:
        pieceLoop();
        pieceDisplay();
        break;
      case ERR:
        break;
    }

    answerLoop();

    //do communication
    byte sendData = (gameState << 3) | (answerState);
    setValueSentOnAllFaces(sendData);

    //dump button presses
    buttonSingleClicked();
    buttonDoubleClicked();
    buttonMultiClicked();
//  }
}

void setupLoop() {
  bool emptyNeighbor = false;
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {//no neighbor
      emptyNeighbor = true;
    } else {
      if (getGameState(getLastValueReceivedOnFace(f)) == SENDING || getGameState(getLastValueReceivedOnFace(f)) == CENTER) {//this neighbor is telling me to play the game
        gameState = WAITING;
        centerFace = f;
      }
    }
  }

  if (emptyNeighbor == true) {
    canBloom = false;
  } else {
    if (canBloom == false) {
      bloomTimer.set(BLOOM_TIME);
      canBloom = true;
    }
  }

  if (canBloom) {
    if (buttonSingleClicked()) {
      //react differently if you're a scoreboard
      if (isScoreboard) {
        currentPuzzleLevel = 0; // Reset the score, once we click the scoreboard
        gameState = CENTER;
        firstPuzzle = false;
      } else {
        gameState = CENTER;
        firstPuzzle = true;
      }
    }
  }
}


void centerLoop() {
  if (gameState == CENTER) {
    //here we just wait for clicks to launch a new puzzle
    if (buttonSingleClicked() || firstPuzzle) {
      gameState = SENDING;
      generatePuzzle();
      firstPuzzle = false;
      datagramTimer.set(DATAGRAM_TIMEOUT);
    }
  } else if (gameState == SENDING) {
    //here we just wait for all neighbors to go into PLAYING_PIECE

    byte piecesPlaying = 0;
    byte whoPlaying[6] = {false, false, false, false, false, false};
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//a neighbor! this actually needs to always be true, or else we're in trouble
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getGameState(neighborData) == PLAYING_PIECE) {
          piecesPlaying++;
          whoPlaying[f] = true;
        }
      }
    }

    if (piecesPlaying == 6) {//all of the pieces have gone into playing, so can we
      gameState = PLAYING_PUZZLE;
    }

    if (datagramTimer.isExpired()) {
      //huh, so we still aren't playing
      //who needs a datagram again?
      FOREACH_FACE(f) {
        if (whoPlaying[f] == false) {
          //update puzzlePacket[5] to reflect the current face
          puzzlePacket[5] = f;

          if (f == answerFace) {
            puzzlePacket[3] = 1;
            sendDatagramOnFace( &puzzlePacket, sizeof(puzzlePacket), f);
          } else {
            puzzlePacket[3] = 0;
            sendDatagramOnFace( &puzzlePacket, sizeof(puzzlePacket), f);
          }
        }
      }
    }

  } else if (gameState == PLAYING_PUZZLE) {
    //so in here, we just kinda hang out and wait to do... something?
    //I guess here we just listen for RIGHT/WRONG signals?
    //and I guess eventually ERROR HANDLING

    //TURN THIS BACK ON TO GET THE DOUBLE-CLICK CHEAT
    //    if (buttonDoubleClicked()) {//here we reveal the correct answer and move forward
    //      answerState = CORRECT;
    //      answerTimer.set(ANSWER_REVEAL_DURATION);   //set answer timer for display
    //      gameState = CENTER;
    //    }


  }
}



void generatePuzzle() {

  // based on LEVEL, create a puzzle
  //  choose{puzzleType, puzzlePalette, puzzleDifficulty, isAnswer, showTime, darkTime};

  //  lookup puzzle type
  puzzlePacket[0] = puzzleArray[currentPuzzleLevel];

  //  choose a puzzle palette
  puzzlePacket[1] = 0;//TODO: multiple palettes

  //  lookup puzzle difficulty
  puzzlePacket[2] = difficultyArray[currentPuzzleLevel];

  //  current level
  puzzlePacket[4] = currentPuzzleLevel;

  //  what face am I
  puzzlePacket[5] = 0;//this changes when I send it, default to 0 is fine

  answerFace = random(5);//which face will have the correct answer?

  FOREACH_FACE(f) {
    //update puzzlePacket[5] to reflect the current face
    puzzlePacket[5] = f;
    if (f == answerFace) {
      puzzlePacket[3] = 1;  // isAnswer = true
      sendDatagramOnFace( &puzzlePacket, sizeof(puzzlePacket), f);
    } else {
      puzzlePacket[3] = 0; // is Answer = false;
      sendDatagramOnFace( &puzzlePacket, sizeof(puzzlePacket), f);
    }
  }
}

void pieceLoop() {
  if (gameState == WAITING) {//check for datagrams, then go into playing
    //listen for a packet on master face
    puzzleStarted = false;

    if (isDatagramReadyOnFace(centerFace)) {//is there a packet?

      if (getDatagramLengthOnFace(centerFace) == 6) {//is it the right length?

        byte *data = (byte *) getDatagramOnFace(centerFace);//grab the data

        for (byte i = 0; i < 6; i++) {
          puzzleInfo[i] = data[i];
        }

        markDatagramReadOnFace(centerFace);

        gameState = PLAYING_PIECE;

        // Parse the data
        currentPuzzleLevel = puzzleInfo[4]; // set our current puzzle level
        // create puzzle state for stage one and stage two
        stageOneData = determineStages(puzzleInfo[0], puzzleInfo[2], puzzleInfo[3], 1);
        stageTwoData = determineStages(puzzleInfo[0], puzzleInfo[2], puzzleInfo[3], 2);
      }
    }


  } else if (gameState == PLAYING_PIECE) {//I guess just listen for clicks and signals?

    //start the puzzle if the center wants me to start
    if (puzzleTimer.isExpired() && getGameState(getLastValueReceivedOnFace(centerFace)) == PLAYING_PUZZLE && puzzleStarted == false) {//I have not started the puzzle, but the center wants me to
      //BEGIN SHOWING THE PUZZLE!

      //Ok, so this
      //puzzleTimer.set((puzzleInfo[4] + puzzleInfo[5]) * 100); //the timing within the datagram is reduced 1/100
      puzzleTimer.set(7000);//TODO: this needs to change based on level
      puzzleStarted = true;
      rotationFace = centerFace;
    }
    
    bool isAllowedToAnswer = puzzleTimer.isExpired(); 
    
    if (buttonSingleClicked() && isAllowedToAnswer) {
      //is this right or wrong?
      bool isCorrect = puzzleInfo[3];

      if (isCorrect) {
        answerState = CORRECT;
        answerTimer.set(ANSWER_REVEAL_DURATION);   //set answer timer for display
        gameState = WAITING;

        //if you are at MAX_LEVEL, you should go into a special kind of correct - VICTORY
        if (puzzleInfo[4] == (MAX_LEVEL-1)) {
          answerState = VICTORY;
        }

      } else {
        answerState = WRONG;
        gameState = SETUP;  // return to setup after wrong
        isScoreboard = true;
        timeOfGameEnding = millis();
      }
    }
  }

}

byte determineStages(byte puzzType, byte puzzDiff, byte amAnswer, byte stage) {
  if (stage == 1) {//determine the first stage - pretty much always a number 0-5, but in duoPetal it's a little more complicated
    if (puzzType == DUO_PETALS) {//special duo petal time!
      //choose a random interior color
      byte interior = random(5);
      //so based on the difficulty, we then choose another color
      byte distance = 5 - puzzDiff;

      byte exterior = 0;
      bool goRight = random(1);
      if (goRight) {
        exterior = (interior + distance) % 6;
      } else {
        exterior = (interior + 6 - distance) % 6;
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
          return ((stageOneData + distance) % 6);
        } else {
          return ((stageOneData + 6 - distance) % 6);
        }
      }
    } else {//if you are not the answer, just return the stage one data
      return (stageOneData);
    }
  }
}

void answerLoop() {
  //so we gotta just listen around for all these signals
  if (answerState == INERT) {//listen for CORRECT or WRONG
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborAnswer = getAnswerState(getLastValueReceivedOnFace(f));
        if (neighborAnswer == CORRECT) {
          answerState = neighborAnswer;
          answerTimer.set(2000);

          if (gameState == PLAYING_PIECE) {
            gameState = WAITING;
          } else if (gameState == PLAYING_PUZZLE) {
            gameState = CENTER;
            currentPuzzleLevel++;
          }
        } else if (neighborAnswer == WRONG) {
          answerState = neighborAnswer;
          gameState = SETUP;
          isScoreboard = true;
          timeOfGameEnding = millis();
        } else if (neighborAnswer == VICTORY) {
          answerState = neighborAnswer;
          gameState = SETUP;
          isScoreboard = true;
          // TODO: maybe figure out something for a timed animation
        }
      }
    }
  } else if (answerState == CORRECT || answerState == WRONG || answerState == VICTORY) {//just wait to go to RESOLVE

    if (answerState == CORRECT) {
      if (gameState == PLAYING_PIECE) {
        gameState = WAITING;
      } else if (gameState == PLAYING_PUZZLE) {
        gameState = CENTER;
      }
    } else if (answerState == WRONG) {
      gameState = SETUP;
    } else if (answerState == VICTORY) {
      gameState = SETUP;
    }

    bool canResolve = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborAnswer = getAnswerState(getLastValueReceivedOnFace(f));
        if (neighborAnswer == INERT) {
          canResolve = false;
        }
      }
    }

    if (canResolve) {
      answerState = RESOLVE;
    }
  } 
  
  else if (answerState == RESOLVE) {//wait to go to INERT

    if (answerState == CORRECT) {
      if (gameState == PLAYING_PIECE) {
        gameState = WAITING;
      } else if (gameState == PLAYING_PUZZLE) {
        gameState = CENTER;
      }
    } else if (answerState == WRONG) {
      gameState = SETUP;
    } else if (answerState == VICTORY) {
      gameState = SETUP;
    }

    bool canInert = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte neighborAnswer = getAnswerState(getLastValueReceivedOnFace(f));
        if (neighborAnswer != INERT && neighborAnswer != RESOLVE) {
          canInert = false;
        }
      }
    }

    if (canInert) {
      answerState = INERT;
    }
  }
}

////DISPLAY FUNCTIONS

void setupDisplay() {
  if (canBloom) { // center piece

    byte bloomProgress = map(bloomTimer.getRemaining(), 0, BLOOM_TIME, 0, 255);

    //    byte bloomHue = map(bloomProgress, 0, 255, YELLOW_HUE, GREEN_HUE);
    //    byte bloomBri = map(255 - bloomProgress, 0, 255, 100, 255);

    byte bloomHue = YELLOW_HUE;
    byte bloomBri = 255;

    setColor(makeColorHSB(bloomHue, 255, bloomBri));
    setColorOnFace(dim(WHITE, bloomBri), random(5));
  }
  else {  // not the center piece
    setColor(makeColorHSB(GREEN_HUE, 255, 100));
  }

  if (isScoreboard) {

    if (puzzleInfo[4] == (MAX_LEVEL-1)) { //oh, this is a VICTORY scoreboard
      setColor(MAGENTA);
    } else {//a regular failure scoreboard

      if (canBloom) {
        // do something special on the middle piece?
        setColor(makeColorHSB(YELLOW_HUE, 255, 255));
      }
      else { // I am a petal, show the score on me
        petalID = puzzleInfo[5];
        setColor(OFF);
        displayScoreboard();
      }
    }
  }
}


void displayScoreboard() {

  numberOfRounds = (currentPuzzleLevel) / PIP_IN_ROUND;
  numberOfPips = (currentPuzzleLevel) % PIP_IN_ROUND; // CAREFUL: 0 pips means a single pip (index of 0), 5 pips means all 6 lit (index of 5)

  uint32_t timeSinceGameEnded = millis() - timeOfGameEnding;

  if (timeSinceGameEnded < ANSWER_REVEAL_DURATION) {
    // display the correct piece and fade out to reveal the gameboard
    byte bri = 255 - (255 * timeSinceGameEnded / ANSWER_REVEAL_DURATION);
    if (puzzleInfo[3]) { // i was the correct answer
      setColor(dim(GREEN, bri));  // show Green for correct
    }
    else {
      setColor(dim(RED, bri)); // show Red for wrong
    }
  }
  else {
    timeSinceScoreboardBegan = millis() - (timeOfGameEnding + ANSWER_REVEAL_DURATION);

    currentRound = timeSinceScoreboardBegan / roundDuration;

    if ( currentRound >= numberOfRounds ) {
      currentRound = numberOfRounds; // cap the rounds at the score
    }

    displayBackground();
    displayForeground();
  }
}

/*
   Display the build of the rounds completed
*/

void displayBackground() {

  uint32_t timeSinceRoundBegan = timeSinceScoreboardBegan - (currentRound * roundDuration);  // time passed in this round

  // display background color on face based on how much time has passed
  FOREACH_FACE(f) {
    byte faceOffset = (f + centerFace + 2) % 6;

    if ( f == centerFace ) { // draw face touching the center
      if (puzzleInfo[3]) { //i was the correct answer
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
      byte faceOffset = (f + centerFace + 2) % 6;

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



void centerDisplay() {
  //so we need some temp graphics
  switch (gameState) {
    case CENTER:
      {
        if (!answerTimer.isExpired()) {
          if (answerState == CORRECT) {
            setColor(GREEN);
          } else if (answerState == WRONG) {
            setColor(RED);
          }
        }
        else {
          setColor(YELLOW);
          setColorOnFace(WHITE, random(5));
        }
      }
      break;
    case SENDING:
      setColor(YELLOW); // CHANGE FOR DEBUG
      break;
    case PLAYING_PUZZLE:
      setColor(YELLOW); // CHANGE FOR DEBUG
      //setColorOnFace(WHITE, answerFace);//DEBUG MODE - INDICATING ANSWER
      break;
  }
  //setColor(makeColorHSB(YELLOW_HUE, 255, 255));
  //setColorOnFace(makeColorHSB(YELLOW_HUE, 0, 255), random(5));
}

void pieceDisplay() {

    if (gameState == WAITING) {//just waiting
  
      //setColor(OFF);
      //setColorOnFace(GREEN, centerFace);
      if (!answerTimer.isExpired()) {
        if (answerState == CORRECT) {
          setColor(GREEN);
        } else if (answerState == WRONG) {
          setColor(RED);
        }
      } else {
        setColor(OFF);
        setColorOnFace(GREEN, centerFace);
      }
  
  
    } else {//show the puzzle
      if (puzzleStarted) {
        if (puzzleTimer.isExpired()) {//show the last stage of the puzzle (forever)
          displayStage(stageTwoData);
        } else if (puzzleTimer.getRemaining() <= 2000) { //show darkness TODO: this should change with each level like the initial setting
          setColor(OFF);
          setColorOnFace(dim(GREEN, 100), centerFace);
        } else {//show the first stage of the puzzle
          displayStage(stageOneData);
        }
      } else {
        setColor(OFF);
        setColorOnFace(dim(GREEN, 100), centerFace);
      }
    }

  /*
     DEBUG GRAPHICS
     ------------------------
  */
//  switch (gameState) {
//    case WAITING:
//      setColor(YELLOW);
//      break;
//    case PLAYING_PIECE:
//      setColor(GREEN);
//      break;
//    default:
//      break;
//  }
//
//  if (puzzleStarted) {
//    setColorOnFace(WHITE, centerFace);
//  } else {
//    setColorOnFace(RED, centerFace);
//  }
  /* ------------------------ */

  /*
     DEBUG ANSWER GRAPHICS
     ------------------------
  */
  //  byte oppFace = (centerFace + 3) % 6;
  //  switch (answerState) {
  //    case INERT:
  //      setColorOnFace(WHITE, oppFace);
  //      break;
  //    case CORRECT:
  //      setColorOnFace(GREEN, oppFace);
  //      break;
  //    case WRONG:
  //      setColorOnFace(RED, oppFace);
  //      break;
  //    case RESOLVE:
  //      setColorOnFace(BLUE, oppFace);
  //      break;
  //  }
}

void displayStage( byte stageData ) {
  //TODO: take into account color palette, defaulting to basics for now
  //puzzleType, puzzlePalette, puzzleDifficulty, isAnswer, showTime, darkTime
  switch (puzzleInfo[0]) {
    case COLOR_PETALS:
      setColor(petalColors[stageData]);
      break;
    case LOCATION_PETALS://dark, with a single lit face
      setColor(OFF);
      setColorOnFace(WHITE, stageData);
      break;
    case DUO_PETALS:
      {
        byte interiorColor = (stageData / 10);
        setColor(petalColors[interiorColor]);//setting the interior color

        byte exteriorColor = (stageData % 10);
        setColorOnFace(petalColors[exteriorColor], (centerFace + 2) % 6);
        setColorOnFace(petalColors[exteriorColor], (centerFace + 3) % 6);
        setColorOnFace(petalColors[exteriorColor], (centerFace + 4) % 6);
      }
      break;
    case ROTATION_PETALS:
      { //I need to do this because I'm gonna make a byte
        if (rotationTimer.isExpired()) {
          rotationTimer.set(ROTATION_RATE);
          if (stageData == 0) { // CW Rotation
            rotationFace = (rotationFace + 1) % 6;
          } else {  // CCW Rotation
            rotationFace = (rotationFace + 5) % 6;
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
      break;
  }
}

////CONVENIENCE FUNCTIONS

byte getGameState(byte data) {
  return (data >> 3);//returns the 1st, 2nd, and 3rd bit
}

byte getAnswerState(byte data) {
  return (data & 7);//returns the 4th, 5th and 6th bit
}
