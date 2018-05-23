#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WifiUDP.h>
#include <String.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <MicroGear.h>
#include <ESP8266HTTPClient.h>

//Reed switch
int reedSwitch = D1;
int reedSwitch_status = 1;
time_t sittingBeginTime = 0;
time_t latestRepeatNotiTime = 0;
bool alreadyUpdatedsittingBeginTime = false;

// Define NTP properties
#define NTP_OFFSET   0      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
WiFiManager wifiManager;

//Time
String date;
String t;
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

//Line chat webhook server and api
const char* host = "https://sit-too-long.herokuapp.com/webhooks.php"; //change this to your linebot server ex.http://numpapick-linebot.herokuapp.com/bot.php
#define APPID   "SitTooLong"     //change this to your APPID
#define KEY     "pzcxVCfzCec49sk"     //change this to your KEY
#define SECRET  "R7QXVeVDitFl5lrKGEQi75s6R"     //change this to your SECRET
#define ALIAS   "NodeMCU1" //set name of device
#define TargetWeb "switch" //set target name of web
String userPushMessageID = "";

WiFiClient client;  //Instantiate WiFi object
String uid = "";
int timer = 0;
MicroGear microgear(client);

//State machine
#define STATE_NOT_SETUP 0
#define STATE_SETTING 1
#define STATE_ALREADY_SETUP 2
//Setting states
#define SETTING_LIMIT_HOURS 11
#define SETTING_LIMIT_MINUTES 12
#define SETTING_LIMIT_SECONDS 13
#define SETTING_REPEAT_HOURS 14
#define SETTING_REPEAT_MINUTES 15
#define SETTING_REPEAT_SECONDS 16
int limit_hours = 0;
int limit_minutes = 0;
int limit_seconds = 0;
int repeat_hours = 0;
int repeat_minutes = 0;
int repeat_seconds = 0;
int currentState = 0;
int currentSetting = 0;
bool sentFirstTimeLimitNoti = false;

//Push Bullet Notification (NOT USED)
/*
const char* WEBSITE = "api.pushingbox.com";    //pushingbox API server
const String devid1 = "vC81F783700E196A";       //device ID from Pushingbox 
Ticker notiSender;
*/

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) 
{
  msg[msglen] = '\0';
  String msgString((char *)msg);
  msgString.trim();
  String incomingUserID = msgString.substring(0, msgString.indexOf(':'));
  String remainString = msgString.substring(msgString.indexOf(':') + 1, msgString.length());
  String replyToken = remainString.substring(0, remainString.indexOf(':'));
  String textBody = remainString.substring(remainString.indexOf(':') + 1, remainString.length());
  textBody.toLowerCase();
  
  Serial.print("Incoming message -> ");
  Serial.println(textBody);
  
  //Common messages
  switch(currentState)
  {
    case STATE_NOT_SETUP:
      if(textBody.equals("setup"))
      {
        userPushMessageID = incomingUserID;
        currentState = STATE_SETTING;
        currentSetting = SETTING_LIMIT_HOURS;
        send_POST("Setting up sitting time limit...\nHow many hours?", true, "", replyToken);
      }
      else
      {
        send_POST("Please type \"setup\" first!", true, "", replyToken);
      }
    break;
    case STATE_SETTING: 
      if(incomingUserID != userPushMessageID)
      {
        send_POST("Service is currently being setup by other account.", true, "", replyToken);
        return;
      }
      
      if(isNumber(textBody))
      {
        int number = textBody.toInt();
        switch(currentSetting)
        {
          case SETTING_LIMIT_HOURS: 
            limit_hours = number;
            send_POST("Got it!\nHow many minutes?", true, "", replyToken);
            currentSetting = SETTING_LIMIT_MINUTES;
          break;
          case SETTING_LIMIT_MINUTES: 
            limit_minutes = number;
            send_POST("Got it!\nHow many seconds?", true, "", replyToken);
            currentSetting = SETTING_LIMIT_SECONDS;
          break;
          case SETTING_LIMIT_SECONDS: 
            limit_seconds = number;
            if(limit_hours == 0 && limit_minutes == 0 && limit_seconds == 0)
            {
              send_POST("Time limit cannot be 0!\nPlease input a number more than 0.", true, "",replyToken);
            }
            else
            {
              send_POST("Got it!\nNext, setting up repeating notification interval...\nHow many hours?", true, "", replyToken);
              currentSetting = SETTING_REPEAT_HOURS;
            }
          break;
          case SETTING_REPEAT_HOURS: 
            repeat_hours = number;
            send_POST("Got it!\nHow many minutes?", true, "", replyToken);
            currentSetting = SETTING_REPEAT_MINUTES;
          break;
          case SETTING_REPEAT_MINUTES: 
            repeat_minutes = number;
            send_POST("Got it!\nHow many seconds?", true, "", replyToken);
            currentSetting = SETTING_REPEAT_SECONDS;
          break;
          case SETTING_REPEAT_SECONDS: 
            repeat_seconds = number;
            if(repeat_hours == 0 && repeat_minutes == 0 && repeat_seconds == 0)
            {
              send_POST("Repeating notification interval cannot be 0!\nPlease input a number more than 0.", true, "", replyToken);
            }
            else
            {
              send_POST("Got it!\nDone setting up!", true, "", replyToken);
              currentState = STATE_ALREADY_SETUP;
              alreadyUpdatedsittingBeginTime = false;
              sentFirstTimeLimitNoti = false;
            }
          break;
        }
      }
      else
      {
        send_POST("Please input a non-negative integer!", true, "", replyToken);
      }
    break;
    case STATE_ALREADY_SETUP: 
      if(textBody.equals("setup"))
      {
        if(userPushMessageID != incomingUserID)
        {
          send_POST("Push message service has been taken over by other account.", false, userPushMessageID, "");
        }
        userPushMessageID = incomingUserID;
        currentState = STATE_SETTING;
        currentSetting = SETTING_LIMIT_HOURS;
        send_POST("Setting up sitting time limit...\nHow many hours?", true, "", replyToken);
      }
      else
      {
        String sittingStatus = "Status: ";
        if(reedSwitch_status == 0)
        {
          sittingStatus += "SITTING\nDuration: " + getPrettyDurationString(now() - sittingBeginTime);
        }
        else
        {
          sittingStatus += "NOT SITTING";
        }
        send_POST(sittingStatus, true, "", replyToken);
      }
    break;
    default: ;
  }
}

bool isNumber(String text)
{
  for(int i=0;i<text.length();i++)
  {
    if(!isdigit(text[i]))
    {
      return false;
    }
  }
  return true;
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) 
{
  Serial.println("Connected to NETPIE...");
  microgear.setName(ALIAS);
}

void setup () 
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(reedSwitch, INPUT_PULLUP);
  
  microgear.on(MESSAGE,onMsghandler);
  microgear.on(CONNECTED,onConnected);
  
  //Connect to WIFI
  //wifiManager.resetSettings();
  wifiManager.autoConnect("SitTooLongIOT", "98765432");
  Serial.println("connected to the internet :)");
  timeClient.begin();   // Start the NTP UDP client

  //Time
  setSyncProvider(getNTPTime);
  setSyncInterval(60);
  Serial.println("Syncing time...");
  while (timeStatus() == timeNotSet);

  //Microgear
  microgear.init(KEY,SECRET,ALIAS);
  microgear.connect(APPID);
  
  digitalWrite(LED_BUILTIN, HIGH);
}

void send_POST(String text, bool isReplyType, String userID, String replyToken)
{
   HTTPClient http;    //Declare object of class HTTPClient
   http.begin("http://sit-too-long.herokuapp.com/botpush.php");      //Specify request destination
   http.addHeader("Content-Type", "application/x-www-form-urlencoded");  //Specify content-type header
   int httpCode = 0;
   if(isReplyType)
   {
      httpCode = http.POST("messagingType=reply&replyToken=" + replyToken + "&text=" + text);   //Send the request
   }
   else
   {
      httpCode = http.POST("messagingType=push&userID=" + userID + "&text=" + text);   //Send the request
   }
   String payload = http.getString(); //Get the response payload
   Serial.println(httpCode);   //Print HTTP return code
   Serial.println(payload);    //Print request response payload
   http.end();
}

time_t getNTPTime()
{
  if (WiFi.status() == WL_CONNECTED) //Check WiFi connection status
  {
    // update the NTP client and get the UNIX UTC timestamp 
    timeClient.update();
    
    // convert received time stamp to time_t object
    time_t utc = timeClient.getEpochTime();

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule thICT = {"ICT", First, Sun, Nov, 2, +420};   //UTC + 6 hours - Thailand
    Timezone thTime(thICT, thICT);
    time_t local = thTime.toLocal(utc);
    Serial.println("Time syncing triggered.");
    return local;
  }
  else
  {
    Serial.println("No internet connection.");
    return 0;
  }
}

void loop() 
{
  reedSwitch_status = digitalRead(reedSwitch);
  if(reedSwitch_status == 0) //SITTING
  {
    if(!alreadyUpdatedsittingBeginTime)
    {
      sittingBeginTime = now();
      alreadyUpdatedsittingBeginTime = true;
    }
  }
  else //NOT SITTING
  {
    alreadyUpdatedsittingBeginTime = false;
    sentFirstTimeLimitNoti = false;
  }

  //Push messaging
  if(currentState == STATE_ALREADY_SETUP && reedSwitch_status == 0)
  {
    if(!sentFirstTimeLimitNoti && now() - sittingBeginTime >= (limit_hours * 3600) + (limit_minutes * 60) + limit_seconds)
    {
      send_POST("LIFT YOUR BOTTOM UP!!!\nIt\'s been " + getPrettyDurationString(now() - sittingBeginTime), false, userPushMessageID, "");
      sentFirstTimeLimitNoti = true;
      latestRepeatNotiTime = now();
    }
    else if(sentFirstTimeLimitNoti && now() - latestRepeatNotiTime >= (repeat_hours * 3600) + (repeat_minutes * 60) + repeat_seconds)
    {
      send_POST("LIFT YOUR BOTTOM UP!!!\nIt\'s been " + getPrettyDurationString(now() - sittingBeginTime), false, userPushMessageID, "");
      latestRepeatNotiTime = now();
    }
  }
  
  //Microgear
  if (microgear.connected()) 
  {
      microgear.loop();
      timer = 0;
  }
  else 
  {
    Serial.println("Connection lost, reconnect...");
    if (timer >= 5000) 
    {
        microgear.connect(APPID); 
        timer = 0;
    }
    else timer += 100;
  }
  
  delay(100);
}

String getPrettyDurationString(time_t duration)
{
  String output = "";
  if(duration >= 3600)
  {
    time_t hourText = duration / 3600;
    duration %= 3600;
    output += String(hourText) + " hour";
    if(hourText > 1)
    {
      output += "s";
    }
    output += " ";
  }
  if(duration >= 60)
  {
    time_t minuteText = duration / 60;
    duration %= 60;
    output += String(minuteText) + " minute";
    if(minuteText > 1)
    {
      output += "s";
    }
    output += " ";
  }
  if(duration > 0)
  {
    output += String(duration) + " second";
    if(duration > 1)
    {
      output += "s";
    }
  }
  return output;
}

String getDateString(time_t timeIn)
{
  time_t localTime = timeIn;
  date = "";
  date += days[weekday(localTime)-1];
  date += ", ";
  date += months[month(localTime)-1];
  date += " ";
  date += day(localTime);
  date += ", ";
  date += year(localTime);

  return date;
}

String getTimeString(time_t timeIn)
{
  time_t localTime = timeIn;
  t = "";
  t += hourFormat12(localTime);
  t += ":";
  if(minute(localTime) < 10)  // add a zero if minute is under 10
  {
    t += "0";
  }
  t += minute(localTime);
  t += " ";
  t += ampm[isPM(localTime)];

  return t;
}

//NOT USED
/* 
void sendPushBulletNotification()
{
  //Start or API service using our WiFi Client through PushingBox
  if (client.connect(WEBSITE, 80))
  { 
    Serial.println("connect api successfully");

    //Send noti
    client.print("GET /pushingbox?devid=" + devid1);
    client.println(" HTTP/1.1"); 
    client.print("Host: ");
    client.println(WEBSITE);
    client.println("User-Agent: ESP8266/1.0");
    client.println("Connection: close");
    client.println();
    
    Serial.println("sent message successfully!!!");
  }
  else
  {
    Serial.println("failed to connect api");
  }
}
 */
