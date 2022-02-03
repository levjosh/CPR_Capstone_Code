#include <Wire.h>

int Slave_Stage = 1;
byte Message;
int intMessage;
byte UART_ack = 255;

int THIS_ADDR;
byte NEXT_ADDR;

int counter = 0;


void receiveEvent(int howMany){
  int DATA [howMany];
  if (Slave_Stage == 3){
    int response;
    while (Wire.available()){
      byte b = Wire.read();
      response = (int)b;
    }
    Serial.print("Number from wire: ");
    Serial.println(response);
    delay(5);

    if (response == 198){
      Slave_Stage = 4;
    }
  }
  if (Slave_Stage == 4){
    while (Wire.available()){
      for (int i = 0; i < howMany; i++){
        byte b = Wire.read();
        DATA[i] = (int)b;
      }
    }
    delay(5);
    digitalWrite(DATA[0], DATA[1]);

  }
}


// Look for incoming address value
void runStage1(){

  while (Slave_Stage == 1){
    if (Serial.available() > 0){
      Message = Serial.read();
      intMessage = (int)Message;
    }
    delay(5);

    // Make sure we see the same address coming in 4 times in a row
    if (intMessage == (int)UART_ack){
      counter++;
      // Completed communication loop and setup
      if (counter >= 3){
        THIS_ADDR = (int) Message;
        NEXT_ADDR = Message + 1;
        counter = 0;
        intMessage = NULL;
        UART_ack = 255;
        Slave_Stage = 2;
      }
    }
    else{
      if (Message != NULL){
        UART_ack = Message;
      }
      counter = 0;
    }  
  }
}

// Keep transmitting NEXT_ADDR until we receive 255 on UART
void runStage2(){

  while (Slave_Stage == 2){
    delay(5);
    Serial.write(NEXT_ADDR);

    if (Serial.available() > 0){
      Message = Serial.read();
      intMessage = (int)Message;
    }
  
    // Make sure we see the '255' 4 times in a row
    if (intMessage == (int)UART_ack){
      counter++;
      // Setup all Wire and closed all 
      if (counter >= 3){
        for(int i = 0; i < 10; i++){
          Serial.write(UART_ack);
          delay(5);
        }
        counter =  0;
        Slave_Stage = 3;
      }
    }
    else{
      counter = 0;
    }  


  }
}


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(100);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

  runStage1();
  runStage2();

  // Initialize Wires for I2C
  Wire.begin(THIS_ADDR);
  Wire.onReceive(receiveEvent);

}

void loop() {
}
