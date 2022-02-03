#include <Wire.h>
#include <TimerEvent.h>
#include <LiquidCrystal.h>
#include <Pushbutton.h>

#define PLAY_PAUSE_SET_PIN 14
#define INCREMENT_PIN 16
#define DECREMENT_PIN 15

// Use 2k Resistor for brightness
LiquidCrystal lcd (9, 11, 5, 4, 3, 2);

Pushbutton play_pause_set_btn (PLAY_PAUSE_SET_PIN);
Pushbutton increment_btn (INCREMENT_PIN);
Pushbutton decrement_btn (DECREMENT_PIN);


int PCB_ADDR [128];
int Master_Stage = 1;

byte Message;
int intMessage;
byte UART_start = 1;
byte UART_ack = 255;

int final_address;

int increment = 1;
int counter = 0;

// Calculated Time Variables
int delayTime;
long finishTime;
const long timeBetweenLoops_UL = 300000;
const long timeBetweenLoops_LL = 0;
long timeBetweenLoops = 3000;


// Timing Variables
int seconds = 0;
int minutes = 0;
int hours = 0;
int days = 0;

// Train length and speeds with limits
const int trainSpeed_UL = 200;
const int trainSpeed_LL = 2;
int trainSpeed = 20;

// Train length upper and lower limits and its original standard length set to 500ft
double trainLength_UL = 264000;
double trainLength_LL = 50;
double trainLength_ft = 500;
double trainLength_m = trainLength_ft / 5280;

unsigned long startTime;
unsigned long Runtime;

// Train counter and pinout array
int frontOfTrain_counter;
int backOfTrain_counter;
byte FDATA[2];
byte BDATA[2];
int F_address;
int B_address;
int pinoutArrayWest [] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
int pinoutArrayEast [] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
long currentLoop = 1;
bool arePCBsConnected = true;
bool activateLooping = true;
bool travelWest = true;

// Drawn infinity symbol
byte infinityChar[] = {
  B00000,
  B00000,
  B01010,
  B10101,
  B10101,
  B01010,
  B00000,
  B00000
};
bool pauseSimulation = false;

// Threads
TimerEvent sequencingThread;
TimerEvent timerThread;



// Clears a section of the LCD screen
void clearRowSection(int startColumn, int endColumn, int row) {
  lcd.setCursor(startColumn, row);
  String clearingSpace = "";
  for (int i = startColumn; i < endColumn; i++) {
    clearingSpace.concat(" ");
  }
  lcd.print(clearingSpace);
  lcd.setCursor(startColumn, row);
}


// Converts feet to miles
double ft2miles(double ft) {
  return (double)ft / 5280;
}


// Confirms with user if the number of PCBs
void confirmPCBs() {
  lcd.clear();
  // Are FINAL_ADDRESS(x) connected
  if (final_address == 1){
    lcd.print("Is 1 PCB linked?");
  }else{
    lcd.print("Are ");
    lcd.print(final_address);
    lcd.print(" PCBs linked?");
  }  
  lcd.setCursor(0, 1);
  lcd.print("Yes -> Press 'Play'");
  lcd.setCursor(0, 2);
  lcd.print("No  -> Press 'Reset'");

  play_pause_set_btn.waitForButton();
}


// Short 4 second intro screen
void introScreen(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("  What's good CPR?  ");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("     Welcome to     ");
  lcd.setCursor(0,2);
  lcd.print(" THE RAIL SIMULATOR ");
  delay(2000);
}


// Ask user which direction they would like the train to travel
void confirmTrainDirection(){
  lcd.clear();
  lcd.print("Travel direction:");
  lcd.setCursor(0, 1);
  lcd.print("West -> Press '+'");
  lcd.setCursor(0, 2);
  lcd.print("East -> Press '-'");
  lcd.setCursor(0, 3);
  lcd.print("Selected: West");
  // Use increment button to set to west, decrement button to set to east, and play/pause button to continue
  while (!play_pause_set_btn.getSingleDebouncedRelease()) {
    if (increment_btn.isPressed()) {
      travelWest = true;
      clearRowSection(10, 16, 3);
      lcd.print("West");
    } else if (decrement_btn.isPressed()) {
      travelWest = false;
      clearRowSection(10, 16, 3);
      lcd.print("East");
    }
  }
}

// Ask user if they want the train to keep looping through the track
void checkForLooping() {
  lcd.clear();
  lcd.print("Activate Looping:");
  lcd.setCursor(0, 1);
  lcd.print("Yes -> Press '+'");
  lcd.setCursor(0, 2);
  lcd.print("No  -> Press '-'");
  lcd.setCursor(0, 3);
  lcd.print("Selected: Yes");

  // Use increment button to set looping to true, decrement button to set looping to false, and play/pause button to continue
  while (!play_pause_set_btn.getSingleDebouncedRelease()) {
    if (increment_btn.isPressed()) {
      activateLooping = true;
      clearRowSection(10, 16, 3);
      lcd.print("Yes");
    } else if (decrement_btn.isPressed()) {
      activateLooping = false;
      clearRowSection(10, 16, 3);
      lcd.print("No");
    }
  }

}

// Ask user how much time they want to wait between train loops through the track again
// Implements an increasing incrementing algorithm while the button is being held down
void checkTimeBetweenLooping() {
  if (activateLooping) {
    increment = 1;
    counter = 0;
    lcd.clear();
    lcd.print("Time between loops:");
    lcd.setCursor(0, 1);
    lcd.print(timeBetweenLoops / 1000);
    lcd.print(" s");
    while (!play_pause_set_btn.getSingleDebouncedRelease()) {
      if (increment_btn.isPressed()) {
        while (increment_btn.isPressed()) {
          if (increment <= 10 && counter <= 32500) {
            increment += counter / 10;
            counter += increment;
          }

          timeBetweenLoops += increment * 1000;
          timeBetweenLoops = constrain(timeBetweenLoops, timeBetweenLoops_LL, timeBetweenLoops_UL);

          clearRowSection(0, 19, 1);
          lcd.print(timeBetweenLoops / 1000);
          lcd.print(" s");
          delay(200);
        }

      }
      else if (decrement_btn.isPressed()) {
        while (decrement_btn.isPressed()) {
          if (increment <= 10 && counter <= 32500) {
            increment += counter / 10;
            counter += increment;
          }

          timeBetweenLoops -= increment * 1000;
          timeBetweenLoops = constrain(timeBetweenLoops, timeBetweenLoops_LL, timeBetweenLoops_UL);

          clearRowSection(0, 19, 1);
          lcd.print(timeBetweenLoops / 1000);
          lcd.print(" s");
          delay(200);
        }
      }
      increment = 1;
      counter = 0;
    }
  }
}


// Set up the original state of the LCD UI
void initializeUI() {
  // Initialize UI
  lcd.clear();

  // PRINT INITIAL TIME
  lcd.print("Time: 0:00:00:00");

  // PRINT TRAIN VARIABLES
  lcd.setCursor(0, 1);
  String strTVars = "";

  // If the train length is less than 10000ft then use ft as a measurement, use miles if more
  if (trainLength_ft < 10000) {
    strTVars.concat(String(int(trainLength_ft)));
    strTVars.concat(" ft | ");
  } else {
    strTVars.concat(String(trainLength_m));
    strTVars.concat(" mi | ");
  }
  strTVars.concat(String(trainSpeed));
  strTVars.concat(" mph");
  lcd.print(strTVars);
  if (activateLooping){
    lcd.setCursor(19, 1);
    lcd.write(byte(0));
  }

  // PRINT TRAIN POSITION
  lcd.setCursor(0, 2);
  String strTrainPosition = "Position: 0 ft";
  lcd.print(strTrainPosition);

  // PRINT CURRENT LOOP
  lcd.setCursor(0, 3);
  String strLoopCount = "Current Loop: 1";
  lcd.print(strLoopCount);
}


// 
void printTime() {
  String c = ":";
  String zero = "0";
  if (seconds > 59) {
    minutes++;
    seconds = 0;
    if ( minutes > 59) {
      hours++;
      minutes = 0;
      if (hours > 24) {
        days++;
        hours = 0;
      }
    }
  }
  String s_seconds = String(seconds);
  String s_minutes = String(minutes);
  String s_hours = String(hours);
  if (seconds < 10) {
    s_seconds = zero + s_seconds;
  }
  if (minutes < 10) {
    s_minutes = zero + s_minutes;
  }
  if (hours < 10) {
    s_hours = zero + s_hours;
  }
  String outputTime = days + c + s_hours + c + s_minutes + c + s_seconds;
  clearRowSection(6, 19, 0);
  lcd.print(outputTime);

  seconds++;
}


// Takes the input speed and converts it to a time value between relay activation
long speedToDelay(double trainSpeed_mph) {
  double fps = trainSpeed_mph * 1.467;
  long delayTime = (50 / fps) * 1000;
  return delayTime;
}


// This is the most important function in the code
// Sends information to slave arduinos with information on how to update the relay sequencing (moves the simulated train)
void sequencing() {
  String strPosition = "";
  if (frontOfTrain_counter >=0){
    // Transmit two bytes with PCB ADDR first the PIN then the STATE
    if (frontOfTrain_counter < (10 * final_address) and frontOfTrain_counter >= 0) {
      if (travelWest){
        F_address = frontOfTrain_counter / 10 + 1;
        FDATA[0] = pinoutArrayWest[frontOfTrain_counter % 10];
      }else{
        F_address = final_address - frontOfTrain_counter / 10;
        FDATA[0] = pinoutArrayEast[frontOfTrain_counter % 10];
      }      
      
      Wire.beginTransmission(F_address);
      Wire.write(FDATA, sizeof(FDATA));
      Wire.endTransmission();
      delay(5);
    }
    if (backOfTrain_counter < (10 * final_address) and backOfTrain_counter >= 0) {
      if (travelWest){
        B_address = backOfTrain_counter / 10 + 1;
        BDATA[0] = pinoutArrayWest[backOfTrain_counter % 10];
      }else{
        B_address = final_address - backOfTrain_counter / 10;
        BDATA[0] = pinoutArrayEast[backOfTrain_counter % 10];
      }
      
      Wire.beginTransmission(B_address);
      Wire.write(BDATA, sizeof(BDATA));
      Wire.endTransmission();
      delay(5);
    }
    if (frontOfTrain_counter < 10000) {
      strPosition.concat(String(frontOfTrain_counter * 50 + 50));
      strPosition.concat(" ft");
    } else {
      strPosition.concat(String(ft2miles(frontOfTrain_counter * 50 + 50)));
      strPosition.concat(" mi");
    }

  }else{
    strPosition = "0 ft";
  }

  // LCD update the train position
  clearRowSection(10, 19, 2);
  lcd.print(strPosition);

  frontOfTrain_counter++;
  backOfTrain_counter++;
}



// Write initial address number
void runStage1() {
  // GIVE EVERY DEVICE AN ADDRESS THROUGH UART
  while (Master_Stage == 1) {
    delay(1000);
    // Master continously send address '1' to next Arduino
    Serial.write(UART_start);

    // Read incoming Serial
    if (Serial.available() > 0) {
      Message = Serial.read();
      intMessage = (int)Message;
    }

    // Make sure we see the same address coming in 4 times in a row
    if (intMessage == (int)UART_ack) {
      counter++;
      // Completed communication loop and setup
      if (counter >= 3) {
        final_address = intMessage - 1;
        counter = 0;
        UART_ack = 255;
        Message = NULL;
        intMessage = NULL;
        Master_Stage = 2;
      }
    }
    else {
      if (Message != NULL) {
        UART_ack = Message;
      }
      counter = 0;
    }
  }
}

// Read & Write S1C
void runStage2() {
  while (Master_Stage == 2) {
    delay(5);
    // Send UART_ack byte
    Serial.write(UART_ack);

    if (Serial.available() > 0) {
      Message = Serial.read();
      intMessage = (int)Message;
    }

    // Check for UART_ack 4 times in a row
    if (intMessage == (int)UART_ack) {
      counter++;
      // Setup all Wire and closed all
      if (counter >= 3) {
        counter = 0;
        Message = NULL;
        intMessage = NULL;
        Master_Stage = 3;
      }
    }
    else {
      counter = 0;
    }
  }
}

// Check if all are connected
void runStage3() {
  if (Master_Stage == 3) {
    for (int i = 0; i < final_address; i++) {
      Wire.beginTransmission(PCB_ADDR[i]);
      Wire.write(198);
      Wire.endTransmission();
      Serial.println("End transmission");
    }
  }
  Master_Stage = 4;
}



// Ask user for speed input
// Implements an increasing incrementing algorithm while the button is being held down
void speedInput() {
  increment = 1;
  counter = 0;
  lcd.clear();
  lcd.print("Train Speed:");
  lcd.setCursor(0, 1);
  lcd.print(trainSpeed);
  lcd.print(" mph");
  while (!play_pause_set_btn.getSingleDebouncedRelease()) {
    if (increment_btn.isPressed()) {
      while (increment_btn.isPressed()) {
        if (increment <= 10 && counter <= 32500) {
          increment += counter / 10;
          counter += increment;
        }

        trainSpeed += increment;
        trainSpeed = constrain(trainSpeed, trainSpeed_LL, trainSpeed_UL);

        clearRowSection(0, 19, 1);
        lcd.print(trainSpeed);
        lcd.print(" mph");
        delay(200);
      }

    }
    else if (decrement_btn.isPressed()) {
      while (decrement_btn.isPressed()) {
        if (increment <= 10 && counter <= 32500) {
          increment += counter / 10;
          counter += increment;
        }

        trainSpeed -= increment;
        trainSpeed = constrain(trainSpeed, trainSpeed_LL, trainSpeed_UL);

        clearRowSection(0, 19, 1);
        lcd.print(trainSpeed);
        lcd.print(" mph");
        delay(200);
      }
    }
    increment = 1;
    counter = 0;
  }
}


// Ask user for train length input
// Implements an increasing incrementing algorithm while the button is being held down
void lengthInput() {
  increment = 50;
  counter = 0;
  trainLength_m = ft2miles(trainLength_ft);
  lcd.clear();
  lcd.print("Train Length:");

  lcd.setCursor(0, 1);
  lcd.print((int)trainLength_ft);
  lcd.print(" ft");

  lcd.setCursor(0, 2);
  lcd.print(trainLength_m);
  lcd.print(" miles");

  while (!play_pause_set_btn.getSingleDebouncedRelease()) {
    if (increment_btn.isPressed()) {
      while (increment_btn.isPressed()) {
        if (increment <= 4000 && counter <= 32500) {
          increment += counter / 500 * 50;
          counter += increment;
        }

        trainLength_ft += increment;
        trainLength_ft = constrain(trainLength_ft, trainLength_LL, trainLength_UL);
        trainLength_m = ft2miles(trainLength_ft);

        clearRowSection(0, 19, 1);
        lcd.print((long)trainLength_ft);
        lcd.print(" ft");
        clearRowSection(0, 19, 2);
        lcd.print(trainLength_m);
        lcd.print(" miles");

        delay(200);
      }

    }
    else if (decrement_btn.isPressed()) {
      while (decrement_btn.isPressed()) {
        if (increment <= 4000 && counter <= 32500) {
          increment += counter / 500 * 50;
          counter += increment;
        }

        trainLength_ft -= increment;
        trainLength_ft = constrain(trainLength_ft, trainLength_LL, trainLength_UL);
        trainLength_m = ft2miles(trainLength_ft);

        clearRowSection(0, 19, 1);
        lcd.print((long) trainLength_ft);
        lcd.print(" ft");
        clearRowSection(0, 19, 2);
        lcd.print(trainLength_m);
        lcd.print(" miles");

        delay(200);
      }
    }
    increment = 50;
    counter = 0;
  }
}



// --------------------------------------------------- VOID SETUP -----------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(200);
  lcd.begin(20, 4);
  lcd.createChar(0, infinityChar);
  lcd.setCursor(0, 0);
  lcd.print("Connect rail PCBs");
  lcd.setCursor(0, 3);
  lcd.print("Loading...");

  // Initialize communications
  runStage1();
  runStage2();
  for (int i = 1; i <= final_address; i++) {
    PCB_ADDR[i - 1] = i;
  }
  delay(1000);
  Wire.begin();
  runStage3();

  // User input setup prior to looping
  confirmPCBs();
  introScreen();
  speedInput();
  lengthInput();
  confirmTrainDirection();
  checkForLooping();
  checkTimeBetweenLooping();
  initializeUI();


  // Initialize Timer Variables
  delayTime = speedToDelay(trainSpeed);
  frontOfTrain_counter = -1;
  backOfTrain_counter = -1 - (trainLength_ft / 50);
  FDATA[1] = 1;
  BDATA[1] = 0;
  sequencingThread.set(delayTime, sequencing);
  timerThread.set(1000, printTime);

}



// ---------------------------------------------------- VOID LOOP -----------------------------------------------------
void loop() {

  // Play/Pause button will pause the simulation
  if (play_pause_set_btn.isPressed()) {
    pauseSimulation = !pauseSimulation;
    play_pause_set_btn.waitForRelease();
    if (pauseSimulation) {
      sequencingThread.disable();
      timerThread.disable();
    } else {
      sequencingThread.reset();
      timerThread.reset();
      sequencingThread.enable();
      timerThread.enable();
    }
  }

  // Threading is used to update the timer and the sequencing at the same time
  if (!pauseSimulation){
    sequencingThread.update();
    timerThread.update();
  
    if (activateLooping) {
      if (backOfTrain_counter >= final_address * 10) {
        sequencingThread.disable();
        finishTime = millis();
        frontOfTrain_counter = -1;
        backOfTrain_counter = -1 - (trainLength_ft / 50);
  
        currentLoop++;
        clearRowSection(14, 19, 3);
        lcd.print(currentLoop);
        Serial.println(timeBetweenLoops);
        Serial.println(finishTime + timeBetweenLoops);
      }
      if (millis() >= finishTime + timeBetweenLoops) {
        sequencingThread.enable();
      }
    } else {
      if (backOfTrain_counter >= final_address * 10) {
        sequencingThread.disable();
        timerThread.disable();
      }
    }
  }
  
}
