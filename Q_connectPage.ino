void connectPageLoad(AsyncWebServerRequest *request) {
  ESP.wdtFeed();
  Serial.println("\nconnecting page-Load()"); 
  request->send(SPIFFS,"connect.html");          
  Serial.println("End of connect page-Load");
}

void connectPageSubmit(AsyncWebServerRequest *request) {
  Serial.println("\nConnect page Submit()");
  Serial.println("Now attempting to connect to target Wi-Fi...");      
  ESP.wdtFeed();  
  //连接到路由器Wi-Fi
  connectToRouter(routerSSID, routerPassword);
  Serial.println("End of Connect.html page submit");      
}
