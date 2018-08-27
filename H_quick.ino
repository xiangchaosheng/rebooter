void quickSetupLoad(AsyncWebServerRequest *request) {
  Serial.println("\nQuick setup page-Load()");  
  ESP.wdtFeed();
  request->send(SPIFFS, "/quick.html");           //必须有这个 ／                    
  Serial.println("End of quickSetup page-Load");
}

void quickSetupSubmit(AsyncWebServerRequest *request) {
  String inputSSID, inputPassword; 
  
  Serial.println("\nQuick setup Submit() ...");
  ESP.wdtFeed();
//Dec. 12/6/2017 Not using the "refresh" button function. Asyn Mode, not predictive for when the SSID.js will refresh  
/*
  if (request->hasArg("refresh")){
      if(request->arg("refresh")=="1"){
          Serial.println("Refresh SSID list and feed to ComboBox by refreshing quick.html page.");
          request->send(200, "text/plain", "{\"code\":\"1\"}");           //1: success; 0:failure
          WiFi.scanNetworksAsync(asyncScanSaveSSID);                     // async scan, after completed, callback function: asyncScanSaveSSID(int);
          quickSetupLoad(request);                                        //refresh current page, BUG: cannot guarantee the SSIDs are new              
      }            
  }
*/           
  if (request->hasArg("ssidCombo")){
    routerSSID = request->arg("ssidCombo");
    Serial.print("SSID entered is : "); 
    Serial.println( routerSSID);   
  }
  else 
    Serial.println("Server does not found - ssidCombo arg.");      
  if (request->hasArg("inputPassword")){
    routerPassword = request->arg("inputPassword");  
    routerPassword.trim();
    Serial.print("Password is: "); 
    Serial.print(routerPassword); 
    Serial.println("[END]");
  }
  else 
    Serial.println("Server does not found - inputPassword arg.");      

  // connect to target Wi-Fi 
  if ((routerSSID.length() > 0) && (routerPassword.length() >= 0)) {      
    // use AsyncWebServer to show connect page
    Serial.println("> > > Display connect page < < <");
    request->redirect("/connect.html");      
  }
  
  
  Serial.println("Leave quickSetupSubmit()....");      
}

