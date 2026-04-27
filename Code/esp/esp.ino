#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "Wire.h" //esp as master
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <UnixTime.h>
#include "Arduino.h"
#include "uEEPROMLib.h"
uEEPROMLib eeprom(0x57);
UnixTime stamp(8);
LiquidCrystal_I2C lcd(0x3f,16,2);
RTC_DS3231 rtc;

#define RED 16
#define GRE 14
#define BLU 12
//memory location
#define STARTUNIX 0
//rgb
#define COMMON_ANODE

String   messagetotelegram, keyboardJson, keyboardJsonmenu, datafromarduino,
         humidity,temperature,heater,humidifier,pir;
uint8_t  rtchour, rtcminute, rtcsecond, rtcday, rtcmonth,
         ntphour, ntpminute, ntpsecond, ntpday, ntpmonth,
         startday, startmonth,
         endday, endmonth,
         page=1,nowday,phase,timediff,humidityint,
         nowdayprev, hourprev;
uint16_t rtcyear,ntpyear,startyear,endyear;
uint32_t rtccurrentunix,ntpcurrentunix,startunix,endunix,botRequestDelay = 500,
         lastTimeBotRan,lastTimeBotRan2,lastTimeBotRan3;
char     rtcdateandtime[17],ntpdateandtime[17],startdate[10],enddate[10],phaseint[1];
int      rgb[3];
bool     sendmessagedaily=1,sendmessagehourly=1,humidifierstate,heaterstate,pirstate,loading=1;

//SSID
char     ssid[] = "hahfizin";
//SSID password
char password[] = "passwordbaru";
//BOT token
//#define BOTtoken "5479682957:AAFcvm9etx7paWhLyqY5co4ijWjL7-KELbw"  //hahafizinbot
char BOTtoken[] = "5617596844:AAFqx1gfNvNJ2W2iJvTb6GAuUyyuNfUeO2I";  //eggincubatorbot
//chat ID
char CHAT_ID[] = "936077087"; //hahafizinbot
//#define CHAT_ID "-1001811075374" //channel

//utp
const long utcOffsetInSeconds = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

X509List cert(TELEGRAM_CERTIFICATE_ROOT);

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  Wire.begin(4, 5);
  Serial.begin(9600);
  rtc.begin();
  lcd.init();
  lcd.backlight();
  timeClient.begin();

  keyboardJsonmenu = "[[\"Get data\"],[\"Set page\"],[\"Set start date\"],[\"ON/OFF lamp\"],[\"Other\"]]";
  
  pinMode(RED, OUTPUT);
  pinMode(GRE, OUTPUT);
  pinMode(BLU, OUTPUT);

  digitalWrite(GRE, HIGH);
  digitalWrite(BLU, HIGH);
  for (int i=255; i>=200; i--) {
    analogWrite(RED, i);
    delay(13);
  }

  lcdState(1);

  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.hostname("Quail Egg Hatcher");
  WiFi.begin(ssid, password);

  //test semua led
  onLed(0,365);

  //masukkan nilai dalam eeprom ke variable startunix
	eeprom.eeprom_read(STARTUNIX, &startunix);

  //tunggu untuk wifi dah connect
  while (WiFi.status() != WL_CONNECTED) delay(1);

  onLed(0,60);
  lcdState(2);
  
  messagetotelegram = "Connected to " + String(ssid) + " network. ESP Restarted.\nList of available commands:";
  bot.sendMessageWithReplyKeyboard(CHAT_ID, messagetotelegram, "", keyboardJsonmenu, true);
  onLed(60,120);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) ESP.reset();
  
  if (loading) lcdState(9); else {
    if      (page==1) lcdState(3);
    else if (page==2) lcdState(4);
    else if (page==3) lcdState(5);
    else if (page==4) lcdState(6);
    else if (page==5) lcdState(7);
    else if (page==6) lcdState(8);
  } 

  //untuk hantar mesej sekali sehari
  nowdayprev=nowday;
  hourprev  =rtchour;

  //time from internet
  timeClient.update();
  stamp.getDateTime(timeClient.getEpochTime());
  ntpcurrentunix  = timeClient.getEpochTime();
  ntphour         = stamp.hour;
  ntpminute       = stamp.minute;
  ntpsecond       = stamp.second;
  ntpday          = stamp.day;
  ntpmonth        = stamp.month;
  ntpyear         = stamp.year;
  sprintf(ntpdateandtime,"NTP: %02d:%02d %02d/%02d",ntphour,ntpminute,ntpday,ntpmonth);

  //time from RTC
  DateTime now = rtc.now();
  rtccurrentunix  = now.unixtime()-28800; //tolak 28800 sbb GMT=+08:00 (8 jam = 28800 saat)
  rtchour         = now.hour();
  rtcminute       = now.minute();
  rtcsecond       = now.second();
  rtcday          = now.day();
  rtcmonth        = now.month();
  rtcyear         = now.year();
  sprintf(rtcdateandtime,"%02d:%02d %02d/%02d/%04d",rtchour,rtcminute,rtcday,rtcmonth,rtcyear);

  //start date
  stamp.getDateTime(startunix);
  startday        = stamp.day;
  startmonth      = stamp.month;
  startyear       = stamp.year;
  sprintf(startdate,"%02d/%02d/%02d",startday,startmonth,startyear);
  //expected end date
  endunix = startunix+(86400*18); //tambah 18 hari
  stamp.getDateTime(endunix);
  endday          = stamp.day;
  endmonth        = stamp.month;
  endyear         = stamp.year;
  sprintf(enddate,"%02d/%02d/%02d",endday,endmonth,endyear);

  timediff = ntpsecond-rtcsecond;
  
  //kalau masa dah nak lari, set balik masa kat rtc ikut masa internet
  if (timediff>=10) {
    timeClient.update();
    stamp.getDateTime(timeClient.getEpochTime());
    rtc.adjust(DateTime(stamp.year,stamp.month,stamp.day,stamp.hour,stamp.minute,stamp.second));
  }
  
  nowday=1+((rtccurrentunix-startunix)/86400);

  if (nowdayprev!=nowday)  sendmessagedaily =1;
  if (hourprev  !=rtchour) sendmessagehourly=1;
  sendNotifications(nowday,rtchour,pirstate);
  
  //tentukan suhu dan kelembapan telur burung puyuh untuk setiap fasa
  if      (nowday >=0  && nowday <=7 ) phase=1;
  else if (nowday >=8  && nowday <=13) phase=2;
  else if (nowday >=14 && nowday <=18) phase=3;
  else if (nowday <=-1 || nowday > 18) phase=0;

  sprintf(phaseint, "%d", phase);
  
  if (millis() > lastTimeBotRan + botRequestDelay) {
    //request mesej dari telegram
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      //Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  if (millis() > lastTimeBotRan2 + 500) {
  //bagitau arduino phase ke berapa dah, nanti arduino maintainkan.
    Wire.beginTransmission(8);
    Wire.write(phaseint);  
    Wire.endTransmission();
    
    //address 8, request 15 huruf sahaja "11,11.11,1,1"
    Wire.requestFrom(8, 14);  
    //kosongkan nilai dalam variable datafromarduino
    datafromarduino="";
    //baca mesej dari arduino satu persatu dan combine
    while (0 < Wire.available()) { char c = Wire.read(); datafromarduino += c; }
    humidity        = datafromarduino.substring(0,2);
    temperature     = datafromarduino.substring(3,8);
    heater          = datafromarduino.substring(9,10);
    humidifier      = datafromarduino.substring(11,12);
    pir             = datafromarduino.substring(13,14);
    humidityint     = humidity.toInt();
    humidifierstate = humidifier.toInt();
    heaterstate     = heater.toInt();
    pirstate        = pir.toInt();

    //jika data masih belum sampai lagi, setkan var loading kpd 1.
    if (humidity != "00") loading=0;

    lastTimeBotRan2 = millis();
  }
}

//Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (text == "/start") {
      onLed(120,60);
      messagetotelegram  = "Welcome to Quail Egg Incubator\n";
      messagetotelegram += "List of available commands:";
      bot.sendMessageWithReplyKeyboard(CHAT_ID, messagetotelegram, "", keyboardJsonmenu, true);
      onLed(60,120);
    }
    
    else if (text == "Get data") {
      onLed(120,60);
      //data from arduino
      messagetotelegram  = "Reported data ``` \n\nDHT 11 sensor\nTemperature : " + temperature + " °C\nHumidity    : " + humidity;
      messagetotelegram += ".00 %";
      if (humidityint <= 45 && !loading) messagetotelegram += " Please check water tank!";
      messagetotelegram += "\n\nOutput state\nHeater      : ";
      if (heaterstate)     messagetotelegram += "ON\nHumidifier  : "; else messagetotelegram += "OFF\nHumidifier  : ";
      if (humidifierstate) messagetotelegram += "ON";                 else messagetotelegram += "OFF";
      //add data from esp - date, day count and phase
      messagetotelegram += "\n\nStart date  : " + String(startday) + "/" + String(startmonth) + "/" + String(startyear);
      messagetotelegram += "\nEnd date    : "   + String(endday)   + "/" + String(endmonth)   + "/" + String(endyear);
      messagetotelegram += "\n\nDay count   : "  + String(nowday)   + "\nPhase       : " + String(phase) + "\n\n";
      if      (phase == 0) messagetotelegram += "Maintaining temperature 37.5 °C and humidity from 50% - 60%";
      else if (phase == 1) messagetotelegram += "Maintaining temperature 38.5 °C and humidity from 50% - 60%";
      else if (phase == 2) messagetotelegram += "Maintaining temperature 38.5 °C and humidity from 60% - 65%";
      else if (phase == 3) messagetotelegram += "Maintaining temperature 38.5 °C and humidity from 60% - 65%";
      messagetotelegram += "```";
      //hantar mesej
      bot.sendMessage(chat_id, messagetotelegram, "markdownV2");
      onLed(60,120);
    }
    
    else if (text == "Set start date") {
      onLed(120,60);
      messagetotelegram  = "Current start date  : " + String(startday) + "/" + String(startmonth) + "/" + String(startyear);
      messagetotelegram += "\n\nChoose to set the date manually or the same as the current date.\n";
      keyboardJson = "[[\"Set manually\", \"Same as the current date\"],[\"Cancel\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text == "Same as the current date") {
      onLed(120,60);
      //convert day,month and year ke unix utk masukkan ke  dlm variable startunix.
      stamp.setDateTime(rtcyear, rtcmonth, rtcday, 0, 0, 0); 
      startunix=stamp.getUnix();
      if (eeprom.eeprom_write(STARTUNIX, startunix)) {
        bot.sendMessage(chat_id, "Done set start date to ```" + String(rtcday) + "/" + String(rtcmonth) + "/" + String(rtcyear) + "```\nUnix : ```" + String(startunix) + "```", "markdownV2");
        bot.sendMessageWithReplyKeyboard(chat_id, "List of available commands:", "", keyboardJsonmenu, true);
      }
      onLed(60,120);
    }
    else if (text == "Set manually") {
      onLed(120,60);
      messagetotelegram = "Select a year.";
      keyboardJson = "[[\"2022\"],[\"2023\"],[\"2024\"],[\"2025\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text == "2022" || text == "2023" || text == "2024" || text == "2025") {
      onLed(120,60);
      //tukar string dalam var text ke integer utk dimasukkan ke var startyear
      startyear = text.toInt();
      messagetotelegram = "Year = " + String(startyear) + ". Select a month.";
      keyboardJson = "[[\"1 - Jan\",\"2 - Feb\",\"3 - Mar\"],[\"4 - Apr\",\"5 - May\",\"6 - Jun\"],[\"7 - Jul\",\"8 - Aug\",\"9 - Sep\"],[\"10 - Oct\",\"11 - Nov\",\"12 - Dec\"],[\"Cancel\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text == "1 - Jan"  || text == "2 - Feb" || text == "3 - Mar" || text == "4 - Apr" || text == "5 - May"  ||
             text == "6 - Jun"  || text == "7 - Jul" || text == "8 - Aug" || text == "9 - Sep" || text == "10 - Oct" ||
             text == "11 - Nov" || text == "12 - Dec") {
      onLed(120,60);
      //tuka string dari variable text ke int utk masukkan ke variable startmonth
      if      (text=="1 - Jan")  startmonth = 1;
      else if (text=="2 - Feb")  startmonth = 2;
      else if (text=="3 - Mar")  startmonth = 3;
      else if (text=="4 - Apr")  startmonth = 4;
      else if (text=="5 - May")  startmonth = 5;
      else if (text=="6 - Jun")  startmonth = 6;
      else if (text=="7 - Jul")  startmonth = 7;
      else if (text=="8 - Aug")  startmonth = 8;
      else if (text=="9 - Sep")  startmonth = 9;
      else if (text=="10 - Oct") startmonth = 10;
      else if (text=="11 - Nov") startmonth = 11;
      else if (text=="12 - Dec") startmonth = 12;
      //set start month
      messagetotelegram = "Year = " + String(startyear) + ", Month = " + String(startmonth) + ". Select a day:";
      keyboardJson = "[[\"1\",\"2\",\"3\",\"4\",\"5\"],[\"6\",\"7\",\"8\",\"9\",\"10\"],[\"11\",\"12\",\"13\",\"14\",\"15\"],[\"16\",\"17\",\"18\",\"19\",\"20\"],[\"21\",\"22\",\"23\",\"24\",\"25\"],[\"26\",\"27\",\"28\",\"29\",\"30\"],[\"31\",\"Cancel\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text == "1"  || text == "2"  || text == "3"  || text == "4"  || text == "5"  || 
             text == "6"  || text == "7"  || text == "8"  || text == "9"  || text == "10" || 
             text == "11" || text == "12" || text == "13" || text == "14" || text == "15" || 
             text == "16" || text == "17" || text == "18" || text == "19" || text == "20" || 
             text == "21" || text == "22" || text == "23" || text == "24" || text == "25" || 
             text == "26" || text == "27" || text == "28" || text == "29" || text == "30" || 
             text == "31" ){
      onLed(120,60);
      startday = text.toInt();
      //convert day,month and year ke unix.
      stamp.setDateTime(startyear,startmonth,startday,0, 0, 0); 
      startunix=stamp.getUnix();
      if (eeprom.eeprom_write(STARTUNIX, startunix)) {
        messagetotelegram = "Done set start date to ```" + String(startday) + "/" + String(startmonth) + "/" + String(startyear) + "```\nUnix : ```" + String(startunix) + "```";
        bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "markdownV2", keyboardJsonmenu, true);
      }
      onLed(60,120);
    }

    else if (text == "ON/OFF lamp") {
      onLed(120,60);
      keyboardJson = "[[\"ON lamp\",\"OFF lamp\"],[\"Done\"]]";
      messagetotelegram="Choose to turn the light on or off.";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text =="ON lamp"){
      Wire.beginTransmission(8);
      Wire.write('4');
      Wire.endTransmission();
    }
    else if (text =="OFF lamp"){
      Wire.beginTransmission(8);
      Wire.write('5');
      Wire.endTransmission();
    }

    else if (text == "Set page") {
      onLed(120,60);
      messagetotelegram = "Select the following options to display on the LCD.\n\n";
      messagetotelegram += "Page 1 - Temperature and humidity\n";
      messagetotelegram += "Page 2 - Heater and humidifier status (ON or OFF).\n";
      messagetotelegram += "Page 3 - Day count and incubation phase.\n";
      messagetotelegram += "Page 4 - The humidity and temperature that needs to be maintained and the current phase.\n";
      messagetotelegram += "Page 5 - Start date and estimated end date of incubation.\n";
      messagetotelegram += "Page 6 - Current date & time.\n";
      keyboardJson = "[[\"Page 1\",\"Page 2\",\"Page 3\"],[\"Page 4\",\"Page 5\",\"Page 6\"],[\"Done\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    //temperature and humidity
    else if (text =="Page 1") {
      onLed(120,60);
      page=1;
      onLed(60,120);
    }
    //heater and humidifier state
    else if (text =="Page 2") {
      onLed(120,60);
      page=2;
      onLed(60,120);
    }
    //Day count and phase of hatching
    else if (text =="Page 3") {
      onLed(120,60);
      page=3;
      onLed(60,120);
    }
    //Current phase, humidity and temperature to be maintained.
    else if (text =="Page 4") {
      onLed(120,60);
      page=4;
      onLed(60,120);
    }
    //Start date & end date
    else if (text =="Page 5") {
      onLed(120,60);
      page=5;
      onLed(60,120);
    }
    //Current date & time.
    else if (text =="Page 6") {
      onLed(120,60);
      page=6;
      onLed(60,120);
    }
    
    else if (text =="Other") {
      onLed(120,60);
      messagetotelegram = "Choose the following options:\n\n1. View SSID and password.";
      keyboardJson = "[[\"View SSID & password\"],[\"Cancel\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }
    else if (text =="View SSID & password") {
      onLed(120,60);
      messagetotelegram  = "SSID : " + String(ssid);
      messagetotelegram += "\nPassword : " + String(password);
      keyboardJson = "[[\"Done\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJson, true);
      onLed(60,120);
    }

    else if (text == "Cancel" || text == "Done") {
      onLed(120,60);
      messagetotelegram = "List of available commands:";
      bot.sendMessageWithReplyKeyboard(chat_id, messagetotelegram, "", keyboardJsonmenu, true);
      onLed(60,120);
    }
    else if (text == "Sync") {
      onLed(120,60);
      //set masa kat rtc berdasarkan ntp
      timeClient.update();
      stamp.getDateTime(timeClient.getEpochTime());
      rtc.adjust(DateTime(stamp.year,stamp.month,stamp.day,stamp.hour,stamp.minute,stamp.second));
      //hantar mesej
      messagetotelegram="Done syncing time.";
      bot.sendMessage(chat_id, messagetotelegram, "");
      onLed(60,120);
    }
    else {
      onLed(120,30);
      messagetotelegram="Invalid command.";
      bot.sendMessage(chat_id, messagetotelegram, "");
      onLed(30,120);
    }
  }
}
void sendNotifications(int nowday, int hour, bool movement) {
  String messagedaily   ="";
  String messagehourly  ="";
  String humiditymessage="";
  
  if (nowday == 1 || nowday == 7 || nowday == 14) {
    messagedaily   = "Message from incubator :\n";
    messagedaily  += "Day " + String(nowday) + ". Please check the quail eggs with an ovoscope.";
    if (sendmessagedaily) {
      onLed(120,180);
      bot.sendMessage(CHAT_ID, messagedaily, "");
      onLed(180,120);
    }
  }
  else if (hour == 7 || hour == 12 || hour == 17 || hour == 21) {
    messagehourly  = "Message from incubator :\n";
    messagehourly += "It's "+ String(hour) +" o'clock now. Please turn over the quail eggs.";
    if (sendmessagehourly) {
      onLed(120,180);
      bot.sendMessage(CHAT_ID, messagehourly, "");
      onLed(180,120);
    }
  }

  sendmessagedaily=0;
  sendmessagehourly=0;

  //kalau humidity kurang 46, hantar mesej setiap 30 saat
  if (millis() > lastTimeBotRan3 + 1000*30) {
    if (humidityint <= 45 && !loading) {
      onLed(120,0);
      humiditymessage  = "Message from incubator :\n";
      humiditymessage += "Your incubator is lacking humidity (" + String(humidityint) + "%). Please refill the water in the tank as soon as possible.";
      bot.sendMessage(CHAT_ID, humiditymessage, "");
      onLed(0,120);
    }
    lastTimeBotRan3 = millis();
  }

  if (movement) {
    onLed(120,180);
    bot.sendMessage(CHAT_ID, "Movement detected!", "");
    onLed(180,120);
  }
}
/*
LCD State
0. kosongkan
1. connecting...
2. connected
3. [display] temperature and humidity
4. [display] heater and humidifier state
5. [display] Day count and phase of hatching
6. [display] Current phase, humidity and temperature to be maintained.
7. [display] Start date & end date
8. [display] Current date & time.
*/
void lcdState(int state) {

  lcd.home();

  if (state==0){
    lcd.print(F("                "));
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  else if (state==1){
    lcd.print(F("  Connecting... "));
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  else if (state==2){
    lcd.print(F("Wi-Fi connected."));
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  else if (state==3){
    lcd.print(F("Temp  : "));lcd.print(temperature);lcd.print(F(" C  "));
    lcd.setCursor(0,1);
    lcd.print(F("Humid : "));lcd.print(humidity);   lcd.print(F(".00 % "));
  }
  else if (state==4){
    lcd.print(F("Heater     : ")); if (heaterstate)     lcd.print(F("ON ")); else lcd.print(F("OFF"));
    lcd.setCursor(0,1);
    lcd.print(F("Humidifier : ")); if (humidifierstate) lcd.print(F("ON ")); else lcd.print(F("OFF"));
  }
  else if (state==5){
    lcd.print(F("   Day   : "));lcd.print(nowday);lcd.print(F("      "));
    lcd.setCursor(0,1);
    lcd.print(F("   Phase : "));lcd.print(phase); lcd.print(F("      "));
  }
  else if (state==6){
    lcd.print(F("   Phase = "));lcd.print(phase);lcd.print(F("     "));
    lcd.setCursor(0,1);
    if      (phase == 0) lcd.print(F(" 37.5 C,50%-60% "));
    else if (phase == 1) lcd.print(F(" 38.5 C,50%-60% "));
    else if (phase == 2) lcd.print(F(" 38.5 C,60%-65% "));
    else if (phase == 3) lcd.print(F(" 38.5 C,60%-65% "));
  }
  else if (state==7){
    lcd.print(F("Start : "));lcd.print(startday);lcd.print(F("/"));lcd.print(startmonth);lcd.print(F("/"));lcd.print(startyear-2000);lcd.print(F("   "));
    lcd.setCursor(0,1);
    lcd.print(F("End   : "));lcd.print(endday);  lcd.print(F("/"));lcd.print(endmonth);  lcd.print(F("/"));lcd.print(endyear-2000);lcd.print(F("   "));
  }
  else if (state==8){
    lcd.print(F("  Current time  "));
    lcd.setCursor(0,1);
    lcd.print(rtcdateandtime);
  }
  else if (state==9){
    lcd.print(F("Wi-Fi connected."));
    lcd.setCursor(0,1);
    lcd.print(F("Getting data..."));
  }
}

/*
On LED
0   = merah
60  = kuning
120 = hijau
180 = cyan
240 = biru
300 = magenta
*/
void onLed(int oldcolor, int newcolor) {
  if (oldcolor < newcolor) {
    for (int i=oldcolor; i<=newcolor;i++) {
      hsi_to_rgb(i,1,1);
      setColor(rgb[0],rgb[1],rgb[2]);
      //Changing the delay() value in milliseconds will change how fast the
      //the light moves over the hue values 
      delay(3);
    }
  }
  if (oldcolor > newcolor) {
    for (int i=oldcolor; i>=newcolor;i--) {
      hsi_to_rgb(i,1,1);
      setColor(rgb[0],rgb[1],rgb[2]);
      //Changing the delay() value in milliseconds will change how fast the
      //the light moves over the hue values 
      delay(3);
    }
  }
}
//Arduino has no prebuilt function for hsi to rgb so we make one:
void hsi_to_rgb(float H, float S, float I) {
  int r, g, b;
  if (H > 360) {
    H = H - 360;
  }
  // Serial.println("H: "+String(H));
  H = fmod(H, 360); // cycle H around to 0-360 degrees
  H = 3.14159 * H / (float)180; // Convert to radians.
  S = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
  I = I > 0 ? (I < 1 ? I : 1) : 0;
  if (H < 2.09439) {
    r = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    g = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    b = 255 * I / 3 * (1 - S);
  } else if (H < 4.188787) {
    H = H - 2.09439;
    g = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    b = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    r = 255 * I / 3 * (1 - S);
  } else {
    H = H - 4.188787;
    b = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    r = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    g = 255 * I / 3 * (1 - S);
  }
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}
void setColor(int red, int green, int blue) {
  #ifdef COMMON_ANODE
    red   = 255 - red;
    green = 255 - green;
    blue  = 255 - blue;
  #endif
  analogWrite(RED, map(red,   0,255,215,255)); 
  analogWrite(GRE, map(green, 0,255,215,255));
  analogWrite(BLU, map(blue,  0,255,215,255));
}