// the setup function runs once when you press reset or power the board
    #include <Arduino.h>
    #include <WiFi.h>
    #include <ESP_Mail_Client.h>
    #include <string>
    #include "time.h"
    #define WIFI_SSID ""
    #define WIFI_PASSWORD "" 
    #define OWNER_EMAIL "dooralarm24@gmail.com"
    #define OWNER_PASSWORD "eyqkepkztrtobbxl"
    #define RECIPIENT_EMAIL "cyrasebastian01@gmail.com"
    #define SMTP_HOST "smtp.gmail.com"
    #define SMTP_PORT 465
    #define ntpServer "pool.ntp.org"
    #define gmtOffset 3600 //was 3600
    #define daylightOffset 3600 //UTC+1 //was 3600
    #define noConnectionAttempts 2
    #define buttonPin 27
    #define buttonPinMask 0x8000000
    #define magneticSensorPin 26
    #define interruptButtonPin GPIO_NUM_27
    #define interruptMagneticSensorPin GPIO_NUM_26
    #define sizeOfTheArray 100
    #define sleepingTime_us 10000000
    RTC_DATA_ATTR bool magneticSensorPreviousState;
    bool sendData(); //Used to send an email
    void buttonPressed();
    void actionPerformed();
    //Class record, contains string with date and bool to notify kind of performed action.
    //Without sets and gets to reduce program storage space just a little bit
    class record {
      char date[20];
      bool closure;
      public:      
      record() {
      }
      std::string getDate() {
        return date;
      }
      bool getClosure() {
        return closure;
      }
      void setDate(std::string date) {
        strcpy(this->date, date.c_str());
      }
      void setClosure(bool closure) {
        this->closure = closure;
      }
    };
    
    RTC_DATA_ATTR record array[sizeOfTheArray];
    //contains the amount of unsent data
    RTC_DATA_ATTR int actualNumberOfActions;
    //when array is overloaded it starts to remove oldest actions
    RTC_DATA_ATTR bool arrayOverloaded;
    
    void setup() {
      // initialize digital pin LED_BUILTIN as an output.
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);
      Serial.begin(115200);
      wakeup_reason();
      //pinMode(LED_BUILTIN, OUTPUT);
      
      //pinMode(buttonPin, INPUT_PULLDOWN);
      pinMode(buttonPin, INPUT);
      pinMode(magneticSensorPin, INPUT);
      
      //attachInterrupt(digitalPinToInterrupt(magneticSensorPin), actionPerformed, CHANGE);
      //attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPressed, FALLING);
      Serial.print("Current size of the array: ");
      Serial.println(actualNumberOfActions);      
      Serial.println("Setup done.");
      esp_sleep_enable_ext1_wakeup(buttonPinMask, ESP_EXT1_WAKEUP_ALL_LOW);
      esp_sleep_enable_ext0_wakeup(interruptMagneticSensorPin, !magneticSensorPreviousState);
      esp_deep_sleep_start(); //go to sleep      
    }

    // the loop function runs over and over again forever
    void loop() {
      Serial.println("loop");
      delay(3000);                       // wait for a second
}   

/*
Checks the reason of waking up.
*/
void wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : 
      Serial.println("Wakeup caused by magnetic sensor."); 
      actionPerformed();
      break;
    case ESP_SLEEP_WAKEUP_EXT1 : 
      Serial.println("Wakeup caused by the button"); 
      buttonPressed();
      break;
    default:
      Serial.println("No reason"); 
      magneticSensorPreviousState = false; 
      arrayOverloaded = false;
      actualNumberOfActions = 0;
      break;
  }
}

  void buttonPressed() {
    Serial.println("Button pressed.");
    esp_sleep_enable_timer_wakeup(sleepingTime_us); //set sleeping time
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);  //turn off all RTC peripherals
    esp_deep_sleep_start(); //go to sleep
  }

  void actionPerformed() {
    magneticSensorPreviousState = !magneticSensorPreviousState;
    if(sendData()) {
      actualNumberOfActions = 0;
      arrayOverloaded = false;
    }
    else {
      if(actualNumberOfActions + 1 <= sizeOfTheArray) {
        addToTheList();        
      }
      else {
        arrayOverloaded = true;
        actualNumberOfActions = 0;
        addToTheList();
      }
    }
    //esp_sleep_enable_ext0_wakeup(interruptButtonPin, LOW);
    //esp_sleep_enable_ext0_wakeup(interruptMagneticSensorPin, !magneticSensorPreviousState);
    //ESP.deepSleep();
    //esp_deep_sleep_start(); //go to sleep    
  }  

  void addToTheList() {
    struct tm time;
    if(actualNumberOfActions >= 0 && actualNumberOfActions <= sizeOfTheArray && getLocalTime(&time)) {
      std::string temp = std::to_string(time.tm_mday) + "." + std::to_string(time.tm_mon + 1) + "." + std::to_string(1900+time.tm_year) +
       "  " + std::to_string(time.tm_hour) + ":" + std::to_string(time.tm_min) + ":" + std::to_string(time.tm_sec);      
      array[actualNumberOfActions].setDate(temp);
      array[actualNumberOfActions].setClosure(magneticSensorPreviousState);
      ++actualNumberOfActions; 
    }
  }

 bool sendData() {
   Serial.println("Entered sendData()!");
   //WiFi.mode(WIFI_STA); //check if it doesnt work
   for(int numberOfAttempt = 0; numberOfAttempt < noConnectionAttempts; ++numberOfAttempt) {
     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
     if(WiFi.waitForConnectResult() == WL_CONNECTED) {
       if(successfullyConnected()) {
         Serial.println("successfullyConnected() true! from sendData");
         //WiFi.disconnect(true, true); //test
         //WiFi.mode(WIFI_OFF);   //test
         return true;        
       }
       Serial.println("successfullyConnected() false! from sendData");
     }
   }
   Serial.println("sendData() returns false!");
   return false;
 }

 bool successfullyConnected() {
  configTime(gmtOffset, daylightOffset, ntpServer);
  Serial.println("Entered successfullyConnected()!");
  /* The SMTP Session object used for Email sending */
    SMTPSession smtp;
   /* Declare the session config data */
   ESP_Mail_Session currentSession;
   /* Set the session config */
   currentSession.server.host_name = SMTP_HOST;
   currentSession.server.port = SMTP_PORT;
   currentSession.login.email = OWNER_EMAIL;
   currentSession.login.password = OWNER_PASSWORD;
   currentSession.login.user_domain = "";
   Serial.println("After currentSession changes!");
   /* Declare the message class */
   SMTP_Message message;
   /* Set the message headers */
   message.sender.name = "ESP";
   message.sender.email = OWNER_EMAIL;
   message.subject = "Door status has changed";
   message.addRecipient("Sebastian", RECIPIENT_EMAIL);
   //Send raw text message
   std::string textMsg;
   struct tm time;
    if(getLocalTime(&time)) {
      textMsg = magneticSensorPreviousState ? "closed" : "opened";
      textMsg = "Door has been " + textMsg + "!\n" + "Exact time: ";    
      textMsg = textMsg + std::to_string(time.tm_mday) + "." + std::to_string(time.tm_mon + 1) + "." + std::to_string(1900+time.tm_year) +
       "  " + std::to_string(time.tm_hour) + ":" + std::to_string(time.tm_min) + ":" + std::to_string(time.tm_sec) + "\n\n";
      //char position[30];
      //textMsg = textMsg + month() + "\n\n";
          
      //sprintf(position, "%A %d %B %Y %H:%M:%S", &time);
      textMsg = textMsg + "\n\n";    
      if(arrayOverloaded) {
        for(int i = actualNumberOfActions - 1; i >= 0 ; --i) {
          textMsg = textMsg + array[i].getDate() + "   " + (array[i].getClosure() ? "closed" : "opened") + "\n";
        }
        for(int i = sizeOfTheArray - 1; i >= actualNumberOfActions ; --i) {
          textMsg = textMsg + array[i].getDate() + "   " + (array[i].getClosure() ? "closed" : "opened") + "\n";
        }
      }
      else{
        for(int i = actualNumberOfActions - 1; i >= 0 ; --i) {
          textMsg = textMsg + array[i].getDate() + "   " + (array[i].getClosure() ? "closed" : "opened") + "\n";
        }        
      }      
    }
   else {
     textMsg = "Fatal error occured";     
   }
   message.text.content = textMsg.c_str();
   message.text.charSet = "us-ascii";
   message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
   Serial.println("After message changes (before sendMail)!");
   /* Connect to server with the session config */
   if (!smtp.connect(&currentSession)) {
     Serial.println("smtp.connect(&currentSession) FALSE");
     return false;
   }
   /* Start sending Email and close the session */
   if (!MailClient.sendMail(&smtp, &message)) {
     Serial.println("Error sending Email, " + smtp.errorReason());
     return false;
   }
Serial.println("successfullyConected() returns true");  
 
   return true;
 }
