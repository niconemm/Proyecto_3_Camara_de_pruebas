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
//#include <WiFi.h>          // Biblioteca para generar la conexión a internet a través de WiFi
//#include <PubSubClient.h>  // Biblioteca para generar la conexión MQTT con un servidor (Ej.: ThingsBoard)
#include <ArduinoJson.h>   // Biblioteca para manejar Json en Arduino
#include "DHT.h"           // Biblioteca para trabajar con DHT 11 (Sensor de temperatura y humedad)


/*========= CONSTANTES =========*/
// Tipo de sensor
#define DHTTYPE DHT11  // tipo de DHT
#define DHT_PIN 21     // DHT en PIN D21

TaskHandle_t taskHandlePeltier;  // Declarar un handle global para la tarea

/*========= VARIABLES =========*/
// Objetos de Sensores o Actuadores
DHT dht(DHT_PIN, DHTTYPE);

// Declaración de variables para los datos a manipular
int tempThreshole = 3; 
int humThreshole = 3;
int humedad = 0;
int temperatura = 0;
int tempSetPoint = 30;
int humSetPoint = 40;
int vibracion = 0;
float alimentacion = 0;
bool sleepProcess = false;

unsigned long lastMsg = 0;
long msgPeriod = 10 * 1000;

/*========= FUNCIONES =========*/

void recibir_serial(){

    String jsonString = Serial.readStringUntil('\n'); // Leer la cadena JSON entrante hasta nueva línea
    StaticJsonDocument<200> receivedDoc;  // Crear un documento JSON
    DeserializationError error = deserializeJson(receivedDoc, jsonString); // Intentar parsear la cadena como JSON

    if (error) {
      Serial.print(F("Error al parsear JSON: "));
      Serial.println(error.f_str());
      return;
    }

    // Extraer valores del JSON
    if(receivedDoc.containsKey("temperatura")){
      temperatura = receivedDoc["temperatura"];
    }
    if(receivedDoc.containsKey("humedad")){
      humedad = receivedDoc["humedad"];
    }
    if(receivedDoc.containsKey("vibracion")){
      vibracion = receivedDoc["vibracion"];
    }
    if(receivedDoc.containsKey("alimentacion")){
      alimentacion = receivedDoc["alimentacion"];
    }
    if(receivedDoc.containsKey("sleepProcess")){
      if(!sleepProcess && receivedDoc["sleepProcess"]){ //Se envio a dormir
        vTaskDelete(taskHandlePeltier);   // Mato las task
      } else if (sleepProcess && !receivedDoc["sleepProcess"]){
        xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, &taskHandlePeltier); //Despierto las task
      }
      sleepProcess = receivedDoc["sleepProcess"];
    }

    Serial.print("OK");
}

void enviar_serial(){
  
  StaticJsonDocument<200> doc;
  doc["temperatura"] = temperatura;
  doc["humedad"] = humedad;
  doc["vibracion"] = vibracion;
  doc["alimentacion"] = alimentacion;
  doc["sleepProcess"] = sleepProcess;
  
  // Convertir el JSON a una cadena y enviarla por el puerto serial
  serializeJson(doc, Serial);
  Serial.println(); // Enviar una nueva línea para marcar el fin del JSON
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
  // Sensores y actuadores
  pinMode(DHT_PIN, INPUT);  // Inicializar el DHT como entrada
  dht.begin();              // Iniciar el sensor DHT

  while(temperatura == 0 && humedad == 0 && vibracion == 0 && alimentacion == 0){
    if (Serial.available() > 0) {
      recibir_serial();
    }
  }
  
  //xTaskCreate(secador, "secador", 2048, NULL , 1, NULL); //  xTaskCreate(nombre, descripcion, tamanio en memoria, parametros, nivel de prioridad, id);
  //xTaskCreate(humidificador, "humidificador", 2048, NULL, 1, NULL);
  xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, &taskHandlePeltier);
  
}

/*========= BUCLE PRINCIPAL =========*/

void loop() {

  // === Realizar las tareas asignadas al dispositivo ===

  if (Serial.available() > 0) {
    recibir_serial();
  }
  unsigned long now = millis();
  if (now - lastMsg > msgPeriod){
    enviar_serial();
    lastMsg = millis();
  }

  
}
