#include <Wire.h>
#include <M5Stack.h>

/////////////////////////////////////////////
//Define global variables
int i2cSpeed = 400000;
int totalMenu = 7;
String menu[] = {"Scan","Read", "Dump", "Clear", "Write Dump Back", "Write New", "Write Old"};
int E2PROM = 256;
int menuSelect = 0;
bool isMainMenu = true;

////////////////////////////////////////////
// Define functions
void clearScreen();
void readData(byte* store);
void writeData(byte* store);
void printData();
void scanI2C();
void dump();
void showMainMenu();
void clearRom();
void writeDumpBack();
void writeNew();
void writeOld();

/////////////////////////////////////////////
// Arduino Main logic
void setup() {
  M5.begin();
  SD.begin();
  Wire.begin();
  clearScreen();
  showMainMenu();
}

void loop() {
  M5.update();
  if (isMainMenu){
    if(M5.BtnC.wasReleased()){
      menuSelect++;
      if(menuSelect >= totalMenu) menuSelect=0;
      showMainMenu();
    }
    else if(M5.BtnA.wasReleased()){
      menuSelect--;
      if(menuSelect < 0) menuSelect = totalMenu-1;
      showMainMenu();
    }
    else if(M5.BtnB.wasReleased()){
      if (menuSelect == 0){
        scanI2C();
        isMainMenu=false;
      }
      else if (menuSelect == 1) {
        printData();
        isMainMenu=false;
      }
      else if (menuSelect == 2){
        dump();
        isMainMenu = false;
      }
      else if (menuSelect == 3){
        clearRom();
        isMainMenu = false;
      }
      else if (menuSelect == 4){
        writeDumpBack();
        isMainMenu=false;
      }
      else if (menuSelect == 5){
        writeNew();
        isMainMenu=false;
      }
      else if (menuSelect == 6){
        writeOld();
        isMainMenu=false;
      }
    }
  }
}

/////////////////////////////////////////////
// function implementation
void showMainMenu(){
  clearScreen();
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(3);
  M5.Lcd.println("     IDBOARD");
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  for(int i = 0;i < totalMenu;i++){
    if (i == menuSelect) M5.Lcd.print('>');
    M5.Lcd.println(menu[i]);
  }
}

void clearScreen(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0,0);
  delay(500);
}

void readData(byte* store){
  Wire.setClock(i2cSpeed);
  // write first to register read from 0x00 address of EEPROM
  Wire.beginTransmission(0x55);
  Wire.write(0);
  Wire.endTransmission();
  // Arduino I2C read/write buf has only 32 bytes, 
  int buf = 32;
  int index = 0;
  for(int i = 0; i< E2PROM/buf; i++){
    Wire.requestFrom(0x55, buf);    
    while (Wire.available()) { 
      store[index++]= Wire.read();         
    }
    delay(50); 
  }
}

void writeData(byte* store){  
  // Arduino I2C read/write buf has only 32 bytes
  // as per write, address needed to transfer first
  // only write 16 bytes at a time
  int buf = 32/2;
  byte index = 0;
  byte dump[buf];

  Wire.setClock(i2cSpeed);
  // write EEPROM, 16 bytes data + 1 address per transmission
  for(int i = 0; i< E2PROM/buf; i++){
    for (int j = 0; j<E2PROM/buf;j++){
      dump[j]=store[j+index];
    }
    Wire.beginTransmission(0x55);  
    Wire.write(index);
    Wire.write(dump, E2PROM/buf);
    Wire.endTransmission();
    delay(50); 
    index += E2PROM/buf;
  }
}


void printData(){
  clearScreen();
  M5.Lcd.println("Reading data.");
  byte data[E2PROM];
  readData(data);
  for(int i = 0; i<E2PROM; i++){
    if (data[i] == 0) M5.Lcd.print('.');
    else M5.Lcd.print((char)data[i]);
  }
  M5.Lcd.println("\nRead finish");

}

void scanI2C(){
  clearScreen();
  int address;
  int error;
  M5.Lcd.println("scanning Address [HEX]");
  
  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if(error==0)
    {
      M5.Lcd.print(address,HEX);
      M5.Lcd.print(" ");
    }
    else M5.Lcd.print(".");

    delay(10);
  }
}

void dump(){
  clearScreen();
  int line = 32;
  byte dump[E2PROM];
  int index = 0;

  // read data from EEPROM
  M5.Lcd.println("First read data from EEPROM.");
  readData(dump);

  // write to file
  M5.Lcd.println("\nWrite file to /dump.dll");
  File file = SD.open("/dump.dll", FILE_WRITE);
  if(!file){
    M5.Lcd.println("Failed to write file dump.dll");
    return;
  }
  for(int i = 0;i < E2PROM;i++){
    if(file.print((char)dump[i])){
    }
    else{
      M5.Lcd.println("failed");
      break;
    }
  }
  file.close();
  M5.Lcd.println("Write OK. read file for verify. Please wait.");

  // read from file
  char readfile[E2PROM];
  index = 0;
  file = SD.open("/dump.dll");
  while(file.available()){
    readfile[index++]=file.read();
  }
  file.close();

  // compare and verify
  for (int i = 0;i<E2PROM;i++) {
    if(dump[i] != readfile[i]) {
      M5.Lcd.println("Error found during verify");
      M5.Lcd.print("\nOrgin data: ");
      M5.Lcd.print(dump[i]);
      M5.Lcd.print("\nRead data: ");
      M5.Lcd.print(readfile[i]);
      break;
    }
  }
  M5.Lcd.println("\n\nDump OK. Verify OK.");

}

void clearRom(){
  byte data[E2PROM];
  Wire.setClock(i2cSpeed);
  for (int i = 0;i < E2PROM;i++){
    data[i] = 0xff;
  }
  writeData(data);
  clearScreen();
  M5.Lcd.println("Clear finish");
}

void writeDumpBack(){
  clearScreen();
  int index = 0;
  byte readfile[E2PROM];

  // read file
  M5.Lcd.println("First read data from TF card.");
  File file = SD.open("/dump.dll");
  if(!file){
    M5.Lcd.println("Failed to open file dump.dll");
    return;
  }
  while(file.available()){
    readfile[index++]=(byte)file.read();
  }

  // write data
  M5.Lcd.println("\nThen write back to EEPROM.");
  writeData(readfile);
  M5.Lcd.println("Write Done!");

  // dump again and verify
  byte dump[E2PROM];
  readData(dump);
  for (int i = 0;i < E2PROM; i++){
    if(dump[i]!=readfile[i]){
      M5.Lcd.println("Error found during verify");
      M5.Lcd.print("\nOrgin data: ");
      M5.Lcd.print(dump[i]);
      M5.Lcd.print("\nRead data: ");
      M5.Lcd.print(readfile[i]);
      break;
    }
  }
  M5.Lcd.println("Verify OK.");
}

void writeNew(){
  clearScreen();
  int index = 0;
  byte readfile[E2PROM];

  M5.Lcd.println("First read data from TF card.");
  File file = SD.open("/new.dll");
  if(!file){
    M5.Lcd.println("Failed to open file new.dll");
    return;
  }
  while(file.available()){
    readfile[index++]=(byte)file.read();
  }

  M5.Lcd.println("\nThen write back to EEPROM.");
  writeData(readfile);
  M5.Lcd.println("Write Done!");

  // dump and verify
  byte dump[E2PROM];
  readData(dump);
  for (int i = 0;i < E2PROM; i++){
    if(dump[i]!=readfile[i]){
      M5.Lcd.println("Error found during verify");
      M5.Lcd.print("\nOrgin data: ");
      M5.Lcd.print(dump[i]);
      M5.Lcd.print("\nRead data: ");
      M5.Lcd.print(readfile[i]);
      break;
    }
  }
  M5.Lcd.println("Verify OK.");
}

void writeOld(){
  clearScreen();
  int index = 0;
  byte readfile[E2PROM];

  M5.Lcd.println("First read data from TF card.");
  File file = SD.open("/old.dll");
  if(!file){
    M5.Lcd.println("Failed to open file old.dll");
    return;
  }
  while(file.available()){
    readfile[index++]=(byte)file.read();
  }

  M5.Lcd.println("\nThen write back to EEPROM.");
  writeData(readfile);
  M5.Lcd.println("Write Done!");

  // dump and verify
  byte dump[E2PROM];
  readData(dump);
  for (int i = 0;i < E2PROM; i++){
    if(dump[i]!=readfile[i]){
      M5.Lcd.println("Error found during verify");
      M5.Lcd.print("\nOrgin data: ");
      M5.Lcd.print(dump[i]);
      M5.Lcd.print("\nRead data: ");
      M5.Lcd.print(readfile[i]);
      break;
    }
  }
  M5.Lcd.println("Verify OK.");
}
