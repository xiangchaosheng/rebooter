#include <ESP8266WiFi.h>
#include <WiFiClient.h>              
#include <DNSServer.h>               // DNS Captive Portal 
#include <ESP8266mDNS.h>            //
#include <EEPROM.h>k
#include <FS.h>       
#include <ESP8266Ping.h>
#include <time.h>                    // 这个与AsyncWebServer 不冲突
#include <ESPAsyncTCP.h>             //AsyncWebServer required 
#include <ESPAsyncWebServer.h>       //AsyncWebServer required 
#include <ESP8266httpUpdate.h>       //Firmware 和 SPIFFS 做Web OTA更新用的

#define PowerOnGPIO         13       //电源按钮上电开的检测，GPIO_13
#define ButtonPressGPIO     5       //电源按钮断电关的检测，GPIO_5
#define PowerControlGPIOA   4       //开关路由器电源, GPIO_4, HIGH:切断电源，LOW:接通电源
#define PowerControlGPIOB   16       //开关路由器电源, GPIO_16, HIGH:切断电源，LOW:接通电源
#define BlueLEDpin          14       //ESP是否连接了路由器的Wi-Fi, GPIO_14, HIGH: 灯不亮， LOW：灯亮  
#define RedLEDpin           12       //是否有Ping超时，路由器重启等, GPIO_12, HIGH: 灯不亮， LOW：灯亮 

#define historyFileSizeKB   100      //history.js文件的大小，单位KB
#define TIME_SYNC_INTERVAL  3600     //每3600秒=1小时，与NTP更新一下时间
#define ButtonHoldTime      5000     //开关按钮按下5秒钟内，开关机；在开机状态下，按下超过53秒钟是Reset（恢复出厂设置）
#define FILE_SIZE_SAFE      10000     //单位：byte, (1000～1KB)针对History.js日志文件，如果10KB之内，可以继续往里添加；超过10K，上传文件成功后，删除本文件，重新开始写
#define FILE_SIZE_LIMIT     20000     //单位：byte, （2000～2KB)History.js 日志文件不能超过20KB，一旦超过后，就要删除所有旧的内容，重新开始写入新纪录
#define FALLING_MODE  false
#define RISING_MODE   true
#define Max_Connection_Attempt  3    //Connect to the Router for 3 times, if all fails, delete credentials and restart as AP only mode

extern "C" {
  #include "user_interface.h"
  extern struct rst_info resetInfo;
}

volatile unsigned long ButtonPressTimer;           // 用户按下开关的时间
bool interrupt_trigger_mode = FALLING_MODE;
bool interrupt_trigger_mode_copy;
volatile unsigned int button_read_cnt = 0;
unsigned int button_power_on_cnt = 0;
        
int ErrorNo=0;                                     // Error code
IPAddress apIP (192,168,4,1);
IPAddress arrayDNS[5];                             // Ping要检测的5个DNS的IP地址，第0-4个是出厂设置，用户可以更改设置
int dnsCounter=0;                                  // dnsCounter: Ping到哪一个DNS了
DNSServer dnsServer;                               // DNS Captive Portal 的定义
bool dnsCaptivePortalReady = false;                //  DNS Captive Portal 是否被启动了，在工作状态 = true，没有启动,stop时，是false
const int httpPort = 80;                           // http 都是这个默认端口
AsyncWebServer server(httpPort);                    //Async Web Server
String Version;                                    //软件版本号，根据这个来决定是否需要OTA
String myHostName="zobox"; 
String AP_Prefix = "Zobox-";
String AP_password="";                              //Max 32 chars for password
bool userLoginSuccess = false;                      //判断用户是否成功登录了，如果不加这个判断变量，那么 192.168.4.1/quick 同样可以进入设置
bool connectedToRouter = false;                     //判断是否连接上路由器的Wi-Fi
bool connecting = false;                            // 判断是否正在连接，Is ESP trying to connect to a Router? Yes = true
String accountEmail, accountPassword;               //已经注册了的账户的Email和Password
const int ConnectionTimeOut = 30000;                // ms为单位，连接超时设置 = 30秒
int turnoffAPTime = 600;                            // s 秒为单位，在这个时间后，关掉AP 3/16/2018
bool turnoffAPSet = false;                          // turnOffAPSet: 用户设定是否需要关闭AP；
bool apTurnedOff = false;                           // apTurnedOff：AP 是否关掉了（状态）
String routerSSID, routerPassword;                  // 路由器的SSID，Password
bool pingEnable;                                     // Ping 功能的开关, default
bool pingRestore = true;                            // 记录用户使用定时重启之前ping的状态，1/18/2018
bool presetRebootInProgress = false;                // 用户定时重启Router是否正在进行中
int PingInterval   = 30;                            // seconds, range (0, 60)minutes, value in config.js, default = 0.5 min = 30 seconds
int PowerInterval  = 5;                             // seconds, 切断路由器电源 5 秒钟后再恢复
int RebootInterval = 180;                           // seconds, range (0, 60)minutes, value in config.js, default = 3.0 min = 180 seconds， UI上用户输入是分钟
int connectAttemptCounter = 0;                      // Try to connec to Router 3 times, if all fails, delete credentials and reboot : 6/18/2018
int RestInterval   = 300;                           // seconds, range (0, 60)minutes, value in config.js, default = 15  min = 900 seconds,  路由器3次重启后的间歇时间    5/8/2018检查：还没有放入config.js中！
int PTC, RC;                                        // PTC: Ping Timeout Counter, RC: Reboot Counter; 
int condition =0;                                   // 1: Ping OK; 2:连续单次超时，连续次数<3次; 3: reboot路由器断电后间隔5秒钟再恢复供电
unsigned long pingTimer;                            // 4：路由器上电后2分钟后Wi-Fi才准备好被我们的ESP去连接；5：路由器连续3次重启，暂停15分钟监控
int userDefinedDNSqty=0;                            // 在 /advanced 页面，用户自定义的 DNS 的个数，default=0,用户一个都没有指定
int loopCounter=0;                                  // 删除，调试检测Loop函数在工作
bool UploadHistoryLog = false;                      // True: history.js 文件>FILE_SIZE_SAFE, 但是<FILE_SIZE_LIMIT,需要立即持续尝试上传文件，一旦上传成功，就删除所有记录
bool timeAcquired=false;                            // 判断是否获取到了有效的时间
bool timeZoneDSTacquired=false;                     // 判断是否从 timezoneHost 获取到了时区和夏时制设置
int timeZone = 0;                                   // Timezone default, London=0, China=8
int daylightSavingTime = 0;                         //  =0 不是daylightSavingTime, =1 在daylightSavingTime
char* timezoneHost = "timezoneapi.io";              //https://timezoneapi.io/api/ip
struct tm *currentTimeStruct;
time_t currentTime;
bool isLeapYear = false;                             // Whether current year is a leap year. This is used to handle Feburary adjustment
int currentSecond, currentMinute, currentHour, currentDay, currentMonth, currentYear;     // 2017-12-5 update
bool userSetReboot = true;                         // user set reboot every day at HH:MM
int userSetRebootMinute, userSetRebootHour;         // 24 Hour format, e.g. 18:35 user local time 
unsigned long timeUpdateInterval = 10*60*1000;       // Update time every 10 minutes regardless of update success or fail, set to 60 minutes when release                    
unsigned long startTime;                            //Timer used for acquire Time
unsigned long connStartTime;                        //Timer used for Async mode Router connection
unsigned long updateStartTime;                      //Timer used for Async mode firmware and SPIFFS update
// OTA related global variables
//Save config.js and router.js variables to EEPROM, so after OTA user does not need to reConfig rebooter
bool needOTAcheck = false;                          //
bool userPermitOTA = false;                         //Whether user agrees to conduct OTA
String otaURL = "update.zobox.co";                  //Initial OTA URL of OTA_Server, "dev.met-light.com";
String otaCheckUpdateFile = "/checkUpdate.txt";     // OTA file name on OTA_Server
String newOtaVersion;                               // new OTA firmware or spiffs version
String otaFirmwareName;
String otaFileSysName;
int otaPortNum;
bool firmwareUpdated = false, firmwareNeedUpdate = false; //settings for Web OTA firmware update
bool spiffsUpdated=false, spiffsNeedUpdate=false;         //settings for Web OTA SPIFFS update
String languageChoice;                              // UI Language selected, 6/4/2018

void handleButtonPressInterrupt();    //11-20-2017
void createAPprefix ();
void startSoftAP_DNSCaptivePortal();     //11-29/2017
void scanSaveSSID();
void showSPIFFcontent();
bool loadConfig();                      // 11-20-2017
bool checkFile (const char* FilePath);
void setSingleAPclient();
void startMDNS ();
void connectToRouter(String strSSID, String strPassword);
void saveRouterWiFi(String strSSID, String strPassword,char state);
bool loadRouterWiFi();
void pingCheck();

void clearHistoryLogFile();
void handleReset();
bool saveToHistoryLogFile(String dnsIP, String Action);
void acquireTime();
void adjustTime();                                            //12/5/2017
void DisplayTimeToSerial();
bool otaCheck();

void welcomePageLoad(AsyncWebServerRequest *request);         // 4/16/2018
void welcomePageSubmit(AsyncWebServerRequest *request);       // 4/16/2018
void securityPageLoad (AsyncWebServerRequest *request) ;
void securityPageSubmit(AsyncWebServerRequest *request);
void dashboardPageLoad(AsyncWebServerRequest *request);
void dashboardPageSubmit(AsyncWebServerRequest *request);
void quickSetupLoad(AsyncWebServerRequest *request) ;
void quickSetupSubmit(AsyncWebServerRequest *request);
void advancedPageLoad(AsyncWebServerRequest *request);
void advancedPageSubmit(AsyncWebServerRequest *request);
void helpPageLoad(AsyncWebServerRequest *request) ;
void helpPageSubmit(AsyncWebServerRequest *request);
void rebootPageLoad(AsyncWebServerRequest *request);
void rebootPageSubmit(AsyncWebServerRequest *request);
void connectPageLoad(AsyncWebServerRequest *request);
void connectPageSubmit(AsyncWebServerRequest *request);             // 12/7/2017
void webOTAPageLoad(AsyncWebServerRequest *request);
void webOTAPageSubmit(AsyncWebServerRequest *request);
void asyncScanSaveSSID(int networksFound);                          //12-6-2017
void historyPageSubmit(AsyncWebServerRequest *request);             // 4-17-2018 
bool checkTime4Reboot(int currentHour, int currentMinute, int currentSecond, int rebootHour, int rebootMinute);  
bool acquireTimezoneAndDaylightSavingSetting();                     //  4/27/2018 updated
void blueLEDblink();
void redLEDblink();
static os_timer_t blueLEDtimer;
static os_timer_t redLEDtimer;
static os_timer_t timeUpdateTimer; 

void dashboardLoadConfig(AsyncWebServerRequest *request);         //3/30/2018
void saveVarToConfig (String varName, String varValue);           //4/11/2018
bool saveConfigToEEPROM ();                                       // 5-4-2018
bool restoreConfigFromEEPROM();                                   // 5-4-2018
