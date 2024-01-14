#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include "Encoder.h"
#include <math.h>      
#include "IRremote.h"

#define DT 8
#define CLK 9
#define okPin 24
#define nextPin 25
#define prevPin 26
#define SW 7
#define IRpin 2


//Object
LiquidCrystal_I2C lcd(0X27, 20, 4);
Encoder encoder(CLK, DT, SW);  //clk,dt
IRrecv RX(IRpin);
File myFile;
File root;

//IR variables
unsigned long IRvalue;
String cmd = "";
int cmdCount = 0;
int cmdEnd = 0;

//variable declaration
unsigned long previousMillis = 0;
unsigned int delayTime = 300;
bool acknowledged = false;
bool inputCparameter;
bool inputVLparameter;
bool inputHLparameter;
bool previous = false;
bool next = false;
int counter = 0;
int SAcounter = 0;
int previousCLK;
int maxIncr = 12;  //change this value later
int AmaxIncr = 12;
int SAmaxIncr = 4;
int slideMax = 3;
int slide = 0;

bool autoMode = false;
bool semiAutoMode = false;
bool fxValue = false;
bool fyValue = false;
bool rValue = false;
bool xcValue = false;
bool ycValue = false;
bool CgcodeGen = false;

bool x0value = false;
bool y0value = false;
bool Lvalue = false;
bool VLgcodeGen = false;
bool HLgcodeGen = false;

String get_ok;
String readString;
bool okToSendGcode = false;
//int p = 0;

String execFile;

//directory variable
#define listItemsTotal 100   //maximum number of file names to store
#define listItemMaxChar 100  //maximum number of characters per file name, including null terminator
char *listItems[listItemsTotal];
char tempString[listItemMaxChar];  //temporary string of the max number of characters per file name
byte listLength = 0;
char dirx[256] = "/";


//****semi-auto variables*****

//math variables//
float deg[] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60,
                65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125,
                130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190,
                195, 200, 205, 210, 215, 220, 225, 230, 235, 240, 245, 250, 255,
                260, 265, 270, 275, 280, 285, 290, 295, 300, 305, 310, 315, 320,
                325, 330, 335, 340, 345, 350, 355, 360 };

//circle parameters
float r;   //40
float xc;  //25;
float yc;  //40;
float fy;  //10000;
float fx;  //= 4000;

//vline parameters
float x0 = 12;
float y0 = 12;
float l = 140;

float xx;
float yy;
float *xxArray;
float *yyArray;
const int arrLen = sizeof(deg) / sizeof(deg[0]);
//---------END-------


void setup() {
  Serial.begin(115200);   //78492
  Serial1.begin(115200);  //78492
  lcd.begin();
  //lcd.clear();
  RX.enableIRIn();
  EncoderInterrupt.begin(&encoder);
  //pinMode(CLK, INPUT_PULLUP);
  //pinMode(DT, INPUT_PULLUP);
  pinMode(okPin, INPUT_PULLUP);
  pinMode(prevPin, INPUT_PULLUP);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(4, OUTPUT);

  if (!SD.begin(4)) {
    Serial.println("SD card no detected!");
    return;
  }

  //sendGcode();

  Serial.println("done!");
}


//*****FILESELECT FUNCTION******
//String selectFile() {
//  scanDir();
//  execFile = listItems[4];
//  return execFile;
//}




//*****DIRECTORY FUNCTION******
void scanDir() {
  freeListMemory();  // Start by freeing previously allocated malloc pointers
  File root = SD.open(dirx);
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;  // no more files
    } else {
      //      listItems[listLength][0] = entry.name(); //add file name to list array
      //      listItems[listLength] = (char[128]) malloc(128);
      sprintf(tempString, "%s", entry.name());                  //save file name to temporary string with null terminator at the end
      listItems[listLength] = (char *)malloc(listItemMaxChar);  //assign enough memory for 100 chars to current list item pointer
      sprintf(listItems[listLength], "%s", tempString);
      listLength++;  //increment counter of files
      entry.close();
    }
  }
}

void freeListMemory() {
  // If we have previous items in the list, then free the memory
  for (byte i = 0; i <= listLength; i++) {
    free(listItems[i]);
  }

  listLength = 0;  //reset list length
}


//*****AUTO MODE SEND GCODE FUNCTION******
void sendGcode(int c) {
  scanDir();

  myFile = SD.open(listItems[c]);
  if (myFile) {
    Serial1.println("G10 P0 L20 X0 Y0 Z0");  //no limits switches so good idea to wakeup GRBL by Zetzeroing.
    while (myFile.available()) {
      while (Serial1.available()) {
        delay(3);
        char c = Serial1.read();
        readString += c;
      }
      if (readString.length() > 0) {
        //Serial.println(readString);//echo
        if (readString.indexOf("ok") >= 0) {
          okToSendGcode = true;
        }
        readString = "";
      }
      if (okToSendGcode == true) {
        //we get "ok" with jogging commands too not only when sending file.
        String l_line = "";                     //create an empty string
        l_line = myFile.readStringUntil('\n');  //it looks for end of the line in my file
        Serial1.println(l_line);                //Yes you can send this line
        Serial.println(l_line);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(l_line);
        okToSendGcode = false;
      }
    }
    myFile.close();
  }
  myFile.close();
}









//********SEMI-AUTO GCODE SENDER FUNCTION*********
void SAsendGcode(String f) {
  myFile = SD.open(f);
  if (myFile) {
    Serial1.println("G10 P0 L20 X0 Y0 Z0");  //no limits switches so good idea to wakeup GRBL by Zetzeroing.
    while (myFile.available()) {
      while (Serial1.available()) {
        delay(3);
        char c = Serial1.read();
        readString += c;
      }
      if (readString.length() > 0) {
        //Serial.println(readString);//echo
        if (readString.indexOf("ok") >= 0) {
          okToSendGcode = true;
        }
        readString = "";
      }
      if (okToSendGcode == true) {
        //we get "ok" with jogging commands too not only when sending file.
        String l_line = "";                     //create an empty string
        l_line = myFile.readStringUntil('\n');  //it looks for end of the line in my file
        Serial1.println(l_line);                //Yes you can send this line
        Serial.println(l_line);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(l_line);
        okToSendGcode = false;
      }
    }
    myFile.close();
  }
  myFile.close();
}

//*****OK FUNCTION******
void okButton() {
  int state, currentState;
  if (digitalRead(okPin) == LOW) {
    state = digitalRead(okPin);
    millis();
    if ((millis() - previousMillis) > delayTime) {
      currentState = digitalRead(okPin);
      if (currentState == state) {
        acknowledged = true;
      } else {
        acknowledged = false;
      }
      previousMillis = millis();
      //Serial.println(acknowledged);
    }
  }
}



//*****PREVIOUS FUNCTION******

void prevButton() {
  int state, currentState;
  if (digitalRead(prevPin) == LOW) {
    state = digitalRead(prevPin);
    millis();
    if ((millis() - previousMillis) > delayTime) {
      currentState = digitalRead(prevPin);
      if (currentState == state) {
        previous = true;
        slide--;
        if (slide < 0) {
          slide = 0;
        }
      } else {
        previous = false;
      }
      previousMillis = millis();
      //Serial.println(previous);
    }
  }
}



//*****NEXT FUNCTION******
void nextButton(int slideMax) {
  int state, currentState;
  if (digitalRead(nextPin) == LOW) {
    state = digitalRead(nextPin);
    millis();
    if ((millis() - previousMillis) > delayTime) {
      currentState = digitalRead(nextPin);
      if (currentState == state) {
        next = true;
        slide++;
        if (slide > slideMax) {
          slide = slideMax;
        }
      } else {
        next = false;
      }
      previousMillis = millis();
      //Serial.println(next);
    }
  }
}



//*****ROTARY ENCODER FUNCTION******
//void rotary(int maxIncr) {
//  if (previousCLK != digitalRead(CLK)) {
//    if (digitalRead(CLK) != digitalRead(DT)) {
//      counter++;
//      if (counter > maxIncr) {
//        counter = maxIncr;
//      }
//    }
//    else {
//      counter--;
//      if (counter < 0) {
//        counter = 0;
//      }
//    }
//    Serial.println(counter);
//  }
//}

void rotary(int maxIncr) {
  bool pb = encoder.button();
  int delta = encoder.delta();
  counter += delta;
  if (counter > maxIncr) {
    counter = maxIncr;
  }
  if (counter < 0) {
    counter = 0;
  }
  Serial.println(counter);
}

void SArotary(int SAmaxIncr) {
  bool pb = encoder.button();
  int delta = encoder.delta();
  SAcounter += delta * 500;
  if (SAcounter > SAmaxIncr) {
    SAcounter = maxIncr;
  }
  if (SAcounter < 0) {
    SAcounter = 0;
  }
  Serial.println(SAcounter);
}


//*****IR Sensor functions
void decodeIR() {
  if (RX.decode()) {
    IRvalue = RX.decodedIRData.decodedRawData;
    delay(150);
    RX.resume();
  }
}

void num() {

  decodeIR();
  switch (IRvalue) {

    case 0xE619FF00:
      cmdCount++;
      cmd = cmd + "0";
      IRvalue = 0;
      break;

    case 0xBA45FF00:
      cmdCount++;
      cmd = cmd + "1";
      IRvalue = 0;
      break;

    case 0xB946FF00:
      cmdCount++;
      cmd = cmd + "2";
      IRvalue = 0;
      break;

    case 0xB847FF00:
      cmdCount++;
      cmd = cmd + "3";
      IRvalue = 0;
      break;

    case 0xBB44FF00:
      cmdCount++;
      cmd = cmd + "4";
      IRvalue = 0;
      break;

    case 0xBF40FF00:
      cmdCount++;
      cmd = cmd + "5";
      IRvalue = 0;
      break;

    case 0xBC43FF00:
      cmdCount++;
      cmd = cmd + "6";
      IRvalue = 0;
      break;

    case 0xF807FF00:
      cmdCount++;
      cmd = cmd + "7";
      IRvalue = 0;
      break;

    case 0xEA15FF00:
      cmdCount++;
      cmd = cmd + "8";
      IRvalue = 0;
      break;

    case 0xF609FF00:
      cmdCount++;
      cmd = cmd + "9";
      IRvalue = 0;
      break;

    case 0xE31CFF00:
      IRvalue = 0;
      return 10;
      break;
  }
}




//*****HOMESCREEN FUNCTION******
void homeScreen(int slide) {
  switch (slide) {
    case 0:
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("CNC PLOTTER");
      delay(250);
      break;
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AUTOMATIC MODE");
      lcd.setCursor(0, 2);
      lcd.print("Press Ok to Select");
      lcd.setCursor(0, 3);
      lcd.print("File");
      delay(250);
      break;


    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SEMI-AUTO MOODE");
      lcd.setCursor(0, 2);
      lcd.print("Press Ok to Enter");
      lcd.setCursor(0, 3);
      lcd.print("Plotting Parameters");
      delay(250);
      break;

    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 2);
      lcd.print("Press Ok to activate");
      lcd.setCursor(0, 3);
      lcd.print("manual mode");
      delay(250);
      break;
  }
}

void semiAutoScreen(int count) {
  switch (count) {
    case 0:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.print("CIRCLE");
      lcd.setCursor(1, 1);
      lcd.print("VERTICAL LINE");
      lcd.setCursor(1, 2);
      lcd.print("HORIZONTAL LINE");
      delay(250);
      break;

    case 2:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CIRCLE");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print("VERTICAL LINE");
      lcd.setCursor(1, 2);
      lcd.print("HORIZONTAL LINE");
      delay(250);
      break;

    case 4:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CIRCLE");
      lcd.setCursor(1, 1);
      lcd.print("VERTICAL LINE");
      lcd.setCursor(0, 2);
      lcd.print(">");
      lcd.print("HORIZONTAL LINE");
      delay(250);
      break;
  }
}

void autoScreen(int count) {
  scanDir();
  switch (count) {
    case 0:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.print(String(listItems[1]));
      lcd.setCursor(1, 1);
      lcd.print(String(listItems[2]));
      lcd.setCursor(1, 2);
      lcd.print(String(listItems[3]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[4]));
      delay(250);
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(String(listItems[1]));
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(String(listItems[2]));
      lcd.setCursor(1, 2);
      lcd.print(String(listItems[3]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[4]));
      delay(250);
      break;

    case 4:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(String(listItems[1]));
      lcd.setCursor(1, 1);

      lcd.print(String(listItems[2]));
      lcd.setCursor(0, 2);
      lcd.print(">");
      lcd.print(String(listItems[3]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[4]));
      delay(250);
      break;



    case 6:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(String(listItems[1]));
      lcd.setCursor(1, 1);

      lcd.print(String(listItems[2]));
      lcd.setCursor(1, 2);
      lcd.print(String(listItems[3]));
      lcd.setCursor(0, 3);
      lcd.print(">");
      lcd.print(String(listItems[4]));
      delay(250);
      break;

    case 8:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.print(String(listItems[5]));
      lcd.setCursor(1, 1);
      lcd.print(String(listItems[6]));
      lcd.setCursor(1, 2);
      lcd.print(String(listItems[7]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[8]));
      delay(250);
      break;

    case 10:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(String(listItems[5]));
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(String(listItems[6]));
      lcd.setCursor(1, 2);
      lcd.print(String(listItems[7]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[8]));
      delay(250);
      break;

    case 12:
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(String(listItems[5]));
      lcd.setCursor(1, 1);

      lcd.print(String(listItems[6]));
      lcd.setCursor(0, 2);
      lcd.print(">");
      lcd.print(String(listItems[7]));
      lcd.setCursor(1, 3);
      lcd.print(String(listItems[8]));
      delay(250);
      break;
  }
}


//*****LOOP FUNCTION******
void loop() {
  okButton();
  prevButton();
  nextButton(slideMax);
  previousCLK = digitalRead(CLK);
  //rotary(maxIncr);
  homeScreen(slide);
  //autoScreen(counter);
  //Serial.println(slide);

  if (slide == 1 && acknowledged) {
    autoMode = true;
    while (autoMode) {
      acknowledged = false;
      autoScreen(counter);
      okButton();
      rotary(AmaxIncr);
      if (counter == 0 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 2 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 4 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 6 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 8 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 10 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }

      else if (counter == 10 && acknowledged) {
        int c = (counter/2)+1;
        sendGcode(c);
        Serial.println("done done");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Executed");
        lcd.setCursor(0, 1);
        lcd.print("Successfully!");
        delay(1000);
        acknowledged = false;
        autoMode = false;
      }
    }
    acknowledged = false;
  }
  //****END OF AUTO MODE LOOP*****

  //****START SEMI-AUTO MODE LOOP*****
  else if (slide == 2 && acknowledged) {
    semiAutoMode = true;
    while (semiAutoMode) {
      acknowledged = false;
      semiAutoScreen(counter);
      okButton();
      rotary(SAmaxIncr);
  


      //****SEMI-AUTO CIRCLE LOOP*****
      if (counter == 0 && acknowledged) {
        inputCparameter = true;
        while (inputCparameter) {

          acknowledged = false;
          okButton();
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Press ok to continue");
          delay(150);
          //fxValue = true;   //move down

          if (acknowledged) {
            fxValue = true;
            while (fxValue) {
              acknowledged = false;
              okButton();
              num();
              fx = cmd.toInt();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Set Feedrate mm/min");
              lcd.setCursor(0, 1);
              lcd.print("for X axis");
              lcd.setCursor(0, 3);
              lcd.print(">");
              lcd.print(cmd);
              delay(150);
              Serial.println(cmdCount);

              if (acknowledged) {
                cmd = "";
                Serial.println("setting fyValue");
                fyValue = true;
                while (fyValue) {
                  acknowledged = false;
                  okButton();
                  num();
                  fy = cmd.toInt();
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Set Feedrate mm/min");
                  lcd.setCursor(0, 1);
                  lcd.print("for Y axis");
                  lcd.setCursor(0, 3);
                  lcd.print(">");
                  lcd.print(cmd);
                  delay(150);
                  //Serial.println(fx);

                  if (acknowledged) {
                    cmd = "";
                    Serial.println("setting r Value");
                    rValue = true;
                    while (rValue) {
                      acknowledged = false;
                      okButton();
                      num();
                      r = cmd.toInt();
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("Set radius of Circle");
                      lcd.setCursor(0, 1);
                      lcd.print("in mm");
                      lcd.setCursor(0, 3);
                      lcd.print(">");
                      lcd.print(cmd);
                      delay(150);
                      //Serial.println(fx);
                      //Serial.println(fy);

                      if (acknowledged) {
                        cmd = "";
                        Serial.println("setting xc Value");
                        xcValue = true;
                        while (xcValue) {
                          acknowledged = false;
                          okButton();
                          num();
                          xc = cmd.toInt();
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Xc from the Origin");
                          lcd.setCursor(0, 1);
                          lcd.print("in mm");
                          lcd.setCursor(0, 3);
                          lcd.print(">");
                          lcd.print(cmd);
                          delay(150);

                          if (acknowledged) {
                            cmd = "";
                            Serial.println("setting yc Value");
                            ycValue = true;
                            while (ycValue) {
                              acknowledged = false;
                              okButton();
                              num();
                              yc = cmd.toInt();
                              lcd.clear();
                              lcd.setCursor(0, 0);
                              lcd.print("Yc from the Origin");
                              lcd.setCursor(0, 1);
                              lcd.print("in mm");
                              lcd.setCursor(0, 3);
                              lcd.print(">");
                              lcd.print(cmd);
                              delay(150);
                              Serial.println(fx);
                              Serial.println(fy);
                              Serial.println(r);
                              Serial.println(xc);
                              if (acknowledged) {
                                cmd = "";
                                Serial.println("Generating Cgcode");
                                CgcodeGen = true;
                                while (CgcodeGen) {
                                  acknowledged = false;
                                  getCoordX(r, xc);
                                  getCoordY(r, yc);
                                  circleGcode(fx, fy);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("GCODE generated");
                                  lcd.setCursor(0, 2);
                                  lcd.print("successfully!");
                                  delay(2000);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("Printing Starts");
                                  lcd.setCursor(0, 2);
                                  lcd.print("shortly!");
                                  delay(2000);
                                  SAsendGcode("circle.txt");
                                  Serial.println("done done!");

                                  Serial.println("Parameters: r: " + String(r) + " xc " + String(xc) + " yc " + String(yc));
                                  Serial.print(" r: " + String(r) + " fx " + String(fx) + " fy " + String(fy));

                                  //****END***

                                  CgcodeGen = false;
                                  ycValue = false;
                                  xcValue = false;
                                  rValue = false;
                                  fyValue = false;
                                  fxValue = false;
                                  inputCparameter = false;
                                  semiAutoMode = false;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      //****SEMI-AUTO VL LOOP*****

      else if (counter == 2 && acknowledged) {
        inputVLparameter = true;
        while (inputVLparameter) {

          acknowledged = false;
          okButton();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Press ok to continue");
          delay(150);
          //fxValue = true;   //move down
          if (acknowledged) {
            fxValue = true;
            while (fxValue) {
              acknowledged = false;
              okButton();
              num();
              fx = cmd.toInt();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Set Feedrate mm/min");
              lcd.setCursor(0, 1);
              lcd.print("for X axis");
              lcd.setCursor(0, 3);
              lcd.print(">");
              lcd.print(cmd);
              delay(150);
              Serial.println(cmdCount);

              if (acknowledged) {
                cmd = "";
                Serial.println("setting fyValue");
                fyValue = true;
                while (fyValue) {
                  acknowledged = false;
                  okButton();
                  num();
                  fy = cmd.toInt();
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Set Feedrate mm/min");
                  lcd.setCursor(0, 1);
                  lcd.print("for Y axis");
                  lcd.setCursor(0, 3);
                  lcd.print(">");
                  lcd.print(cmd);
                  delay(150);
                  if (acknowledged) {
                    cmd = "";
                    Serial.println("setting X0 Value");
                    x0value = true;
                    while (x0value) {
                      acknowledged = false;
                      okButton();
                      num();
                      x0 = cmd.toInt();
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("Set x0 from the origin");
                      lcd.setCursor(0, 1);
                      lcd.print("in mm");
                      lcd.setCursor(0, 3);
                      lcd.print(">");
                      lcd.print(cmd);
                      delay(150);
                      if (acknowledged) {
                        cmd = "";
                        Serial.println("setting Y0 Value");
                        y0value = true;
                        while (y0value) {
                          acknowledged = false;
                          okButton();
                          num();
                          y0 = cmd.toInt();
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Set y0 from the origin");
                          lcd.setCursor(0, 1);
                          lcd.print("in mm");
                          lcd.setCursor(0, 3);
                          lcd.print(">");
                          lcd.print(cmd);
                          delay(150);
                          if (acknowledged) {
                            cmd = "";
                            Serial.println("setting length Value");
                            Lvalue = true;
                            while (Lvalue) {
                              acknowledged = false;
                              okButton();
                              num();
                              l = cmd.toInt();
                              lcd.clear();
                              lcd.setCursor(0, 0);
                              lcd.print("Set the length of line");
                              lcd.setCursor(0, 1);
                              lcd.print("in mm");
                              lcd.setCursor(0, 3);
                              lcd.print(">");
                              lcd.print(cmd);
                              delay(150);
                              if (acknowledged) {
                                cmd = "";
                                Serial.println("Generating VLgcode");
                                VLgcodeGen = true;
                                while (VLgcodeGen) {
                                  acknowledged = false;
                                  vLineGcode(x0, y0, l, fx, fy);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("GCODE generated");
                                  lcd.setCursor(0, 2);
                                  lcd.print("successfully!");
                                  delay(2000);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("Printing Starts");
                                  lcd.setCursor(0, 2);
                                  lcd.print("shortly!");
                                  delay(2000);
                                  SAsendGcode("vline.txt");
                                  Serial.println("done done!");

                                  Serial.println("Parameters: x0: " + String(x0) + " y0 " + String(y0));
                                  Serial.print(" l: " + String(l) + " fx " + String(fx) + " fy " + String(fy));
                                  //****END***

                                  VLgcodeGen = false;
                                  Lvalue = false;
                                  y0value = false;
                                  x0value = false;
                                  fyValue = false;
                                  fxValue = false;
                                  inputVLparameter = false;
                                  semiAutoMode = false;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      //****SEMI-AUTO HL LOOP*****

      else if (counter == 4 && acknowledged) {
        inputHLparameter = true;
        while (inputHLparameter) {

          acknowledged = false;
          okButton();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Press ok to continue");
          delay(150);
          //fxValue = true;   //move down
          if (acknowledged) {
            fxValue = true;
            while (fxValue) {
              acknowledged = false;
              okButton();
              num();
              fx = cmd.toInt();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Set Feedrate mm/min");
              lcd.setCursor(0, 1);
              lcd.print("for X axis");
              lcd.setCursor(0, 3);
              lcd.print(">");
              lcd.print(cmd);
              delay(150);
              Serial.println(cmdCount);

              if (acknowledged) {
                cmd = "";
                Serial.println("setting fyValue");
                fyValue = true;
                while (fyValue) {
                  acknowledged = false;
                  okButton();
                  num();
                  fy = cmd.toInt();
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Set Feedrate mm/min");
                  lcd.setCursor(0, 1);
                  lcd.print("for Y axis");
                  lcd.setCursor(0, 3);
                  lcd.print(">");
                  lcd.print(cmd);
                  delay(150);
                  if (acknowledged) {
                    cmd = "";
                    Serial.println("setting X0 Value");
                    x0value = true;
                    while (x0value) {
                      acknowledged = false;
                      okButton();
                      num();
                      x0 = cmd.toInt();
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("Set x0 from the origin");
                      lcd.setCursor(0, 1);
                      lcd.print("in mm");
                      lcd.setCursor(0, 3);
                      lcd.print(">");
                      lcd.print(cmd);
                      delay(150);
                      if (acknowledged) {
                        cmd = "";
                        Serial.println("setting Y0 Value");
                        y0value = true;
                        while (y0value) {
                          acknowledged = false;
                          okButton();
                          num();
                          y0 = cmd.toInt();
                          lcd.clear();
                          lcd.setCursor(0, 0);
                          lcd.print("Set y0 from the origin");
                          lcd.setCursor(0, 1);
                          lcd.print("in mm");
                          lcd.setCursor(0, 3);
                          lcd.print(">");
                          lcd.print(cmd);
                          delay(150);
                          if (acknowledged) {
                            cmd = "";
                            Serial.println("setting length Value");
                            Lvalue = true;
                            while (Lvalue) {
                              acknowledged = false;
                              okButton();
                              num();
                              l = cmd.toInt();
                              lcd.clear();
                              lcd.setCursor(0, 0);
                              lcd.print("Set the length of line");
                              lcd.setCursor(0, 1);
                              lcd.print("in mm");
                              lcd.setCursor(0, 3);
                              lcd.print(">");
                              lcd.print(cmd);
                              delay(150);
                              if (acknowledged) {
                                cmd = "";
                                Serial.println("Generating HLgcode");
                                HLgcodeGen = true;
                                while (HLgcodeGen) {
                                  acknowledged = false;
                                  hLineGcode(x0, y0, l, fx, fy);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("GCODE generated");
                                  lcd.setCursor(0, 2);
                                  lcd.print("successfully!");
                                  delay(2000);
                                  lcd.clear();
                                  lcd.setCursor(0, 0);
                                  lcd.print("Printing Starts");
                                  lcd.setCursor(0, 2);
                                  lcd.print("shortly!");
                                  delay(2000);
                                  SAsendGcode("hline.txt");
                                  Serial.println("done done!");

                                  Serial.println("Parameters: x0: " + String(x0) + " y0 " + String(y0));
                                  Serial.print(" l: " + String(l) + " fx " + String(fx) + " fy " + String(fy));
                                  //****END***

                                  HLgcodeGen = false;
                                  Lvalue = false;
                                  y0value = false;
                                  x0value = false;
                                  fyValue = false;
                                  fxValue = false;
                                  inputHLparameter = false;
                                  semiAutoMode = false;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}





//******SEMI-AUTO FUNCTIONS*******
float *getCoordX(float r, float xc) {
  static float x[arrLen] = {};
  for (int i = 0; i < arrLen; i++) {
    x[i] = (r * cos((deg[i] * PI) / 180)) + xc;
  }
  return x;
}

float *getCoordY(float r, float yc) {
  static float y[arrLen] = {};
  for (int i = 0; i < arrLen; i++) {
    y[i] = (r * sin((deg[i] * PI) / 180)) + yc;
  }
  return y;
}


void circleGcode(float fx, float fy) {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  if (SD.exists("circle.txt")) {
    SD.remove("circle.txt");
  }
  Serial.println("initialization done.");


  myFile = SD.open("circle.txt", FILE_WRITE);

  if (myFile) {

    xxArray = getCoordX(r, xc);
    yyArray = getCoordY(r, yc);
    myFile.println("M3");
    myFile.println("G90");
    myFile.println("G21");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println(String("G1") + String("X") + String(xxArray[0], 4) + String(" ") + String("Y") + String(yyArray[0], 4));
    myFile.println("M5 S90");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fx, 4));


    for (int i = 1; i < arrLen; i++) {
      xx = *(xxArray + i);
      yy = *(yyArray + i);


      myFile.println(String("G1 ") + String("X") + String(xx, 4) + String(" ") + String("Y") + String(yy, 4));

      delay(10);
    }
    myFile.println("M3");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println("G1 X0 Y0");

    // close the file:
    myFile.close();
    Serial.println("done.");

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening circle.txt");
  }

  // re-open the file for reading:
  //SAsendGcode("vline.txt");
}

void vLineGcode(float x0, float y0, float l, float fx, float fy) {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  if (SD.exists("vline.txt")) {
    SD.remove("vline.txt");
  }
  Serial.println("initialization done.");


  myFile = SD.open("vline.txt", FILE_WRITE);

  if (myFile) {

    myFile.println("M3");
    myFile.println("G90");
    myFile.println("G21");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println(String("G1") + " " + String("X") + String(x0, 4) + String(" ") + String("Y") + String(y0, 4));
    myFile.println("M5 S90");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fx, 4));

    myFile.println(String("G1") + " " + String("X") + String(x0, 4) + String(" ") + String("Y") + String(l + y0, 4));

    myFile.println("M3");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println("G1 X0 Y0");

    // close the file:
    myFile.close();
    Serial.println("done.");

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening circle.txt");
  }

  // re-open the file for reading:
  //SAsendGcode("vline.txt");
}

void hLineGcode(float x0, float y0, float l, float fx, float fy) {
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  if (SD.exists("hline.txt")) {
    SD.remove("hline.txt");
  }
  Serial.println("initialization done.");


  myFile = SD.open("hline.txt", FILE_WRITE);

  if (myFile) {

    myFile.println("M3");
    myFile.println("G90");
    myFile.println("G21");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println(String("G1") + " " + String("X") + String(x0, 4) + String(" ") + String("Y") + String(y0, 4));
    myFile.println("M5 S90");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fx, 4));

    myFile.println(String("G1") + " " + String("X") + String(l + x0, 4) + String(" ") + String("Y") + String(y0, 4));

    myFile.println("M3");
    myFile.println("G4 P0.20000000298023224");
    myFile.println("G1 F" + String(fy, 4));
    myFile.println("G1 X0 Y0");

    // close the file:
    myFile.close();
    Serial.println("done.");

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening circle.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("hline.txt");
  if (myFile) {
    Serial.println("hline.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening circle.txt");
  }
}
