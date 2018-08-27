/* 处理开关按钮 */
void handleButtonPressInterrupt(){
  detachInterrupt(digitalPinToInterrupt(ButtonPressGPIO)); 
  interrupt_trigger_mode_copy = interrupt_trigger_mode;
  if(interrupt_trigger_mode_copy == FALLING_MODE){
    ESP.wdtFeed();
    for(button_read_cnt = 0; button_read_cnt < 10; button_read_cnt++){
      delay(20);
      if(digitalRead(ButtonPressGPIO) == HIGH){
        attachInterrupt(digitalPinToInterrupt(ButtonPressGPIO), handleButtonPressInterrupt, FALLING); 
        interrupt_trigger_mode = FALLING_MODE;
        button_read_cnt = 0;
        break;
      }
    }
    if(button_read_cnt != 0){
      ButtonPressTimer = millis();      //开始计时
      attachInterrupt(digitalPinToInterrupt(ButtonPressGPIO), handleButtonPressInterrupt, RISING);
      interrupt_trigger_mode = RISING_MODE;  
    }
  }
  else{
    ESP.wdtFeed();
    for(button_read_cnt = 0; button_read_cnt < 10; button_read_cnt++){
      delay(20);
      if(digitalRead(ButtonPressGPIO) == LOW){
        attachInterrupt(digitalPinToInterrupt(ButtonPressGPIO), handleButtonPressInterrupt, RISING); 
        interrupt_trigger_mode = RISING_MODE;
        button_read_cnt = 0;
        break;
      }
    }
    if(button_read_cnt != 0){
       if (millis() < (ButtonPressTimer + ButtonHoldTime)){        // 关机
        Serial.println("Button Press detected. Turn OFF device power.");
        digitalWrite(PowerOnGPIO,LOW);
        //按键抬起瞬间又按下（8266未来得及断电）认为是重新开机
        ESP.wdtFeed();
        delay(200);
        ESP.restart();
      }
      else{                                                      // reset 恢复出厂设置
        Serial.println("Button pressed > 3 seconds. Reset to factory default activated.");
        handleReset();        //直接调用reset函数
      }        
    }
  } 
}

