/////////////////////////////////////////////////////////////
//////////////////////// AUTOPLANT //////////////////////////
//////////////// Proyecto integrador 3 2024 /////////////////
/////////////////////////////////////////////////////////////
///////// Nicolas Nemmer - Juan Pablo Peyroulou /////////////
//////// Prof. Diego Saez - Prof. Nicolas Cremona ///////////
/////////////////////////////////////////////////////////////
///////////////////// Universidad ORT ///////////////////////
/////////////////////////////////////////////////////////////


/*========= BIBLIOTECAS =========*/
#include <Arduino.h>
#include <WiFi.h>          // Biblioteca para generar la conexión a internet a través de WiFi
#include <PubSubClient.h>  // Biblioteca para generar la conexión MQTT con un servidor (Ej.: ThingsBoard)
#include <ArduinoJson.h>   // Biblioteca para manejar Json en Arduino
#include "DHT.h"           // Biblioteca para trabajar con DHT 11 (Sensor de temperatura y humedad)


/*========= CONSTANTES =========*/

// Credenciales de la redWiFi
//const char* ssid     = "HUAWEI-IoT";
//const char* password = "ORTWiFiIoT";

const char* ssid = "iPhone de Juan Pablo";
const char* password = "jp123456";

// Host de ThingsBoard
const char* mqtt_server = "demo.thingsboard.io";
const int mqtt_port = 1883;

// Token del dispositivo en ThingsBoard
const char* token = "DAtq3V0L8Bi26NCp6iuk";

// Tipo de sensor
#define DHTTYPE DHT11  // tipo de DHT
#define DHT_PIN 21     // DHT en PIN D21

/*========= VARIABLES =========*/

// Objetos de conexión
WiFiClient espClient;            // Objeto de conexión WiFi
PubSubClient client(espClient);  // Objeto de conexión MQTT

// Objetos de Sensores o Actuadores
DHT dht(DHT_PIN, DHTTYPE);

// Declaración de variables para los datos a manipular
unsigned long lastMsg = 0;  // Control de tiempo de reporte
int msgPeriod = 5000;       // Actualizar los datos cada 2 segundos
int tempThreshole = 3; 
int humThreshole = 3;
int humedad = 0;
int temperatura = 0;
int tempSetPoint = 30;
int humSetPoint = 40;


int humidity = 0;
int temperature = 0;

// Mensajes y buffers
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
char msg2[MSG_BUFFER_SIZE];

// Objeto Json para recibir mensajes desde el servidor
DynamicJsonDocument incoming_message(256);

/*========= FUNCIONES =========*/

// Inicializar la conexión WiFi
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Conectando a: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("¡Conectado!");
  Serial.print("Dirección IP asignada: ");
  Serial.println(WiFi.localIP());
}


// Función de callback para recepción de mensajes MQTT (Tópicos a los que está suscrita la placa)
// Se llama cada vez que arriba un mensaje entrante (En este ejemplo la placa se suscribirá al tópico: v1/devices/me/rpc/request/+)
/*void callback(char* topic, byte* payload, unsigned int length) {

  // Log en Monitor Serie
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // En el nombre del tópico agrega un identificador del mensaje que queremos extraer para responder solicitudes
  String _topic = String(topic);

  // Detectar de qué tópico viene el "mensaje"
  if (_topic.startsWith("v1/devices/me/rpc/request/")) {  // El servidor "me pide que haga algo" (RPC)
    // Obtener el número de solicitud (request number)
    String _request_id = _topic.substring(26);

    // Leer el objeto JSON (Utilizando ArduinoJson)
    deserializeJson(incoming_message, payload);  // Interpretar el cuerpo del mensaje como Json
    String metodo = incoming_message["method"];  // Obtener del objeto Json, el método RPC solicitado

    // Ejecutar una acción de acuerdo al método solicitado
    //----------------------CHECK STATUS--------------------------------------------
    if (metodo == "checkStatus") {  // Chequear el estado del dispositivo. Se debe responder utilizando el mismo request_number

      char outTopic[128];
      ("v1/devices/me/rpc/response/" + _request_id).toCharArray(outTopic, 128);

      DynamicJsonDocument resp(256);
      resp["status"] = true;
      char buffer[256];
      serializeJson(resp, buffer);
      client.publish(outTopic, buffer);
    }
  }

  if (_topic.startsWith("v1/devices/me/rpc/attributes/")) {  // El servidor "me pide que haga algo" (RPC)
    // Obtener el número de solicitud (request number)
    String _request_id = _topic.substring(26);

    // Leer el objeto JSON (Utilizando ArduinoJson)
    deserializeJson(incoming_message, payload);      // Interpretar el cuerpo del mensaje como Json
    String estado = incoming_message["ledBuiltin"];  // Obtener del objeto Json, el método RPC solicitado


    if (estado) {
      digitalWrite(LED, LOW);  // Encender LED
      Serial.println("Encender LED");
    } else {
      digitalWrite(LED, HIGH);  // Apagar LED
      Serial.println("Apagar LED");
    }

    // Actualizar el atributo relacionado
    DynamicJsonDocument resp(256);
    resp["estado"] = !digitalRead(LED);
    char buffer[256];
    serializeJson(resp, buffer);
    client.publish("v1/devices/me/attributes", buffer);  //Topico para actualizar atributos
    Serial.print("Publish message [attribute]: ");
    //Serial.println(buffer);
  }
}*/

// Establecer y mantener la conexión con el servidor MQTT (En este caso de ThingsBoard)
void reconnect() {
  // Bucle hasta lograr la conexión
  while (!client.connected()) {
    Serial.print("Intentando conectar MQTT...");
    if (client.connect("ESP8266", token, token)) {  //Nombre del Device y Token para conectarse
      Serial.println("¡Conectado!");

      // Una vez conectado, suscribirse al tópico para recibir solicitudes RPC
      client.subscribe("v1/devices/me/rpc/request/+");

    } else {

      Serial.print("Error, rc = ");
      Serial.print(client.state());
      Serial.println("Reintenar en 5 segundos...");
      // Esperar 5 segundos antes de reintentar
      delay(5000);
    }
  }
}

// Función de la tarea 'celdaPeltier'
void celdaPeltier(void* params) {
  bool peltier = false;
  Serial.println("Task 'celdaPeltier' is running");
  while (true) {
    temperatura = dht.readTemperature();  // Leer la temperatura
    humedad = dht.readHumidity();        // Leer la humedad
    if (((temperatura > (tempSetPoint + tempThreshole)) || (humedad > (humSetPoint + humThreshole))) && peltier == false){
      digitalWrite(18, HIGH);
      peltier = true;
      Serial.println("Task 'celdaPeltier' is ON");
    }else if (((temperatura < (tempSetPoint - tempThreshole)) && (humedad < (humSetPoint - humThreshole))) && peltier == true){
      digitalWrite(18, LOW);
      peltier = false;
      Serial.println("Task 'celdaPeltier' is OFF");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Task 'peltier' is running");
    Serial.println(temperatura);
    Serial.println(tempSetPoint);
    Serial.println(' ');
    Serial.println(humedad);
    Serial.println(humSetPoint);
  }
}

// Función de la tarea 'secador'
void secador(void* params) {
   bool secador = false;
  Serial.println("Task 'secador' is running");
  while (true) {
    temperatura = dht.readTemperature();  // Leer la temperatura
    if (temperatura < (tempSetPoint - tempThreshole) && secador == false){
      //digitalWrite(19, HIGH); //prendo secador
      secador = true;
      //Serial.println("Task 'secador' is ON");
      //Serial.println(temperatura);
    }else if (temperatura > (tempSetPoint + tempThreshole) && secador == true){
      //digitalWrite(19, LOW); //apago secador
      secador = false;
      //Serial.println("Task 'secador' is OFF");
      //Serial.println(temperatura);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Task 'secador' is running");
    Serial.println(temperatura);
    Serial.println(tempSetPoint);
    //Serial.println(tempThreshole);
  }
}

// Función de la tarea 'humidificador'
void humidificador(void* params) {
  bool humidificador = false;
  Serial.println("Task 'humidificador' is running");
  while (true) {
    humedad = dht.readHumidity();  // Leer la temperatura
    if (humedad < (humSetPoint - humThreshole) && humidificador == false){
      //digitalWrite(18, HIGH); //prendo humidificador
      humidificador = true;
      //Serial.println("Task 'humidificador' is ON");
      //Serial.println(humedad);
    }else if (humedad > (humSetPoint + humThreshole) && humidificador == true){
      //digitalWrite(18, LOW); //apago humidificador
      humidificador = false;
      //Serial.println("Task 'humidificador' is OFF");
      //Serial.println(humedad);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Task 'humidificador' is running");
    Serial.println(humedad);
    Serial.println(humSetPoint);
    //Serial.println(tempThreshole);
  }
}



/*========= SETUP =========*/

void setup() {
  // Conectividad
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  //digitalWrite(18, HIGH);
  //digitalWrite(19, HIGH);
  Serial.begin(115200);                      // Inicializar conexión Serie para utilizar el Monitor
  setup_wifi();                              // Establecer la conexión WiFi
  client.setServer(mqtt_server, mqtt_port);  // Establecer los datos para la conexión MQTT
  //client.setCallback(callback);              // Establecer la función del callback para la llegada de mensajes en tópicos suscriptos

  // Sensores y actuadores
  pinMode(DHT_PIN, INPUT);  // Inicializar el DHT como entrada
  dht.begin();              // Iniciar el sensor DHT
  //xTaskCreate(secador, "secador", 2048, NULL , 1, NULL); //  xTaskCreate(nombre, descripcion, tamanio en memoria, parametros, nivel de prioridad, id);
  //xTaskCreate(humidificador, "humidificador", 2048, NULL, 1, NULL);
  xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, NULL);
  
}

/*========= BUCLE PRINCIPAL =========*/

void loop() {

  // === Conexión e intercambio de mensajes MQTT ===
  if (!client.connected()) {  // Controlar en cada ciclo la conexión con el servidor
    reconnect();              // Y recuperarla en caso de desconexión
  }

  client.loop();  // Controlar si hay mensajes entrantes o para enviar al servidor

  // === Realizar las tareas asignadas al dispositivo ===

  unsigned long now = millis();
  if (now - lastMsg > msgPeriod) {  // Repetir solo cada 2 segundos
    lastMsg = now;

    temperature = dht.readTemperature();  // Leer la temperatura
    humidity = dht.readHumidity();        // Leer la humedad

    DynamicJsonDocument resp(256);
    resp["temperatura"] = temperature;
    resp["humedad"] = humidity;

    char buffer[256];
    serializeJson(resp, buffer);
    client.publish("v1/devices/me/telemetry", buffer);  // Publica el mensaje de telemetría

    Serial.print("Publicar mensaje [telemetry]: ");
    Serial.println(buffer);
  }
}
