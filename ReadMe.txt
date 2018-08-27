6/21/2018:
针对David提出的要求：路由器没有Wi-Fi信号时，只要是在监控状态，就要重启路由器。
改进程序：pingCheck() 函数中优化
把	 if(ip[0]!=0）{
更改为： if(ip[0]!=0 || (routerSSID.length()>0) )
并将下文的 Serial.println("Connected to Router Wi-Fi");
更改为：
    if(ip[0]!=0)  
        Serial.println("Connected to Router Wi-Fi");
    if(ip[0] == 0 && (routerSSID.length()>0))
        Serial.println("Not connected to router Wi-Fi. But router SSID exists. Follow the same ping failure routine."); 

6/20/2018:
David的把手机热点给Rebooter共享来测试的方法是不对的：
关闭手机热点模拟的是路由器断电，没有Wi-fi信号了；
常见的断网是Internet没信号，但是路由器的热点Wi-Fi还是存在的。
David希望当路由器没有Wi-Fi信号放出来时，也要能重启路由器.

6/19/2018:
David 提出一个很好的问题：
如果Rebooter连接Router不成功，要努力继续尝试。
场景：
用户出门在外好几天，中间某时如果Rebooter发现Internet不行了，那么要努力持续尝试，
否则用户家的智能设备一直都无法工作了。
目前程序就是这样的，Ping失败后就重启路由器，重启3次后休息15分钟。

另外发现的问题：
断电重启路由器后2分钟，重新连接上Router的是ESP.restart()!
这个Bug导致 reboot counter每次重启后，都 = 0，无法计数！
也就无法实现 3 次 断电重启后休息15分钟的效果。

解决方法：
升级ESP8266 Arduino函数库到 2.4.1 版本，就实现了不用 ESP.restart() 也可以连接到Router.

JP feedback: 
Router重新启动需要 3 分钟： 更改了 config.js 
Factory Reset按键需要长按 5 秒钟: 更改了 
	#define ButtonHoldTime      5000

6/18/2018:
有时候一次性connectToRouter()运行可能失败，增加了2个变量：
#define Max_Connection_Attempt  3
int connectAttemptCounter=0;
对 connectToRouter() 进行计数，可连续运行3次来尝试连接。

6/11/2018:
没有用到的变量：
int Timeout4AcquireTime = 1000; 
在L_Time.ino 的bool acquireTimezoneAndDaylightSavingSetting() 中，
增加了利用从timezoneapi.io的返回值分离出本地时间的方法，
无需持续从 time.nist.gov 获取时间了

6/10/2018:
检测覃俊贴片回来的 Rebooter
硬件问题：开机直接导通 或者 无法关机： 2个三极管坏了

6/7/2018:
清理代码：A_Rebooter.ino 里删除如下内容，因为根本没有用到
void loginEmailLoad(AsyncWebServerRequest *request);              //3/30/2018
bool saveRebootHistory(String dnsIP, String Action);
bool saveToHistoryLogFile(String dnsIP, String Action);	出现了2次，重复了
bool readRebootHistory();

6/6/2018:
OTA 更新：
	在setup() 中增加对EEPROM地址0的检验：
	如果是'1'，那么说明刚经过OTA，config.js 和 router.js 需要从EEPROm中恢复出来；
	否则：无需从EEPROM中恢复 config.js 或 router.js 

	O_OTA.ino 中 的saveConfigToEEPROM ()函数：
	增加了 EEPROM 地址 0 对OTA标志位的书写：
	address = 0;
	EEPROM.write(address, '1'); 

	在 restoreConfigFromEEPROM() 中：
	增加了 EEPROM 地址 0 对OTA标志位的清理：EEPROM.write(0,0);

Fixed a bug in /dashboard: turnoffAPTime 更改后，保存到 config.js 中出错


6/5/2018:
dashboard.html 更新JS：
	如果没有连接上Router，那么：
		不显示 routerSSID/localIP
		将Monitor My Internet 开关Disable
	如果连接上Router，那么：
		显示 routerSSID/localIP
		将Monitor My Internet 开关Enable

6/4/2018:
想到一个方法可以多语言UI：
在Data目录下增加 Spanish， German，French 子目录
将Language设置增加到 config.js 文件末尾(只能添加到末尾，程序改动最小)
在 advanced.html 中增加 Language 的下拉选择框
用户选择了 语言 后，需要ESP.restart() 才能生效
生效的方法：
在 setup() 中，读取 config.js 的Language设置；
根据这个Language设置，把 如下语句中的 setDefaultFile 更改为相应语言的即可
server.serveStatic("/", SPIFFS, "/", "max-age=86400").setDefaultFile("welcome.html"); 
这样在每个.html文件里的URL跳转链接接上，都更改才行。


6/3/2018:
dashboard.html 更改JS：
	只要routerSSID & localIP 不是Null，就显示
	不再依赖于 Monitor My Internet 开关状态
其他页面 confirm 按钮字体大小 = cancel 按钮字体大小

5/31/2018:
pingEnable 是在 router.js 里的 "status":"1" 来控制的
把 router.js 里的 "status":"1" 作为默认值

5/26/2018:
陈红林更新了Button的防抖动处理

5/23/2018:
收到David退回的Rebooter样品
检查结果：陈红林的Button Press程序错误
检测抖动过程中，产生了关机

5/16/2018:
检查了LED闪烁的设置：
Red Blink：上电没有连接上路由器、Ping出错、
Red：	   Reboot
Blue Blink：连接上路由器，没有Ping
Blue:	    连接上路由器，Ping 没有出错

测试SPIFFS的OTA: OK :)
http://dev.met-light.com/A_Global_Async_v3_FS.bin 
大约需要 12 分钟 来下载更新

5/15/2018:
将OTA代码移动到 loop() 中执行,并完善了OTA代码
更新了 /dashboard.html 中的JS
更新了 /advanced.html  中的JS
测试Firmware的OTA更新：OK
http://dev.met-light.com/A_Global_Async_v3.bin 

5/14/2018:
合并优化按键部分代码
升级Arduino中ESP8266支持到 2.4.1
这样就支持WiFiClient的URL为 String 变量类型了
否则只能支持 char *
修复bug
将版本号修改为 v3

5/11/2018:
OTA 可以运行了：
http://dev.met-light.com/A_Global_Async_OTA.bin 
http://dev.met-light.com/A_Global_Async_spiffs.bin 
修改了 OTA.ino 
把OTA的代码移动到了 loop() 里面，就可以下载了！
如果放在WebOTA_submit() 函数里总是出现Error：
HTTP_UPDATE_FAILD Error (-1): HTTP error: connection refused

带来的新问题：
String Version = "1.1";  已经能从 config.js 中读出来，但是，
需要Server提供一个新的Version号。
在otaCheck()中实现这个功能，在Server上增加一个version.txt文件，
包含需要更新的信息。

5/8/2018:
修正Bug：
关闭Ping时，刷新页面，仍旧是关闭状态；
关闭Ping时，如果Reboot（重启Rebooter），不会直接进入Ping的状态；
没有搞明白的问题：
David的设备重启后，就是无法连接上之前的路由器！
Solution: 在 loop.ini 中，用 138-141行代码替换了 143-160 行代码

5/2/2018:
修正了 Security.ino中的 JSON 字符串中的 \n 为 \\n，后来直接去掉了。
这样就可以跳转了。

修正了Security.ino中的JSON，增加request->send(200, "text/JSON", "...code=2...");
的情况，以区分：
	在connectedToRouter == false 应该跳转到 quick.html
	如果已经连接上路由器，connectedToRouter == true，跳转回 dashboard.html 

接纳陈红林修改的 Time.ino 中关于无效时间的判定：
在 acquireTime() 函数中增加：
if(currentTimeStruct->tm_year + 1900 >= 2018) {...}

4/27/2018:
根据David的建议，修改了UI中HTML页面的局部内容
security.html 页面：增加了密码长度 8 -- 32 之间的错误提示；
如果<8个字符，WiFi.softAP()这个函数无法发挥功能，看不到AP！

4/24/2018:
优化了 Time.ino文件中 acquireTimezoneAndDaylightSavingSetting();
之前: 保留了所有Server返回的字符串，长度超过 2KB （难怪会逼迫系统重启）！
更新：只截取并保留 date_time 和 dst 部分，不到 100 个字符！
新的文件名：A_Global_Async_Lite_v2.ino 

4/22/2018:
Ethan 美化了各个页面

4/20/2018:
增加Zobox-ABCD的连接密码

4/19/2018:
经过痛苦的摸索，<input type="time"> 是无法去掉 AM/PM的，
更改为使用 comboBox 固定选项的下拉选择框，体积最小
也放弃了使用单个timePicker.js / .css 的方式
涉及的变化有：
config.js 
loadConfig()
saveVarToConfig()	:应该不用改
reset()
/dashboard
/advanced
同样需要处理的是：
RebootInterval
PingInterval

4/16/2018:
改写HTML页面，做成Lite版

4/10/2018:
为了解决 /dashboard.html 载入过慢的问题：
当清理了Safari的缓存后，CSS都从缓存清理了，需要重新载入所有内容
此时，会非常缓慢
解决方法：用一个简单的 /db2.html 来取代原来的 /dashboard.html 
即：重新改写所有的HTML页面

3/27/2018:
解决 /dashboard.html 载入过慢的问题：
减少与 .js 配置文件的交互。
把 history.js 放到一个单独的页面进行。

login.html 页面的安全隐患：完全暴露了用户名和密码。
改进：
用 GET方法通过 request->send 进行了数据交换

3/9/2018:
关于从网页获取时区/夏时制的设置：
1）在未获取时区/DST之前，显示GMT时间；
2）如果获取时区/DST，然后再立即开始更新时间；
      并却掉时间尾部的GMT显示

关于/dashboard 页面载入过慢的问题：
估计是 .js 文件交互太多？

2/26/2018:
dashboard.html 改进了，能够从 config.js 中读取定时重启的设置了

2/20/2018：
bug: 
	http://zoboot.local/在Android手机上不显示
	并且，在 firefox 浏览器里也有内容不显示
	Android device does not support : 
		hugo's Android phone
		Google Chrome on David's phone
	原因：mDNS 在Android下不工作
解决方案：Java？

2/19/2018:
修复Bug：
1）出现ping 了5个IP地址后，如果第5个IP地址Ping失败，
      那么就去 Ping 192.168.4.1 的错误;
2)  一旦Ping 失败，Ping下一个IP地址又成功后，
     Ping时间间隔无效了 的错误；
3）在 advanced.html 页面修改ping时间间隔后，无法立即生效的错误
     这是因为需要重新载入配置信息;
4) RESET 的时候，把各 js 配置文件，例如config.js，都清理了一遍
    把IP1～IP5的重新恢复为提前预设的IP地址

2/16/2018:
检查到需要修复的地方：
1）Dashboard页面载入时，没有从 config.js 中读入用户关于定时开关的设置：	
	"userSetReboot":"0",
	"userSetRebootTime":"00:00",
修复了：
路由器连接3次失败，重启后，无法自动再次连接上路由器；
定时重启路由器后，无法自动再次连接上路由器。
这两个问题都是在 wifi.disconnect(); 后，需要进行如下设置： 
	ConnectedToRouter = false;
	connecting = false; 

2/8/2018:
对于新增加的功能还没有实施：
如果PING的时间太长，就自动重启路由器

1/31/2018:
/dashboad 页面关于定时reboot的时间和开关保存到了config.js中
为了提高页面在iPhone上的载入速度，修改了 setup()中的代码，
去掉了server.on("/", HTTP_GET, loginPageLoad); 等所有关于 HTTP_GET的；
这些代码仍旧会被执行，只是被server.serveStatic("/", SPIFFS, "/").setDefaultFile("login.html"); 取代了。
在各个html页面，增加各个链接为 .html 结尾的完整文件名。

还没有尝试 AsyncPing 

1/30/2018:
Ethan 修改了 dashboard & advanced.html, 
添加了 JS 代码，于是就不需要加载 main.mini.js 了
所以，加载 dashboard & advanced 页面的速度就快了 

增加了重启路由器时的count-down timer页面！

1/29/2018:
Ethan 修改了advanced 和 dashboard 的HTML，
把缺少的UI交互数据都补上了
我增加了 dashboard.html 的第172行， 第190行的0/1互换了


1/21/2018:
华仔已经按照iOS的界面设计更新了 Dashboard 和 Advanced 页面

1/19/2018:
Font: verdana
zoboot logo image needs to be updated


1/12/2018:
Davie Lee feedback: 增加定时重启功能(每天凌晨几点就重新启动一次)
在 Advanced Setup 页面中添加这个功能
1) 用户没有开启Ping的功能，只是在固定时间重启路由器；
2) 用户开启了Ping的功能，并且在固定时间重启路由器
    只有在condition=
修改Advanced Setup页面；
增加config.js文件中关于定时开关的设置：
这个功能的代码还没有开始写。

12/13/2017:
移动了mDNS启动的顺序，更改到启动WiFi.begin()之前

12/12/2017:
华仔更新了 html 文件：
/advanced	/webOTA		/quick	/connect
修改了日期中只有1位(<10)时在这个位数前补0；
Reboot后恢复mDNS服务；
/advanced页面可以更新Email地址;
/dashboard修复了现实Log;
demo给David看了

12/10/2017:
这个Async的版本，增加了 
	/connect.html
	/reboot.html
	/webOTA.html
并且都连接到了 /quick 或者 /advanced 页面上

12/7/2017:
这个版本里，/reset 产生了变化：(相对于12/5/2017的备份版本)
原来的 /reset_bkup 增加了Confirm 和 Cancel 两个按钮，形成了 /reset.
进展：
/dashboard 按钮的逻辑和/reset中新增按钮的逻辑，都还没有更改
这么做的目的是：
当用户在 /dashboard 中点击 Reboot 按钮时，会转到 /reset 页面。
之前的设置总是转不到 /reset 页面，因为 ESP 还没有来得及传输完 /reset 页面，
就 ESP.restart() 了。如果加载完 /reset 后，已经是在 /reset 的 pageLoad 中，此时
是HTTP GET方法，需要在 /reset 的 pageSubmit 中用HTTP POST方法传递参数给
ESP来重启。于是，只能在 /reset 的下面增加确认按钮来做POST方式确认重启。

 /quick 页面输入密码后，无法载入 /connect 页面：
这个问题同上面的/reset，需要更改 /quick，并在/connect增加 confirm, cancel 按钮

/dashboard 页面需要增加 check for update 按钮，如果有更新，需要
显示 /webOTA 页面：
方法同/reset的处理，本版本还没有做任何改动。


