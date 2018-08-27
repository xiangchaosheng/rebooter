/*获取时间, AsyncWebServer 与 NTP 方式冲突
 * 只能用 time.h 来获取时间
    基于能够成功访问互联网的情况下才可以
*/
void acquireTime() {
  ESP.wdtFeed();
  if(connectedToRouter){
    if (time(&currentTime)){                               
      Serial.print("Current system local time: ");
      Serial.println(ctime(&currentTime));      
      currentTimeStruct = localtime(&currentTime);
      if(currentTimeStruct->tm_year + 1900 >= 2018) {
        timeAcquired = true;
        startTime = millis();        
        currentSecond =0; currentMinute=0; currentHour=0; currentDay=0; currentMonth=0; currentYear=0;              
        /* determine whether this year is Leap Year, ideally this only need to do ONCE */ 
        if(((currentTimeStruct->tm_year + 1900) % 4 == 0) && ((currentTimeStruct->tm_year + 1900) % 100 != 0) || ((currentTimeStruct->tm_year + 1900) % 400 == 0)){
          isLeapYear = true;       
        } 
        else{
          isLeapYear = false;
        } 
        Serial.printf("%d year is Leap year? %s\n", (currentTimeStruct->tm_year+1900), (isLeapYear)?"true":"false");  
      }
      else 
        timeAcquired = false;               
    }
//    else
//      timeAcquired = false;
    ESP.wdtFeed();
    // acquire Timezone and Daylight Saving Setting from http://timezoneapi.io/api/ip     
    if(timeZoneDSTacquired==false){
      if (acquireTimezoneAndDaylightSavingSetting())
        timeZoneDSTacquired = true;
      else
        timeZoneDSTacquired = false;
    }
  }
  else{      
    timeAcquired = false;  
    timeZoneDSTacquired = false; 
  } 
}

void DisplayTimeToSerial(){
  if (timeAcquired){
    adjustTime(); 
    Serial.printf("Adjusted Date and Time M/D/Year H:M:S format: %d/",currentMonth);   //format output
    if(currentDay < 10) 
      Serial.printf("0%d/", currentDay);
    else             
      Serial.printf("%d/",  currentDay);
    Serial.printf("%d  %d:", currentYear, currentHour);
    if (currentMinute < 10) 
      Serial.printf("0%d:", currentMinute);
    else                  
      Serial.printf("%d:",  currentMinute);
    if(currentSecond < 10)  
      Serial.printf("0%d\n",currentSecond);
    else                  
      Serial.printf("%d\n", currentSecond);                  
  }
  // else 
  //   Serial.println("timeAcquired=false. Have no valid time yet.");      
}

void adjustTime(){
  unsigned long secondsElapsed;
  int secondOffset, minuteOffset, hourOffset, dayOffset, monthOffset; 
  
  if (timeAcquired){
    secondsElapsed = (millis() - startTime)/1000;
    //Serial.printf("elapsed seconds: %d\n", secondsElapsed);
    monthOffset = int(secondsElapsed/(30*24*60*60*1000));      // this program will update time every 30 minutes or so, it will never need monthOffset. 
    dayOffset = int(secondsElapsed/86400);
    hourOffset = int((secondsElapsed - dayOffset*86400)/3600);
    minuteOffset = int((secondsElapsed - dayOffset*86400 - hourOffset*3600)/60);
    secondOffset = secondsElapsed - dayOffset*86400 - hourOffset*3600 - minuteOffset*60;     
    //Serial.printf("adjustments: day:%d, hour:%d, minute:%d, second:%d\n", dayOffset, hourOffset, minuteOffset, secondOffset);
    if (currentTimeStruct->tm_sec+secondOffset >= 60){
      currentSecond = currentTimeStruct->tm_sec + secondOffset - 60;
      minuteOffset = minuteOffset+1;
    }
    else 
      currentSecond = currentTimeStruct->tm_sec + secondOffset;

    if(currentTimeStruct->tm_min + minuteOffset >= 60){
      currentMinute = currentTimeStruct->tm_min + minuteOffset - 60;
      hourOffset = hourOffset + 1;
    }
    else 
      currentMinute = currentTimeStruct->tm_min + minuteOffset;

    if(currentTimeStruct->tm_hour + hourOffset >= 24){
      currentHour = currentTimeStruct->tm_hour + hourOffset - 24;
      dayOffset = dayOffset + 1;
    } 
    else 
      currentHour = currentTimeStruct->tm_hour + hourOffset;

    currentDay = currentTimeStruct->tm_mday + dayOffset;           
    currentMonth = currentTimeStruct->tm_mon + 1 + monthOffset; 
    currentYear = currentTimeStruct->tm_year+1900;    
    // complication with Feb. in Leap Year 
    if (isLeapYear && currentMonth==2 && currentDay > 29){
      currentMonth = currentMonth+1;
      currentDay = currentDay - 29;
    }
    if (!isLeapYear && currentMonth ==2 && currentDay > 28){
      currentMonth = currentMonth + 1;
      currentDay = currentDay - 28;
    }
    if(currentMonth==4 ||currentMonth ==6||currentMonth ==9 ||currentMonth ==11){   //30-days a month
      if (currentDay > 30) {
        currentMonth = currentMonth + 1;                    //Month + 1
        currentDay = currentDay - 30;                       //day in new month
      }
    }
    if(currentMonth==1 ||currentMonth==3||currentMonth==5||currentMonth==7||currentMonth==8||currentMonth==10){ // 31-days a month
      if (currentDay > 31) {
        currentMonth = currentMonth + 1;                  //Month + 1
        currentDay = currentDay - 31;                     //day in new month
      }
    }   
    if (currentMonth == 12){
      if(currentDay > 31){
        currentDay = currentDay - 31;
        currentMonth = currentMonth + 1 - 12;
        currentYear = currentTimeStruct->tm_year+1900+1;
      }
      else {
        currentYear = currentTimeStruct->tm_year+1900;
      }
    }    
/*         
    Serial.printf("Adjusted Date and Time M/D/Year H:M:S format: %d/",currentMonth);   //format output
    if(currentDay < 10) 
      Serial.printf("0%d/", currentDay);
    else              
      Serial.printf("%d/",  currentDay);
    Serial.printf("%d  %d:", currentYear, currentHour);
    if (currentMinute < 10) 
      Serial.printf("0%d:", currentMinute);
    else                  
      Serial.printf("%d:",  currentMinute);
    if(currentSecond < 10)  
      Serial.printf("0%d\n",currentSecond);
    else                  
      Serial.printf("%d\n", currentSecond); 
*/                        
  }  
}

/* 
 * 
 */
bool checkTime4Reboot(int currentHour, int currentMinute, int currentSecond, int rebootHour, int rebootMinute){
  int secondsDifference = 0;
  /*
    Serial.println("\r\nCalculating whether it is time to reboot...");
    Serial.printf("Current time: %d:%d:%d \r\n", currentHour, currentMinute, currentSecond);
    Serial.printf("Reboot time: %d:%d \r\n", rebootHour, rebootMinute);
  */    
  if(rebootHour < currentHour){      //e.g. reboot time : 01:05 early morning, but current time is: 10:30 mid-morning. 
    secondsDifference = ((rebootHour+24) * 60 * 60 + rebootMinute*60)-(currentHour * 60 * 60 + currentMinute*60 + currentSecond ) ;      
  }
  else
    secondsDifference = (currentHour * 60 * 60 + currentMinute*60 + currentSecond ) - (rebootHour * 60 * 60 + rebootMinute*60);
  //Serial.printf("Seconds to reboot: %d\r\n", secondsDifference);
  if (secondsDifference > 0 && secondsDifference < 60){               // accuracy: 60 seconds 
    //Serial.println("Time to reboot.");
    return true;
  }
  else{
    //Serial.println("Not rebooting time yet. Keep waiting.");
    return false;
  }
}

/* 如下功能在 4/24/2018 进行了代码优化
 * 因为经常在调用该函数时，出现了stack 的error，设备自动重启。
 * 为了能减少多内存的使用，丢弃用不着的数据
*/
/*
void acquireTimezoneAndDaylightSavingSetting(){                         // from http://timezoneapi.io/api/ip
  String result="", tempStr;
  WiFiClient client; 
  
  Serial.println("Attempt to get Local Time Zone and Daylight Saving Time from Web");
  if(connectedToRouter){
    ESP.wdtFeed();
    if (!client.connect(timezoneHost, httpPort))              
      Serial.println("Cannot acquire timezone or daylight saving setting because WiFi client connection to web Failed");
    else {                
      client.print(String("GET /api/ip") + " HTTP/1.1\r\n" + "Host: " + timezoneHost + "\r\n" + "Connection: close\r\n\r\n");                                 
      Serial.print("Connected to Router. Request sent to: ");
      Serial.println(timezoneHost);
      result ="";
      while (client.connected()) {
        if(client.available())
          result = result + char(client.read());              //如果没有char()转换，出来的都是HEX                  
      }
      Serial.println(result);
      Serial.println("closing connection");
      client.stop();
      if(result.indexOf("200 OK") > 0){
        int i = result.indexOf("offset_gmt");
        int j = result.indexOf("dst");
        tempStr = result.substring(i+13, i+16);
        Serial.print("Offset GMT:");
        Serial.println(tempStr);
        if(tempStr[0] == '+'){                    
          timeZone = (tempStr[1]-'0')*10 + (tempStr[2] - '0');
        }
        if(tempStr[0] == '-'){                    
          timeZone = - ((tempStr[1]-'0')*10 + (tempStr[2] - '0'));
        }      
        Serial.print("Converted timeZone = ");
        Serial.println(timeZone);         
        tempStr = result.substring(j+6, j+11);
        Serial.print("Daylight Saving Time:");
        Serial.println(tempStr); 
        if(tempStr.indexOf("true") > 0)
          daylightSavingTime = 1;
        else
          daylightSavingTime = 0;
        Serial.print("Converted daylightSavingTime = ");
        Serial.println(daylightSavingTime);    
        //Update TimeZone and daylightSavingTime setting 
        Serial.println("Restart time update timer to make timezone and daylight saving time changes");
        configTime(timeZone * 3600, daylightSavingTime, "pool.ntp.org", "time.nist.gov"); 
        os_timer_disarm(&timeUpdateTimer);
        os_timer_setfn(&timeUpdateTimer, (os_timer_func_t*)&acquireTime, 0);
        os_timer_arm(&timeUpdateTimer, timeUpdateInterval, 1 );             
      }
    }
  }
  Serial.println("END of acquireTimezoneAndDaylightSavingSetting()");
}
*/

bool acquireTimezoneAndDaylightSavingSetting(){                         // from http://timezoneapi.io/api/ip
  String result, tempStr, indexStr;
  char c; 
  int i,j,k;
  unsigned long timer; 
  WiFiClient client; 
  
  Serial.println("Attempt to get Local Time Zone and Daylight Saving Time from Web ...");
  if(connectedToRouter){
    ESP.wdtFeed();
    if (!client.connect(timezoneHost, httpPort))              
      Serial.println("Cannot acquire timezone or daylight saving setting because WiFi client connection to web Failed");
    else {                
      client.print(String("GET /api/ip") + " HTTP/1.1\r\n" + "Host: " + timezoneHost + "\r\n" + "Connection: close\r\n\r\n");                                 
      Serial.print("Connected to Router. Request sent to: ");
      Serial.println(timezoneHost);
      result =""; tempStr = ""; indexStr=""; i= 0; j=0; k=0; 
      timer = millis();  
      while (client.connected()) {
        /* Addition on 6/11/2018: if takes longer than 5 seconds to receive data, exit this function 
         */
        if(millis()-timer> 5000){                               
            Serial.println("Time takes too long. Exit Sub.");  
            return false;
        }      
        if(client.available()){
          c = char(client.read());                            //如果没有char()转换，出来的都是HEX
          if(k < 20){                                           //第一行内容是 ，20个字符足够了
            indexStr = indexStr + c;
            k = k+1;
          }
          if (k == 20){                                       //读出20个字符，如果有 200 OK 就是连接成功
            Serial.print("Receive HTTP Header:");            //连接成功：计算{ 的个数，找到目标字符串
            Serial.println(indexStr);
            if (indexStr.indexOf("200 OK") < 0){               // 没有200 OK，出错了
              Serial.println("Connection Failure. Exit acquireTimezoneAndDaylightSavingSetting() ");   
              client.stop();
              return false; 
            }
            k = k + 1;                                       // skip next round when k==20 
          }
          if(c=='{' && i<=5){
            i = i + 1;                                      // i: counter of {
          }
          if(i==5 && c=='"'){
            j = j + 1;                                      // j: counter of "
          }
          if(i==5 && j==19 ){
            result = result + c;            
          }
          if(j==20){
            Serial.print("Extracted date and time =");
            Serial.println(result);            
          }
          if(i==5 && j==131){                                // continue count " 
            tempStr = tempStr + c;
          }
          if(j==132){            
            Serial.print("Extracted daylight saving time (dst) =");
            Serial.println(tempStr);                    
          }
        }                        
      }
      if(result.length() > 0 && tempStr.length() > 0) {
        /* Addition on 6/11/2018:
           extract Date and Time from "2018-06-11T13:10:30+08:00, and assign to localTime
           so, no need to query time.nist.gov
        */
        currentTimeStruct->tm_year = (result[1]-'0')*1000+ (result[2]-'0')*100 + (result[3]-'0')*10 + (result[4]-'0') - 1900;
        currentTimeStruct->tm_mon=(result[6]-'0')*10 + (result[7]-'0') - 1;
        currentTimeStruct->tm_mday=(result[9]-'0')*10 + (result[10]-'0');
        currentTimeStruct->tm_hour=(result[12]-'0')*10 + (result[13]-'0');
        currentTimeStruct->tm_min=(result[15]-'0')*10 + (result[16]-'0');
        currentTimeStruct->tm_sec=(result[18]-'0')*10 + (result[19]-'0');
        currentTime = mktime(currentTimeStruct);
        if (currentTimeStruct->tm_year + 1900 >= 2018){
            timeAcquired = true;
        }
        Serial.print("Extracted data and time and assigned to current local time:");
        Serial.println(ctime(&currentTime));          
        // extract Timezone from "2018-06-11T13:10:30+08:00
        result = result.substring(20, 23);
        Serial.print("Timezone subString:");
        Serial.println(result);
        if(result[0] == '+'){                    
          timeZone = (result[1]-'0')*10 + (result[2] - '0');
        }
        if(result[0] == '-'){                    
          timeZone = - ((result[1]-'0')*10 + (result[2] - '0'));
        }  
        Serial.print("Converted timeZone = ");
        Serial.println(timeZone);
        
        Serial.print("Daylight Saving Time:");
        Serial.println(tempStr); 
        if(tempStr.indexOf("true") > 0)
          daylightSavingTime = 1;
        else
          daylightSavingTime = 0;
        Serial.print("Converted daylightSavingTime = ");
        Serial.println(daylightSavingTime);
      }
      Serial.println("closing connection");
      client.stop();
    
      //Update TimeZone and daylightSavingTime setting 
      Serial.println("Restart time update timer to make timezone and daylight saving time changes");
      configTime(timeZone * 3600, daylightSavingTime * 3600, "pool.ntp.org", "time.nist.gov"); 
      os_timer_disarm(&timeUpdateTimer);
      os_timer_setfn(&timeUpdateTimer, (os_timer_func_t*)&acquireTime, 0);
      os_timer_arm(&timeUpdateTimer, timeUpdateInterval, 1 );  
    }
  }
  Serial.println("END of acquireTimezoneAndDaylightSavingSetting()");
  return true;
}

