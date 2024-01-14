#include <SD.h>
File myFile;
File root;

String get_ok;
String readString;
bool okToSendGcode = false;
int p = 0;

String n;


#define listItemsTotal 100 //maximum number of file names to store
#define listItemMaxChar 100 //maximum number of characters per file name, including null terminator
char *listItems[listItemsTotal];
char tempString[listItemMaxChar];//temporary string of the max number of characters per file name
byte listLength = 0;
char dirx[256]="/";


void setup()
{
  Serial.begin(115200);//78492
  Serial1.begin(115200);//78492
  pinMode(4, OUTPUT);
  if (!SD.begin(4))
  {
    return;
  }
  


  //to cout list of files
   scanDir();
   for(int i=0; i<listLength; i++){
        //Serial.println(listItems[i]);
    }
    n = listItems[5];

      sendGcode();

  Serial.println("done!");

}

void loop()
{ 
}

void sendGcode() {
  myFile = SD.open(n);
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
        okToSendGcode = false;
      }
    }
    myFile.close();
  }
  myFile.close();
}





void scanDir(){
  freeListMemory(); // Start by freeing previously allocated malloc pointers
  File root = SD.open(dirx);
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {break;} // no more files
    else {
//      listItems[listLength][0] = entry.name(); //add file name to list array
//      listItems[listLength] = (char[128]) malloc(128);
      sprintf(tempString, "%s", entry.name());//save file name to temporary string with null terminator at the end
      listItems[listLength] = (char *)malloc(listItemMaxChar);//assign enough memory for 100 chars to current list item pointer
      sprintf(listItems[listLength],"%s",tempString);
      listLength++; //increment counter of files
      entry.close();
    }

  }
}

void freeListMemory(){
  // If we have previous items in the list, then free the memory
      for (byte i=0; i<=listLength; i++)
        {
          free(listItems[i]);
        }
        
    listLength = 0;//reset list length
}
