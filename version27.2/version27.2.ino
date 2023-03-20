#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>  //Create software serial object to communicate with SIM800L
SoftwareSerial GSM(11, 12);    //SIM800L Tx is 11 & Rx is 12 connected to Arduino

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

char phone_no[] = "+84973915035";  //change +84 with Vietnam country code and 973915035 with phone number to sms

#define FlameSensorPin 8  // choose the input pin for Flame sensor
#define GasSensorPin 2    // choose the input pin for Gas sensor
#define TempsensorPin 3   // choose the input pin for Temperature sensor
#define buzzer 6          // choose the pin for the Buzzer
#define led_C 9           // choose the pin for the Green LED Call/Message active indication
#define led_M 10          // choose the pin for the Yellow LED Message active indication
#define led_F 5           // choose the pin for the Red LED Fire detection indication
int read_value;           // variable for reading the sensorpin status
int sms_Status, call_Status;
int flag = 0;
int var_1 = 0, var_2 = 0;
char input_string[15];  // will hold the incoming character from the GSM field


int val_gas = 0;
int val_temp = 0; int temp_range = 0;
int val_flam = 0;

//----------------------------------------- SET UP ----------------------------------------------------

void setup() {
  Serial.begin(9600);              // Begin Serial communication Arduino and Arduino IDE (Serial Monitor)
  GSM.begin(9600);                 // Begin serial communication with Arduino and SIM800L
  lcd.init();                      // initialize the lcd
  lcd.backlight();                 // print a message to the LCS
  pinMode(GasSensorPin, INPUT);    // GAS sensor
  pinMode(TempsensorPin, INPUT);   // Temperature Sensor
  pinMode(led_F, OUTPUT);          // Declare Red LED as outputv - fire
  pinMode(led_C, OUTPUT);          // declare Green LED as output - call
  pinMode(led_M, OUTPUT);          // declare Yellow LED as output - message
  pinMode(buzzer, OUTPUT);         // Buzzer
  pinMode(FlameSensorPin, INPUT);  // FLame Sensor

  Serial.println("Initializing...");
  delay(1000);

  initModule("AT", "OK", 1000);                 //Once the handshake test is successful, it will back to OK
  initModule("ATE1", "OK", 1000);               //this command is used for enabling echo
  initModule("AT+CPIN?", "READY", 1000);        //this command is used to check whether SIM card is inserted in GSM Module or not
  initModule("AT+CMGF=1", "OK", 1000);          //Configuring TEXT mode
  initModule("AT+CNMI=2,2,0,0,0", "OK", 1000);  //Decides how newly arrived SMS messages should be handled
  Serial.println("Initialized Successfully");

  sendSMS(phone_no, "Start GSM Fire Alert System");
  sms_Status = EEPROM.read(1);   // read EEPROM data at address 1
  call_Status = EEPROM.read(2);  // read EEPROM data at address 2
  lcd.setCursor(0, 0);  // (col, row): move to new position, the text written to the LCS will be displayed from 0,0
  lcd.print(" GSM Base Fire     ");
  lcd.setCursor(0, 1);
  lcd.print(" Security System ");
  delay(2000);
  lcd.clear();
  Serial.println("AT+CNMI=2,2,0,0,0");  // message indication to TE
  delay(500);
  Serial.println("AT+CMGF=1");  // select message format
  delay(1000);
}

//-------------------------------------------- LOOP ---------------------------------------------------------

void loop() {
  readSMS();
  updateSerial();

  val_gas =  digitalRead(2); // gas sensor output pin connected
  val_temp = analogRead(A0); // temperature sensor output pin connected
  val_flam = digitalRead(8); // flame sensor output pin connected
  Serial.println(val_gas); // see the value in serial monitor in Arduino IDE
  Serial.println(val_temp); // see the value in serial monitor in Arduino IDE
  Serial.println(val_flam); // see the value in serial monitor in Arduino IDE
  delay(100);
  /* calculate the value (Volt) from temp sensor, lm35 range (-55,150) -> 55+150 = 205
  205*5 = 1025

  */
  if(val_gas == 0 || temp_range == 1 || val_flam == 1)  // fire
  {
   Serial.print("\r");
   delay(1000);
   Serial.print("AT+CMGF=1\r");
   digitalWrite( led_F, HIGH); // led
   digitalWrite( buzzer, HIGH); // Buzzer
   lcd.setCursor(0,0);
   lcd.print(" Fire Detected     ");
   lcd.setCursor(0,1);
   lcd.print("   Be Safe    ");
   if(flag == 0 )
   {
     flag = 1;
     if (sms_Status == 1)
     {
       sendSMS(phone_no, "Fire is Detected Alert.....!!!");
     }
     delay(1000);
     if (call_Status == 1)
     {
       callUp(phone_no);
     }
   }
    delay(1000);
    Serial.print("AT+CMGS=\"+84973915035\"\r");
    delay(1000);
    //The text of the message to be sent.
    Serial.print("Fire Alert");
    delay(1000);
    Serial.write(0x1A);
  }
  else  // not fire
  {
     digitalWrite( led_F,LOW); // led
     digitalWrite( buzzer,LOW); // Buzzer
     lcd.setCursor(0,0);
     lcd.print("  FIRE NOT    ");
     lcd.setCursor(0,1);
     lcd.print("  DETECTED    ");
     digitalWrite(buzzer, LOW);
     digitalWrite(led_F, LOW);
     flag = 0;
  }
  if (var_1 == 1)
  {
    if (!(strncmp(input_string, "Sms On", 6)))
    {
      sms_Status = 1;
      EEPROM.write(1, sms_Status);
      sendSMS(phone_no, "Message is Active");
    } else 
    if (!(strncmp(input_string, "Sms Off", 7)))
    {
      sms_Status = 0;
      EEPROM.write(1, sms_Status);
      sendSMS(phone_no, "Message is Deactivate");
    } else 
    if (!(strncmp(input_string, "Call On", 7)))
    {
      call_Status = 1;
      EEPROM.write(2, call_Status);
      sendSMS(phone_no, "Call is Active");
    } else 
    if (!(strncmp(input_string, "Call Off", 8)))
    {
      call_Status = 0;
      EEPROM.write(2, call_Status);
      sendSMS(phone_no, "Call is Deactivate");
    }
    var_1 = 0;
    var_2 = 0;
  }
  digitalWrite(led_M, sms_Status); //LED On SMS Active, LED Off SMS Deactivate
  digitalWrite(led_C, sms_Status); //LED On Call Active, LED Off Call Deactivate
}


//--------------------------------------SEND SMS--------------------------------------------------

void sendSMS(char *number, char *msg) {
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"\r\n");
  //AT+CMGS=”Mobile Number” <ENTER> - Assigning recipient’s mobile number
  delay(500);
  GSM.println(msg);  // Message contents
  delay(500);
  GSM.write(byte(26));  //Ctrl+Z  send message command (26 in decimal).
  delay(5000);
}

//--------------------------------------CALL UP-------------------------------------------------

void callUp(char *number) {
  GSM.print("ATD +");
  GSM.print(number);  // call to the specific number
  GSM.println(";");   // ends with semi-colon
  delay(1000);
}

void readSMS() {
  while (GSM.available() > 0) {
    //------------ PIN HERE --------------//
    if (GSM.find("/786"))  //'/786' where 786 is 3 digit PIN.
    //--------- PIN HERE ---------//
    {
      delay(1000);
      while (GSM.available()) {
        char input_char = GSM.read();
        input_string[var_2++] = input_char;
        if (input_char == '/') {
          var_1 = 1;
          return;
        }
      }
    }
  }
}

//------------------------------------initModule-------------------------------------------

void initModule(String cmd, char *res, int t) {
  while(1)
  {
  Serial.println(cmd);
  GSM.println(cmd);
  delay(1000);
  // Serial.println(GSM.available());
  while (GSM.available()) {
    Serial.write(GSM.read());
    if(GSM.find(res))
    {
      Serial.println(res);
      delay(t);
      return;
    } else {
      Serial.println("Error");
      // Serial.println(GSM.read());
    }
  }
  // while (Serial.available()) {
  //   GSM.write(Serial.read());  //Forward what Serial received to Software Serial Port
  // }
  delay(t);
  }
}

//----------------------------------------updateSerial------------------------------------------------

void updateSerial() {
  delay(500);
  while (Serial.available()) 
  {
    GSM.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(GSM.available()) 
  {
    Serial.write(GSM.read());//Forward what Software Serial received to Serial Port
  }
}