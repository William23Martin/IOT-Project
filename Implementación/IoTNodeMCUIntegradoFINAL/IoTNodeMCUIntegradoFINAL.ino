/**
   Tomado de https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
*/
//Librerías
#include <ESP8266WiFi.h> //Librería de control del NodeMCU y su módulo WiFi-ESP8266.
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <PubSubClient.h> //Librería para que el NodeMCU cumpla las labores y tenga las responsabilidades de Publisher/Subscriber en la arquitectura MQTT.
//-----------------------
//Parámetros y variables de conexión Wifi.
const char* ssid     = "emcali_1221"; //ssid de ingreso a la conexión WiFi.
const char* password = "perro#?;123lucky"; //Password de la conexión WiFi.
WiFiClient espClient; //Objeto del módulo WiFi del NodeMCU.
WiFiClient espClientThingSpeak; //Objeto del módulo WiFi del NodeMCU.
//-----------------------
//Parámetros y variables de conexión al broker.
const char* mqtt_server = "broker.mqttdashboard.com"; //Nombre del componente Broker de la arquitectura MQTT. Asignado por defecto el público de HiveMQ.
PubSubClient client(espClient); //El cliente (subscriber/publisher) se conecta mediante el módulo WiFi del ESP8266.
//-----------------------
//Parámetros de conexión con el servidor de ThingSpeak:
String apiKey = "WE0QGV0PQYR4TCI6";
const char* server = "api.thingspeak.com";
//-----------------------
// Objeto dispositivo Adarfuit para adaptación de entradas análogas
Adafruit_ADS1115 ads(0x48);
//--------------------------
//Tópico de subscripción y mensajes asociados:
const char* stateTopic = "state/";
//Activar la bomba de agua de la Rúgula. Valor de mensaje '1'.
//Activar la bomba de agua del Cilantro. Valor de mensaje '2'.
//Activar la bomba de agua del Tomate. Valor de mensaje '3'.
//Activar las tres bombas de agua del Cultivo. Valor de mensaje '4'.
//Activar el Modo Automático. Valor de mensaje '5'.
//Activar la Luz de la Lámpara. Valor de mensaje '6'.
//----------------------
//Tópicos de publicación:
const char* connectedTopic = "mcu/connected"; //Tópico de activación de la luz de la lámpara.
//----------------------
//Variable de modo automático:
volatile boolean modoAuto = false;
//----------------------
// Variables capturadas para lectura de los sensores de humedad--------
//Voltaje de humedad de la Rúgula
float ArugulaVoltage = 0.0;
//Voltaje de humedad de Cilantro
float CorianderVoltage = 0.0;
//Voltaje de la humedad de Tomate
float TomatoVoltage = 0.0;
//Voltaje de nivel de agua
float WaterLevel = 0.0;
//Voltaje de la intensidad de luz
float LuminicVoltage = 0.0;
//--------------------------------------------------------------------
//Variables del LED--------------------------------------------------------
const int ledRojo = D0;
const int ledVerde = D4;
const int ledAzul = D3;
//--------------------------------------------------------------------
//Variables de  RELAY--------------------------------------------------
const int pinLamp = D5;
#define pulsorLamp 9
const int pinTomato = D6;
const int pinCoriander = D7;
const int pinArugula = D8;
//--------------------------------------------------------------------
//Constantes de rangos de voltaje para prender los relays------------------
const int DRY_SOIL = 2400;
//-------------------------------------------------------------------------
//ENTRADAS ANALOGICAS--------------------------
int16_t adc0, adc1, adc2, adc3, adc4;
//----------------------------------------------
//Variable de estado inicial
volatile int stateOfLampManual = HIGH; //estado inicial
volatile int stateOfTomatoManual = HIGH; //estado inicial
//---------------------------
//VARIABLE DE METODO EVALUARTOPIC--------------------
int tiempoDelay = 2000;

/**
   Método de inicialización de variables.
*/
void setup() {
  Serial.begin(115200);
  delay(10);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(pinLamp, OUTPUT);
  pinMode(pulsorLamp, INPUT_PULLUP);
  attachInterrupt(pulsorLamp, parpadeo, FALLING);
  pinMode(pinTomato, OUTPUT);
  pinMode(pinCoriander, OUTPUT);
  pinMode(pinArugula, OUTPUT);
  //ENTRADAS ANALOGICAS
  ads.begin();
  //----------------
}

/**
   Método que procesa los mensajes que son publicados por otros clientes, mediante
   la subscripción al Broker,
*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje enviado por el topico [");
  Serial.print(topic);
  Serial.print("]");
  Serial.print(", ");
  Serial.println();
  Serial.print("Se recibio un mensaje de:");
  Serial.print((char)payload[0]);
  Serial.println();
  delay(20);
  //Actuar según el tópico y el valor del mensaje que este reciba:
  evaluarTopico(topic, payload);
  delay(20);
}

/**
   Método para la toma de desiciones del sistema frente a los
   mensajes de entrada que recibe. Evalúa todos los tópicos.
*/
void evaluarTopico(char* topic, byte* payload) {
  char mensajeRecibido = (char)payload[0];
  //Si recibe una irrigación para Rúgula.
  int tiempoDelay = 2000;
  if (mensajeRecibido == '1') {
    digitalWrite(pinArugula, HIGH); delay(tiempoDelay); digitalWrite(pinArugula, LOW);
    Serial.println("Irrigating Arugula\n");
  } else if (mensajeRecibido == '2') {
    digitalWrite(pinCoriander, HIGH); delay(tiempoDelay); digitalWrite(pinCoriander, LOW);
    Serial.println("Irrigating Cilantro\n");
  } else if (mensajeRecibido == '3') {
    digitalWrite(pinTomato, HIGH); delay(tiempoDelay); digitalWrite(pinTomato, LOW);
    Serial.println("Irrigating Tomato\n");
  } else if (mensajeRecibido == '4') {
    digitalWrite(pinArugula, HIGH);
    digitalWrite(pinCoriander, HIGH);
    digitalWrite(pinTomato, HIGH);
    delay(tiempoDelay);
    digitalWrite(pinArugula, LOW);
    digitalWrite(pinCoriander, LOW);
    digitalWrite(pinTomato, LOW);
    Serial.println("Irrigating Crops\n");
  } else if (mensajeRecibido == '5') {
    modoAuto = !modoAuto;
    digitalWrite(pinArugula, LOW);
    digitalWrite(pinCoriander, LOW);
    digitalWrite(pinTomato, LOW);
    Serial.println("Switching mode \n");
    Serial.print(modoAuto);
    delay(50);
  } else if (mensajeRecibido == '6') {
    parpadeo();
    Serial.println("Switching Light\n");
    delay(50);
  }
}

/**
   Inicialización de la conexión WiFi. Es necesario un AccessPoint
   que tenga las configuraciones de ssid y password especificadas.
*/
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void updateSensors() {
  adc4 = analogRead(A0);
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);

  ArugulaVoltage = (adc0);
  CorianderVoltage = (adc1);
  TomatoVoltage = (adc2);
  WaterLevel = (adc3) - 3400.00;
  LuminicVoltage = (double)(adc4 * 5) / 1023;

  Serial.print("Humedad de la Rugula: ");
  Serial.println(ArugulaVoltage);
  Serial.print("Humedad del Cilantro: ");
  Serial.println(CorianderVoltage);
  Serial.print("Humedad del Tomate: ");
  Serial.println(TomatoVoltage);
  Serial.print("Nivel de agua en tanque: ");
  Serial.println(WaterLevel);
  Serial.print("Intensidad de luz: ");
  Serial.println(LuminicVoltage);
  Serial.println();
}

void updateLED(float val) {
  //El parametro este entre 0 y 14000.
  digitalWrite(ledRojo, HIGH);
  digitalWrite(ledVerde, HIGH);
  digitalWrite(ledAzul, HIGH);

  if (val > 9000) {
    digitalWrite(ledRojo, LOW);
  } else {
    digitalWrite(ledVerde, LOW);
  }
}

/**
   Este metodo se encarga de gobernar el riego de las plantas.
  ->modoAuto= true, solo se riega cuando los cultivos están secos.
  ->modoAuto= false, no hace nada y cede el control al usuario. No activa la válvula.
*/
void updateRelaysCrop(float tomato , float coriander, float arugula) {
  if (modoAuto) {
    digitalWrite(pinTomato, LOW);
    digitalWrite(pinCoriander, LOW);
    digitalWrite(pinArugula, LOW);
    if (tomato <= DRY_SOIL) {
      digitalWrite(pinTomato, HIGH);
    }
    if (coriander <= DRY_SOIL) {
      digitalWrite(pinCoriander, HIGH);
    }
    if (arugula <= DRY_SOIL) {
      digitalWrite(pinArugula, HIGH);
    }
  }
}

/**
  Este metodo se encarga de prender y apagar la luz, la variable modoAuto determina si
  ->modoAuto= true, prende y apaga el sensor de luz
  ->modoAuto= false, prende y apaga la app web y el pulsor
 **/
void updateRelaysLight(float luz) {
  digitalWrite(pinLamp, LOW);
  //digitalWrite(pulsorLamp,estado);
  if (modoAuto) {
    if (luz <= 2.5) {
      digitalWrite(pinLamp, HIGH);
    } else {
      digitalWrite(pinLamp, LOW);
    }
  }
}

void updateThingSpeak(String tsData) //Tomado de https://github.com/iobridge/ThingSpeak-Arduino-Examples/blob/master/Ethernet/Arduino_to_ThingSpeak.ino
{
  if (espClientThingSpeak.connect(server, 80))
  {
    espClientThingSpeak.print("POST /update HTTP/1.1\n");
    espClientThingSpeak.print("Host: api.thingspeak.com\n");
    espClientThingSpeak.print("Connection: close\n");
    espClientThingSpeak.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    espClientThingSpeak.print("Content-Type: application/x-www-form-urlencoded\n");
    espClientThingSpeak.print("Content-Length: ");
    espClientThingSpeak.print(tsData.length());
    espClientThingSpeak.print("\n\n");
    espClientThingSpeak.print(tsData);
  }

  if (espClientThingSpeak.connected())
  {
    Serial.println("Data sended to ThingSpeak...");
    Serial.println();
  }
  else
  {
    Serial.println("Connection to ThingSpeak failed ");
    Serial.println();
  }

  espClientThingSpeak.stop();
}


/**
   Método para la conexión inicial y la renovación de la conexión en caso de falla.
   clientId-Identificador del subscriber/publisher suministrado por el Broker.
*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "clientId-KTO3aBrCNq";
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      //Publicar:
      client.publish(connectedTopic, "true"); //-->mcu/connected
      Serial.println("I published my conection state of true");
      delay(50);
      //Subscribirse:
      client.subscribe("state/");
      Serial.println("Subscribe to All mesages of state topic");
      delay(50);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //ENVIO DE INFORMACION A THINGSPEAK
      updateThingSpeak("field1=" + String(ArugulaVoltage) + "&field2=" + String(CorianderVoltage) + "&field3=" + String(TomatoVoltage) + "&field4=" + String(WaterLevel) + "&field5=" + String(LuminicVoltage));
      //------------
      delay(5000);
    }
  }
}

void parpadeoTomato() {
  stateOfTomatoManual = !stateOfTomatoManual;
}

void parpadeo() {
  stateOfLampManual = !stateOfLampManual;
  digitalWrite(pinLamp, stateOfLampManual);
}

void loop() {

  updateSensors();
  //SENSOR NIVEL DE AGUA ->actuando sobre ->  LED
  updateLED(WaterLevel);
  //SENSOR HUMEDAD TIERRA ->actuando sobre ->  BOMBAS DE AGUA
  updateRelaysCrop(TomatoVoltage, CorianderVoltage, ArugulaVoltage);
  //SENSOR FOTOCELDA ->actuando sobre ->  LAMPARA
  updateRelaysLight(LuminicVoltage);


  if (!client.connected()) {
    reconnect();
    updateThingSpeak("field1=" + String(ArugulaVoltage) + "&field2=" + String(CorianderVoltage) + "&field3=" + String(TomatoVoltage) + "&field4=" + String(WaterLevel) + "&field5=" + String(LuminicVoltage));
  } else {
    Serial.println("MQTT is still connected");
    //Publicar:
    client.publish(connectedTopic, "true"); //-->mcu/connected
    Serial.println("I published my conection state of true");
    delay(50);
    //ENVIO DE INFORMACION A THINGSPEAK
    updateThingSpeak("field1=" + String(ArugulaVoltage) + "&field2=" + String(CorianderVoltage) + "&field3=" + String(TomatoVoltage) + "&field4=" + String(WaterLevel) + "&field5=" + String(LuminicVoltage));
    //------------
  }
  client.loop();
}

