#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include "Encoder.h"

#define DT  8
#define CLK 9
#define okPin 24
#define nextPin 25
#define prevPin 26
#define SW 7


//Object
LiquidCrystal_I2C lcd(0X27, 20, 4);
Encoder encoder( CLK, DT, SW );   //clk,dt
File myFile;
File root;


//variable declaration
unsigned long previousMillis = 0;
unsigned int delayTime = 300;
bool acknowledged = false;
bool previous = false;
bool next = false;
int counter = 0;
int previousCLK;
int maxIncr = 12; //change this value later
int slideMax = 3;
int slide = 0;

bool autoMode = false;

String get_ok;
String readString;
bool okToSendGcode = false;
//int p = 0;

String execFile;

//directory variable
#define listItemsTotal 100 //maximum number of file names to store
#define listItemMaxChar 100 //maximum number of characters per file name, including null terminator
char *listItems[listItemsTotal];
char tempString[listItemMaxChar];//temporary string of the max number of characters per file name
byte listLength = 0;
char dirx[256] = "/";


void setup()
{
  Serial.begin(115200);//78492
  Serial1.begin(115200);//78492
  lcd.begin();
  lcd.clear();
  EncoderInterrupt.begin( &encoder );
  //pinMode(CLK, INPUT_PULLUP);
  //pinMode(DT, INPUT_PULLUP);
  pinMode(okPin, INPUT_PULLUP);
  pinMode(prevPin, INPUT_PULLUP);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(4, OUTPUT);

  if (!SD.begin(4))
  {
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




//*****SEND GCODE FUNCTION******
void sendGcode(int c) {
  scanDir();

  myFile = SD.open(listItems[c]);
  if (myFile) {
    Serial1.println("G10 P0 L20 X0 Y0 Z0");//no limits switches so good idea to wakeup GRBL by Zetzeroing.
    while (myFile.available()) {
      while (Serial1.available()) {
        delay(3);
        char c = Serial1.read();
        readString += c;
      }
      if (readString.length() > 0) {
        //Serial.println(readString);//echo
        if (readString.indexOf("ok") >= 0)
        {
          okToSendGcode = true;
        }
        readString = "";
      }
      if (okToSendGcode == true) {
        //we get "ok" with jogging commands too not only when sending file.
        String l_line = "";//create an empty string
        l_line = myFile.readStringUntil('\n'); //it looks for end of the line in my file
        Serial1.println(l_line);//Yes you can send this line
        Serial.println(l_line);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(l_line);
        okToSendGcode = false;
      }
    }
    myFile.close();
  }
  myFile.close();
}




//*****DIRECTORY FUNCTION******
void scanDir() {
  freeListMemory(); // Start by freeing previously allocated malloc pointers
  File root = SD.open(dirx);
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break; // no more files
    }
    else {
      //      listItems[listLength][0] = entry.name(); //add file name to list array
      //      listItems[listLength] = (char[128]) malloc(128);
      sprintf(tempString, "%s", entry.name());//save file name to temporary string with null terminator at the end
      listItems[listLength] = (char *)malloc(listItemMaxChar);//assign enough memory for 100 chars to current list item pointer
      sprintf(listItems[listLength], "%s", tempString);
      listLength++; //increment counter of files
      entry.close();
    }

  }
}

void freeListMemory() {
  // If we have previous items in the list, then free the memory
  for (byte i = 0; i <= listLength; i++)
  {
    free(listItems[i]);
  }

  listLength = 0;//reset list length
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
      }
      else {
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
        slide --;
        if (slide < 0) {
          slide = 0;
        }
      }
      else {
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
        slide ++;
        if (slide > slideMax) {
          slide = slideMax;
        }
      }
      else {
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
  rotary(maxIncr);
  homeScreen(slide);
  //autoScreen(counter);
  //Serial.println(slide);

  if(slide == 1 && acknowledged){
    autoMode = true;
    while(autoMode){
      acknowledged = false;
      autoScreen(counter);
       okButton();
       rotary(maxIncr);
      if(counter == 2 && acknowledged){
         sendGcode(2);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }

      else if(counter == 4 && acknowledged){
         sendGcode(3);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }

      else if(counter == 6 && acknowledged){
         sendGcode(4);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }

      else if(counter == 8 && acknowledged){
         sendGcode(5);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }

      else if(counter == 10 && acknowledged){
         sendGcode(6);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }

      else if(counter == 10 && acknowledged){
         sendGcode(6);
         Serial.println("done done");
         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("Executed");
         lcd.setCursor(0,1);
         lcd.print("Successfully!");
         delay(1000);
         acknowledged = false;
         autoMode = false;
      }
      
    }
    acknowledged = false;
  }
}
