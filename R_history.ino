void historyPageSubmit(AsyncWebServerRequest *request) {  
  String tempStr;
  Serial.println("\nhistoryPageSubmit()");
  ESP.wdtFeed();
  clearHistoryLogFile();
  request->send(200,"text/json", "{\"code\":\"1\",\"data\":\"1\"}");
  //request->send(SPIFFS, "/history.html");  
  Serial.println("End of historyPageSubmit()");
}

bool saveToHistoryLogFile(String dnsIP, String Action){
  File historyFile;
  bool openWrite=false;                                                    //   var histories = [
  String tempStr;                                                          //  {serial:1234567,date:'9/10/2017',time:'14:03:25',ip: '8.8.8.8',action: 'reboot'},
  int i,j;                                                                 //  {serial:1234567,date:'9/18/2017',time:'14:04:55',ip: '8.8.4.4',action: 'Ping failure'}
  long fileSize;                                                           //    ]
  Serial.println("Save reboot history to /scripts/history.js...");        
  historyFile = SPIFFS.open("/scripts/history.js", "r");
  fileSize = historyFile.size();
  historyFile.close();
  Serial.print("/scripts/history.js file size in Bytes is: ");
  Serial.println(fileSize);
  ESP.wdtFeed();
  if (fileSize > FILE_SIZE_LIMIT){                                 // 文件 > 200KB,直接用 w 方式打开，删除之前的所有记录                   
    //historyFile = SPIFFS.open("/scripts/history.js", "w");               // 直接删除所有记录，size 太大，不用上传了
    /* 删除 history.js 中的最悠久的历史记录：算法如下：3/18/2018
      *  以只读方式打开 history.js 文件，文件指针向下移动文件记录的50%
      *  （先数一下有多少个逗号，就是有多少个记录，然后文件指针移动到1/2个记录数量的地方）
      *  以W 方式创建一个空白的新文件 newHistory.js
      *  将 history.js 中指针所指向的开始到该文件结尾部分的内容，copy 进入 newHistory.js 中
      *  删除 history.js 文件
      *  将 newHistory.js 重命名为 history.js
      */
    Serial.println("Begin to shrink history.js file size...");
    historyFile = SPIFFS.open("/scripts/history.js", "r"); 
    i=0;
    while(historyFile.available()){         
      if (historyFile.find('}')){
        i = i+1;
        historyFile.seek(1,SeekCur); 
      }
    }     
    Serial.print("\r\nTotal # of records in history.js:");
    Serial.println(i);
    j = int(i/2);
    Serial.print("Half of the slice is:");
    Serial.println(j);
    historyFile.seek(0,SeekSet);    //move to the beginning of file
    File newHistory = SPIFFS.open("/scripts/newHistory.js","w");
    if(newHistory){
      char c; 
      newHistory.print("var histories = [");
      i = 0;
      while(i<j){
        c = char(historyFile.read());                  
        if( c=='}') {
          i=i+1;
        }                          
      }          
      historyFile.seek(1,SeekCur);       
      while(historyFile.available()){ 
        c = char(historyFile.read());       
        newHistory.print(c);               
      }         
      newHistory.close();
    }
    else
      Serial.println("Fail to create newHistory.js");
    historyFile.close();
    if(SPIFFS.remove("/scripts/history.js")){
      if(SPIFFS.rename("/scripts/newHistory.js", "/scripts/history.js")){
        Serial.println();
        Serial.println("rename success");
        checkFile("/scripts/history.js");
      }
    }
    Serial.println("Successfully REDUCED history.js file size.");        
  }
  else{          
    Serial.println("Open history.js as r+");
    historyFile = SPIFFS.open("/scripts/history.js", "r+");             // 200KB 以下，接着往里写log内容          
    if (historyFile.size() > FILE_SIZE_SAFE){                             // 100KB 以上，要上传日志，并且删除之前的所有记录
      UploadHistoryLog = true;                                        // Loop 中 PING Success 时处理
      Serial.println("/scripts/history.js size > FILE_SIZE_SAFE, UploadHistoryLog=True");
    }
  }    
  if(!historyFile){
    Serial.println();
    Serial.println("Failed to open Log file /scripts/history.js");
    return false;
  } 
  else{
    Serial.println("start saving to history log file");
    tempStr = "\n";                                       // 首个记录, 空白文件内容是 var histories = []
    historyFile.seek(0, SeekEnd);                        // 文件指针放于文件尾部
    while(char(historyFile.peek()) != ']' && historyFile.position() > 0 ){        //找到 ]
      historyFile.seek(-1, SeekCur);                                          
    }                                            
    if (timeAcquired ){                
      adjustTime();         
      tempStr = tempStr + "{serial:'" + ESP.getChipId() + "',date:'" +    //date format is Month/Day/Year HH:MM:SS
                            String(currentMonth) +"/";
      if(currentDay<10)     tempStr = tempStr + "0"; 
      tempStr = tempStr +   String(currentDay)+"/"+
                            String(currentYear)+"',time:'"+ 
                            String(currentHour)+":";
      if(currentMinute<10)  tempStr = tempStr + "0";  
      tempStr = tempStr +   String(currentMinute) + ":";
      if(currentSecond<10)  tempStr = tempStr + "0";
      tempStr = tempStr +   String(currentSecond) + "'";          
    }
    else  tempStr = tempStr + "{serial:'"+ ESP.getChipId()+ "',date:'',time:''";        //这是没有成功获取时间的情况下，就不要写 1／1／1970 了
    tempStr = tempStr + ",ip:'" + dnsIP + "',action:'" + Action + "'},]";               // 去掉空格, 注意 the comma 
    Serial.println("New Log is: ");  
    Serial.println(tempStr);     
    historyFile.print(tempStr);                                     //写入文件中                 
    historyFile.close();
    Serial.println("Save Reboot history to /scripts/history.js Success");
  }             
  Serial.println("Save reboot history Ends.");  
}

void clearHistoryLogFile(){
  File historyFile;
  Serial.println("Delete all records in history.js file.");
  historyFile = SPIFFS.open("/scripts/history.js", "w"); 
  if(historyFile){
    historyFile.print("var histories = []");
    historyFile.close();
  }
  else
    Serial.println("FS Error: history.js cannot open for Write.");
  Serial.println("Finished delete all records in history.js file.");  
}



/*  保存reboot记录，限制文件大小（有限保存 reboot 历史）
 *  This method is not good. replaced with 
 *  bool  saveToHistoryLogFile(String dnsIP, String Action);
 */
/*
bool saveRebootHistory(String dnsIP, String Action){                           // var histories = [
      String tempStr;                                                          //  {Date: '9/10/2017', Time:'14:03:25', IP: '8.8.8.8',  Action: 'reboot'},
      int i,j;                                                                 //  {Date: '9/18/2017', Time:'14:04:55', IP: '8.8.4.4',  Action: 'Ping failure'}
      Serial.println("Save reboot history Starts...");                        //    ];
      File historyFile = SPIFFS.open("/scripts/history.js", "r");               //读取内容到 tempStr 中，对 tempStr 添加内容，再写回去
      if(!historyFile){
          Serial.println();
          Serial.println("Failed to open Log file /scripts/history.js to Read: ");
          return false;
      } 
      else{
          while(historyFile.available()){                   //将整个history文件读入
            tempStr += historyFile.readStringUntil('\n');
          }
          historyFile.close();                              // Read方式关闭 
          tempStr.trim();
          Serial.print("Read file content:");
          Serial.println(tempStr);  
          //i = tempStr.lastIndexOf("}");                   //找到数据尾部
          if (tempStr.lastIndexOf("}")==-1)   {             //没找到数据,空文件，往里添加
              i = tempStr.indexOf("[");
              tempStr = tempStr.substring(0,i+1);           // 必须 i+1 才能把 [ 包含进去
          }          
          else  {                                           //往尾部添加数据
              i = tempStr.lastIndexOf("}");
              tempStr = tempStr.substring(0,i+1);           //要到 i+1 才不丢失那个 }
              tempStr = tempStr + ",\n";                    //多了一个 逗号,分隔JSON数据文件格式, 增加 \n, 在上传数据时，按行上传，无需载入整个文件        
          }
          Serial.print("Current year() =");
          Serial.println(year());
          if (year() != '1970' && timeAcquired == true){
                 tempStr = tempStr + "{Date:'" + month() +"/"+day()+"/"+year()+"', Time:'"+hour()+":";
                if(minute()<10) tempStr = tempStr + "0";
                tempStr = tempStr + minute() + ":";
                if(second()<10) tempStr = tempStr + "0";
                tempStr = tempStr + second();           
          }
          else  tempStr = tempStr + "{Date:'',Time:''";                           //这是没有成功获取NTP时间的情况下，就不要写 1／1／1970 了
          tempStr = tempStr + "', IP:'" + dnsIP + "', Action:'" + Action + "'}]"; 
          Serial.println("New Log: ");  
          Serial.println(tempStr);        
      }    
      historyFile = SPIFFS.open("/scripts/history.js", "w");               //以 Write 方式打开文件
      if(!historyFile){
          Serial.println();
          Serial.print("Failed to open Reboot history file to Append: ");
          Serial.println(historyFile);  
          return false;
      } 
      else{
          historyFile.println(tempStr);                                         //写入文件中                 
          historyFile.close();
          Serial.println("Save Reboot history to /scripts/history.js");
      }             
      Serial.println("Save reboot history Ends.");
}
*/
