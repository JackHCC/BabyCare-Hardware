
#include <DHT.h>
#include <DHT_U.h>

#include <SoftwareSerial.h>
#include "DHT.h"


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
String ssid = "JACKPC";  //无线网账号
String password = "87654312";  //无线网密码
String device_id = "536315660";  //设备id
String api_key = "1hK8AC3aucVjXMB2zVchmtaoHd0=";  //鉴权码


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
 

  
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }

  // 连接WIFI  
  mySerial.begin(9600);
  mySerial.println("AT+RST");   // 初始化重启一次esp8266
  delay(1500);
  echo();
  mySerial.println("AT");
  echo();
  delay(500);
  mySerial.println("AT+CWMODE=3");  // 设置Wi-Fi模式
  echo();
  mySerial.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");  // 连接Wi-Fi
  echo();
  delay(10000);

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
  String DTHvalue=getAndSendTemperatureAndHumidityData(); 


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

   
  

  //WiFi串口
  if (mySerial.available()) {  //软串口，发送到esp8266的都走这个串口
    Serial.write(mySerial.read());
  }
  if (Serial.available()) {  //串口，打印到串口监视器
    mySerial.write(Serial.read());
  }
    
  String value = String(LisLimit)+","+String(PIRState)+","+DTHvalue+","+String(var_light_a)+","+String(LisAValue)+","+String(present);  //上传的数据

  post(value);
  
}


//温湿度传感器函数
String getAndSendTemperatureAndHumidityData(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println();
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");

  return String(h)+","+String(t);
}


//读取应答消息
void echo(){
  delay(50);
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
}

 //上传数据点
void post(String value){
  mySerial.println("AT+CIPMODE=1");
  echo();
  mySerial.println("AT+CIPSTART=\"TCP\",\"api.heclouds.com\",80");  // 连接服务器的80端口
  delay(100);//初始值1000
  echo();
  mySerial.println("AT+CIPSEND"); // 进入TCP透传模式，接下来发送的所有消息都会发送给服务器
  echo();
  delay(500); //初始值5000
  String data ="{\"datastreams\": [{\"id\": \"temperature\",\"datapoints\": [{\"value\": \""+value+"\"}]}]}";
  mySerial.print("POST /devices/"+device_id+"/datapoints HTTP/1.1\r\n"); // 开始发送post请求
  mySerial.print("api-key: "+api_key+"\r\nHost: api.heclouds.com\r\nContent-Length: " + String(data.length()) + "\r\n\r\n"); // post请求的报文格式
  mySerial.println(data); // 结束post请求
  delay(300);//初始值300
  echo();
  delay(100); //初始值100
  mySerial.print("+++"); // 退出tcp透传模式，用println会出错
  Serial.println();
//  delay(2000);  
}
