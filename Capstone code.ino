#include <SoftwareSerial.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define RX 2
#define TX 3

#define ONE_WIRE_BUS 4

int Turbidity_Sensor_Pin = A1;
float Turbidity_Sensor_Voltage;
float phValue;
float temperature;
int samples = 600;
float ntu;

#define SensorPin A2 
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

String AP = "Polepalii";       // AP NAME
String PASS = "pvnr@2900"; // AP PASSWORD
String API = "X0SX3NXKXSYERAOD";   // Write API KEY
String HOST = "api.thingspeak.com";
String PORT = "80";
int countTrueCommand;
int countTimeCommand; 
boolean found = false;

#define green_temp 12
#define red_temp 11
#define green_turb 10
#define red_turb 9
#define green_ph 8
#define red_ph 7
  
SoftwareSerial esp8266(RX,TX); 
  
void setup() {
  Serial.begin(9600);
  esp8266.begin(115200);
  sendCommand("AT",5,"OK");
  sendCommand("AT+CWMODE=1",5,"OK");
  sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");
  sensors.begin();  
  pinMode(Turbidity_Sensor_Pin, INPUT);
  pinMode(13,OUTPUT);
}

void loop() {

  digitalWrite(green_temp,LOW);
  digitalWrite(red_temp,LOW);
  digitalWrite(green_turb,LOW);
  digitalWrite(red_turb,LOW);
  digitalWrite(green_ph,LOW);
  digitalWrite(red_ph,LOW);

  phValue= getPhValue();
  temperature= getTemperatureValue();
  ntu= getNtuValue();

  // Printing Temperature Value
  Serial.print("Temperature: ");         // To print Temperature
  Serial.print(temperature);    // Getting the Temperature
  Serial.println("Celsius");
  
  // Printing Turbidity Value
  Serial.print("voltage: ");
  Serial.println(Turbidity_Sensor_Voltage);
  Serial.print("NTU: ");
  Serial.println(ntu);
  
  // Printing pH Value
  Serial.print("pH:");  
  Serial.print(phValue,2);
  Serial.println(" ");  

  if (ntu<500){
    digitalWrite(green_turb,HIGH);
    Serial.println("Turbidity is normal");
  }
  else{
    digitalWrite(red_turb,HIGH);
    Serial.println("Turbidity is high");
  }

  if(temperature>25 && temperature<32){
    digitalWrite(green_temp,HIGH);
    Serial.println("Temperature is optimal");
    }
  else{
    digitalWrite(red_temp,HIGH);
    Serial.println("Temperature is not optimal");
  }

  if(phValue>6.5 && phValue<9.0){
    digitalWrite(green_ph,HIGH);
    Serial.println("Ph is in normal range");
  }
  else{
    digitalWrite(red_ph,HIGH);
    Serial.println("Ph is not in normal range");
  }
  
 String getData = "GET /update?api_key="+ API +"&field1="+String(phValue)+"&field2="+String(temperature)+"&field3="+String(ntu);
 sendCommand("AT+CIPMUX=1",5,"OK");
 sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,15,"OK");
 sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
 esp8266.println(getData);
 delay(1500);
 countTrueCommand++;
 sendCommand("AT+CIPCLOSE=0",5,"OK");
}


float getTemperatureValue(){

  // Code for Temperature Sensor
  sensors.requestTemperatures();  // To request Global Temperature
  temperature = sensors.getTempCByIndex(0); 
  return float(temperature); 
  
}

float getPhValue(){
  
  // Code for pH sensor
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(SensorPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue;
  return float(phValue);
}

float getNtuValue(){
  
  // Code for Turbidity Sensor
  Turbidity_Sensor_Voltage = 0;
    for(int i=0; i<samples; i++)
    {
      Turbidity_Sensor_Voltage += ((float)analogRead(Turbidity_Sensor_Pin)/1023)*5;
    }
    
    Turbidity_Sensor_Voltage = Turbidity_Sensor_Voltage/samples;
    Turbidity_Sensor_Voltage = round_to_dp(Turbidity_Sensor_Voltage,2);
    
    if(Turbidity_Sensor_Voltage < 2.5){
      ntu = 3000;
    }else{
      ntu = -1120.4*square(Turbidity_Sensor_Voltage)+ 5742.3*Turbidity_Sensor_Voltage - 4352.9; 
    }
    return float(ntu);
}

float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  
  found = false;

 }