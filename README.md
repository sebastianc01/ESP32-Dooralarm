# ESP32-Dooralarm
ESP32 software, uses magnetic door sensor to watch door status. When it changes, sends an email to notify. 
The button is currently used for stop the program for 10 minutes (deep sleep of the ESP32 without checking peripherls), but in the future it will set ESP32 as the access point
to let the user change some data (recipient email, wifi name and password, sender email and password). These informations have to be stored in a EEPROM memory.
It may cause a problem with the memory limitations.
