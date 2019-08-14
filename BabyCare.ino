#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h>

#include <DHT.h>
#include <DHT_U.h>

#include <SoftwareSerial.h>


#define RX 2    //wifi上行端口
#define TX 3    //wifi下行端口

#define DHTPIN 4    //温湿度传感器管脚数字4
#define DHTTYPE DHT11   //传感器类型
DHT dht(DHTPIN, DHTTYPE);

#define LightDOPin 6   //光敏数字6
#define LightAOPin A2  //光敏模拟A2
float var_light_a=0;   //模拟数据
int var_light_d=0;  //数字数据

//Wifi接入热点及OneNet平台设备
char WIFI_AP[] = "JACKPC";  //无线网账号
char WIFI_PASSWORD[] = "87654312";  //无线网密码
//String device_id = "536315660";  //设备id
//String api_key = "1hK8AC3aucVjXMB2zVchmtaoHd0=";  //鉴权码

char mqttServer[] = "188.131.133.135";

WiFiEspClient espClient;

PubSubClient client(espClient);

SoftwareSerial soft(RX, TX);

int status = WL_IDLE_STATUS;
unsigned long lastSend;


//声音传感器变量
int ListenAPin = A0; //选择模拟0输入端口  
int ListenDPin=5; //选择数字5输入端口 
 
int LEDPin = 13;    //选择LED显示端口  
int LisAValue = 0;    //声音模拟值变量  
boolean  LisLimit;    //声音数字值

//人体热释传感器变量
int PIRPin = A1;  //人体热释传感器管脚

int PIRState = 0;   //人体热释传感器状态  0---未检测到，1----检测到
int val=0;   // 感应数值

//振动传感器
int vibrationPin=7;  //数字7管脚

int present=0;  //当前数值

//交通灯 
int redled =9;   
int yellowled =10; 
int greenled =11; 


SoftwareSerial mySerial(RX, TX); // RX, TX  通过软串口连接esp8266
 
void setup() {

  Serial.begin(9600);

  dht.begin();    //温湿度传感器开启
  
  pinMode(LEDPin,OUTPUT); //设置LED管脚模式
  pinMode(ListenDPin,INPUT); //设置声音数字输入管脚
  pinMode(ListenAPin,INPUT); //设置声音模拟输入管脚
  pinMode(PIRPin,INPUT);  //设置人体热释模拟输入管脚
  pinMode(LightDOPin,INPUT);  //设置光敏数字输入
  pinMode(LightAOPin,INPUT);  //设置光敏模拟输入
  pinMode(vibrationPin,INPUT); //设置振动数字输入
  //交通灯输出
  pinMode(redled, OUTPUT);  
  pinMode(yellowled, OUTPUT); 
  pinMode(greenled, OUTPUT); 

  InitWiFi();                                // 连接WiFi
  client.setServer( mqttServer, 1883 );      // 连接WiFi之后，连接MQTT服务器
  
  lastSend = 0;
 
}

 
void loop() {

    //声音传感器
    LisAValue = analogRead(ListenAPin);    //读声音传感器的值  
    LisLimit=digitalRead(ListenDPin);    //度数字管脚值
    if(!LisLimit) 
        {  
          digitalWrite(LEDPin, HIGH); //大于阈值灯亮
          delay(100);
        }  
    else  
        digitalWrite(LEDPin, LOW);  //小于阈值灯灭  

    Serial.println(LisAValue, DEC);  //打印数字引脚高低电平
    delay(100);


    //人体红外传感器
    val = analogRead(PIRPin);    //读取A0口的电压值并赋值到val
    Serial.println(val);            //串口发送val值
  
    if (val > 150)//判断PIR数值是否大于150，
    {
        Serial.println("有人");
        //digitalWrite(LedPin,HIGH);  //大于表示感应到有人
        PIRState=1;
     }
     else
      {
        //digitalWrite(LedPin,LOW);   //小于表示无感应到有人
        Serial.println("无人");
        PIRState=0;
       }

      Serial.println(PIRState);  
      delay(250);


  //温湿度传感器获取数据      
  String humData = getHumidityData();
  String temData = getTemperatureData();


  //光敏传感器
  var_light_a=analogRead(LightAOPin);
  var_light_d=digitalRead(LightDOPin);
  Serial.println(var_light_a);


  //振动传感器
  present=digitalRead(vibrationPin);
  Serial.println(present);


  //交通灯判决：声音阈值和振动阈值同时达到为红，振动或声音为黄，全无为绿
  //蜂鸣器判决：
  if(LisLimit==0 && present==1){
    digitalWrite(redled, HIGH);
    digitalWrite(yellowled, LOW);
    digitalWrite(greenled, LOW); 
    //song();
    
    }
    else if(LisLimit==1 && present==0){
    digitalWrite(redled, LOW);
    digitalWrite(yellowled, LOW);
    digitalWrite(greenled, HIGH); 
    }else{
    digitalWrite(redled, LOW);
    digitalWrite(yellowled, HIGH);
    digitalWrite(greenled, LOW);   
   }


  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print("[loop()]Attempting to connect to WPA SSID: ");
      Serial.println(WIFI_AP);
      // 连接WiFi热点
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("[loop()]Connected to AP");
  }

  if (!client.connected() ) {
    reconnect();
  }

  // 构建一个 JSON 格式的payload的字符串
  String payload = "{";
  payload += "\"LisLimit\":"; payload += String(LisLimit); payload += ",";
  payload += "\"PIRState\":"; payload += String(PIRState); payload += ",";
  payload += "\"humData\":"; payload += String(humData); payload += ",";
  payload += "\"temData\":"; payload += String(temData); payload += ",";
  payload += "\"var_light_a\":"; payload += String(var_light_a); payload += ",";
  payload += "\"LisAValue\":"; payload += String(LisAValue); payload += ",";
 
  payload += "\"present\":"; payload += String(present);
  payload += "}";
    
//  String value = String(LisLimit)+","+String(PIRState)+","+DTHvalue+","+String(var_light_a)+","+String(LisAValue)+","+String(present);  //上传的数据

  if ( millis() - lastSend > 1000 ) { // 用于定时1秒钟发送一次数据
    char attributes[120];
    payload.toCharArray( attributes, 120 );
    client.publish( "data", attributes );
    lastSend = millis();
  }
  
  client.loop();
}


//温湿度传感器函数
String getTemperatureData(){
  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  return String(t);
}

String getHumidityData(){
  float h = dht.readHumidity();

  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  return String(h);
}

void InitWiFi()
{
  // 初始化软串口，软串口连接ESP模块
  soft.begin(9600);
  // 初始化ESP模块
  WiFi.init(&soft);
  // 检测WiFi模块在不在，宏定义：WL_NO_SHIELD = 255,WL_IDLE_STATUS = 0,
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }

  Serial.println("[InitWiFi]Connecting to AP ...");
  // 尝试连接WiFi网络
  while ( status != WL_CONNECTED) {
    Serial.print("[InitWiFi]Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("[InitWiFi]Connected to AP");
}

/**
 * 
 * MQTT客户端断线重连函数
 * 
 */
void reconnect() {
  // 一直循环直到连接上MQTT服务器
  while (!client.connected()) {
    Serial.print("[reconnect]Connecting to MQTT Server ...");
    // 尝试连接connect是个重载函数 (clientId, username, password)
    if ( client.connect("mqtt", NULL, NULL, "kill", 1, false, "kill") ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ mqtt connect error code = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );    // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}
