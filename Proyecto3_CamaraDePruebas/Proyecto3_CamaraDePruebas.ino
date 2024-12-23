/////////////////////////////////////////////////////////////
//////////////////////// ///////// //////////////////////////
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
#include <ArduinoJson.h>  // Biblioteca para manejar Json en Arduino
#include "DHT.h"          // Biblioteca para trabajar con DHT 11 (Sensor de temperatura y humedad)


/*========= CONSTANTES =========*/
// Tipo de sensor
#define DHTTYPE DHT11  // tipo de DHT
#define DHT_PIN 21     // DHT en PIN D21

#define pinFuente 33
#define trans4 23  //Amarillo
#define trans3 22  //Naranja
#define trans2 19  //violeta
#define trans1 18  //azul

#define pinHumi 25
#define pinSolenoide 35
#define pinSecador 26
#define pinPeltier 32

TaskHandle_t taskHandleSecador;  // Declarar un handle global para la tarea
TaskHandle_t taskHandleHumidificador;  // Declarar un handle global para la tarea
TaskHandle_t taskHandlePeltier;  // Declarar un handle global para la tarea
TaskHandle_t taskHandleFuente;  // Declarar un handle global para la tarea
TaskHandle_t taskHandleVibrador;  // Declarar un handle global para la tarea

/*========= VARIABLES =========*/
// Objetos de Sensores o Actuadores
DHT dht(DHT_PIN, DHTTYPE);

// Declaración de variables para los datos a manipular
int tempThreshole = 3;
int humThreshole = 3;
int humedad = 0;
int temperatura = 0;
int tempSetPoint = 0;
int humSetPoint = 0;
int vibracion = 0;
float alimentacion = 0;
bool sleepProcess = false;
int mod_fuente = 0;

// Definimos los parámetros del PWM
const int canalPWM = 0;           // Canal 0
const int frecuenciaPWM = 0;      // Frecuencia inicial de 0Hz
const int resolucionPWM = 8;      // Resolución de 8 bits
const int pinPWM = pinSolenoide;  // Pin de salida del PWM (puedes cambiarlo)

unsigned long lastMsg = 0;
long msgPeriod = 10 * 1000;

unsigned long lastRead = 0;
long readPeriod = 1 * 1000;

/*========= FUNCIONES =========*/

void recibir_serial() {

  String jsonString = Serial.readStringUntil('\n');                       // Leer la cadena JSON entrante hasta nueva línea
  StaticJsonDocument<200> receivedDoc;                                    // Crear un documento JSON
  DeserializationError error = deserializeJson(receivedDoc, jsonString);  // Intentar parsear la cadena como JSON

  if (error) {
    Serial.print(F("Error al parsear JSON: "));
    Serial.println(error.f_str());
    return;
  }

  // Extraer valores del JSON
  if (receivedDoc.containsKey("temperatura")) {
    tempSetPoint = receivedDoc["temperatura"];
  }
  if (receivedDoc.containsKey("humedad")) {
    humSetPoint = receivedDoc["humedad"];
  }
  if (receivedDoc.containsKey("vibracion")) {
    vibracion = receivedDoc["vibracion"];
  }
  if (receivedDoc.containsKey("alimentacion")) {
    mod_fuente = receivedDoc["alimentacion"];
  }
  if (receivedDoc.containsKey("sleepProcess")) {
    if (!sleepProcess && receivedDoc["sleepProcess"]) {  //Se envio a dormir
      vTaskDelete(taskHandleSecador);                    // Mato las task
      vTaskDelete(taskHandleHumidificador); 
      vTaskDelete(taskHandlePeltier); 
      vTaskDelete(taskHandleFuente); 
      vTaskDelete(taskHandleVibrador); 
      digitalWrite(pinSecador, HIGH);
      digitalWrite(pinPeltier, HIGH);
      digitalWrite(pinHumi, HIGH);
      digitalWrite(pinFuente, HIGH);
      ledcWriteTone(canalPWM, 0);
    } else if (sleepProcess && !receivedDoc["sleepProcess"]) { //Despierto las task
      xTaskCreate(secador, "secador", 2048, NULL, 1, &taskHandleSecador);  
      xTaskCreate(humidificador, "humidificador", 2048, NULL, 1, &taskHandleHumidificador);
      xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, &taskHandlePeltier);
      xTaskCreate(fuente, "fuente", 2048, NULL, 1, &taskHandleFuente);
      xTaskCreate(vibrador, "vibrador", 2048, NULL, 1, &taskHandleVibrador);
    }
    sleepProcess = receivedDoc["sleepProcess"];
  }

  //Serial.print("OK");
}

void enviar_serial() {

  StaticJsonDocument<200> doc;
  doc["temperatura"] = temperatura;
  doc["humedad"] = humedad;
  doc["vibracion"] = vibracion;
  doc["alimentacion"] = mod_fuente;
  doc["sleepProcess"] = sleepProcess;

  // Convertir el JSON a una cadena y enviarla por el puerto serial
  serializeJson(doc, Serial);
  Serial.println();  // Enviar una nueva línea para marcar el fin del JSON
}


// Función de la tarea 'celdaPeltier'
void celdaPeltier(void* params) {
  bool peltier = false;
  Serial.println("Task 'celdaPeltier' is running");
  while (true) {
    //temperatura = dht.readTemperature();  // Leer la temperatura
    //humedad = dht.readHumidity();        // Leer la humedad
    if (((temperatura > (tempSetPoint + tempThreshole)) || (humedad > (humSetPoint + humThreshole))) && peltier == false) {
      digitalWrite(pinPeltier, LOW);
      peltier = true;
      Serial.println("Task 'celdaPeltier' is ON");
    } else if (((temperatura < (tempSetPoint)) && (humedad < (humSetPoint))) && peltier == true) {
      digitalWrite(pinPeltier, HIGH);
      peltier = false;
      Serial.println("Task 'celdaPeltier' is OFF");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Peltier: " + String(peltier));
    //Serial.println("Task 'peltier' is running");
    //Serial.println(temperatura);
    //Serial.println(tempSetPoint);
    //Serial.println(' ');
    //Serial.println(humedad);
    //Serial.println(humSetPoint);
  }
}

// Función de la tarea 'secador'
void secador(void* params) {
  bool secador = false;
  Serial.println("Task 'secador' is running");
  while (true) {
    //temperatura = dht.readTemperature();  // Leer la temperatura
    if (temperatura < (tempSetPoint - tempThreshole) && secador == false) {
      digitalWrite(pinSecador, LOW);  //prendo secador
      secador = true;
      //Serial.println("Task 'secador' is ON");
      //Serial.println(temperatura);
    } else if (temperatura > (tempSetPoint) && secador == true) {
      digitalWrite(pinSecador, HIGH);  //apago secador
      secador = false;
      //Serial.println("Task 'secador' is OFF");
      //Serial.println(temperatura);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Secador: " + String(secador));
    //Serial.println("Task 'secador' is running");
    //Serial.println(temperatura);
    //Serial.println(tempSetPoint);
    //Serial.println(tempThreshole);
  }
}

// Función de la tarea 'humidificador'
void humidificador(void* params) {
  bool humidificador = false;
  Serial.println("Task 'humidificador' is running");
  while (true) {
    //humedad = dht.readHumidity();  // Leer la temperatura
    if (humedad < (humSetPoint - humThreshole) && humidificador == false) {
      digitalWrite(pinHumi, LOW);  //prendo humidificador
      delay(100);
      digitalWrite(pinHumi, HIGH);  //prendo humidificador
      delay(200);
      digitalWrite(pinHumi, LOW);  //prendo humidificador
      delay(100);
      digitalWrite(pinHumi, HIGH);  //prendo humidificador

      humidificador = true;
      //Serial.println("Task 'humidificador' is ON");
      //Serial.println(humedad);
    } else if (humedad > (humSetPoint) && humidificador == true) {
      digitalWrite(pinHumi, LOW);  //apago humidificador
      delay(100);
      digitalWrite(pinHumi, HIGH);
      humidificador = false;
      //Serial.println("Task 'humidificador' is OFF");
      //Serial.println(humedad);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("humidificador: " + String(humidificador));
    //Serial.println("Task 'humidificador' is running");
    //Serial.println(humedad);
    //Serial.println(humSetPoint);
  }
}

// Función de la tarea 'fuente'
void fuente(void* params) {
  Serial.println("Task 'fuente' is running");
  while (true) {
    switch (mod_fuente) {

      case 0:
        digitalWrite(pinFuente, HIGH); //apaga fuente
        //Serial.println("MODO 0");
        break;
      
      case 1:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 1);
        digitalWrite(trans2, 1);
        digitalWrite(trans3, 1);
        digitalWrite(trans4, 1);
        //Serial.println("MODO 1");
        break;

      case 2:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 0);
        digitalWrite(trans2, 0);
        digitalWrite(trans3, 0);
        digitalWrite(trans4, 1);
        //Serial.println("MODO 2");
        break;

      case 3:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 1);
        digitalWrite(trans2, 1);
        digitalWrite(trans3, 1);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 3");
        break;
      case 4:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 0);
        digitalWrite(trans2, 1);
        digitalWrite(trans3, 1);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 4");
        break;
      case 5:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 1);
        digitalWrite(trans2, 0);
        digitalWrite(trans3, 1);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 5");
        break;
      case 6:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 0);
        digitalWrite(trans2, 0);
        digitalWrite(trans3, 1);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 6");
        break;
      case 7:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 1);
        digitalWrite(trans2, 1);
        digitalWrite(trans3, 0);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 7");
        break;
      case 8:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 0);
        digitalWrite(trans2, 1);
        digitalWrite(trans3, 0);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 8");
        break;
      case 9:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 1);
        digitalWrite(trans2, 0);
        digitalWrite(trans3, 0);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 9");
        break;
      case 10:
        digitalWrite(pinFuente, LOW); //prende fuente
        digitalWrite(trans1, 0);
        digitalWrite(trans2, 0);
        digitalWrite(trans3, 0);
        digitalWrite(trans4, 0);
        //Serial.println("MODO 10");
        break;
    }
    Serial.println("Modo Fuente: " + String(mod_fuente));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Funcion de la tarea 'vibracion'
void vibrador(void* params) {
  Serial.println("Task 'vibrador' is running");
  // Inicializamos el canal PWM
  ledcSetup(canalPWM, frecuenciaPWM, resolucionPWM);
  // Asignamos el canal PWM al pin de salida
  ledcAttachPin(pinPWM, canalPWM);
  // Genera una señal PWM con un ciclo de trabajo del 50%
  ledcWrite(canalPWM, 128);  // 128 representa el 50% de 255 (8 bits)
  while (true) {
    // Cambio la frecuencia del PWM
    ledcWriteTone(canalPWM, vibracion);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Vibracion: " + String(vibracion));
  }
}


/*========= SETUP =========*/

void setup() {
  // Conectividad
  pinMode(pinSecador, OUTPUT);
  pinMode(pinPeltier, OUTPUT);
  pinMode(pinSolenoide, OUTPUT);
  pinMode(pinHumi, OUTPUT);
  pinMode(pinFuente, OUTPUT);
  pinMode(trans1, OUTPUT);
  pinMode(trans2, OUTPUT);
  pinMode(trans3, OUTPUT);
  pinMode(trans4, OUTPUT);

  //relay apagado
  digitalWrite(pinSecador, HIGH);
  digitalWrite(pinPeltier, HIGH);
  digitalWrite(pinHumi, HIGH);
  digitalWrite(pinFuente, HIGH);

  Serial.begin(115200);  // Inicializar conexión Serie para utilizar el Monitor
  // Sensores y actuadores
  pinMode(DHT_PIN, INPUT);  // Inicializar el DHT como entrada
  dht.begin();              // Iniciar el sensor DHT

  Serial.println("Esperando valores iniciales");
  while (tempSetPoint == 0 && humSetPoint == 0 && vibracion == 0 && mod_fuente == 0) {
    if (Serial.available() > 0) {
      recibir_serial();
    }
  }
  xTaskCreate(secador, "secador", 2048, NULL, 1, &taskHandleSecador);  //  xTaskCreate(nombre, descripcion, tamanio en memoria, parametros, nivel de prioridad, id);
  xTaskCreate(humidificador, "humidificador", 2048, NULL, 1, &taskHandleHumidificador);
  xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, &taskHandlePeltier);
  xTaskCreate(fuente, "fuente", 2048, NULL, 1, &taskHandleFuente);
  xTaskCreate(vibrador, "vibrador", 2048, NULL, 1, &taskHandleVibrador);
}

/*========= BUCLE PRINCIPAL =========*/

void loop() {

  // === Realizar las tareas asignadas al dispositivo ===

  if (Serial.available() > 0) {
    recibir_serial();
  }
  unsigned long now = millis();
  if (now - lastMsg > msgPeriod) {
    enviar_serial();
    lastMsg = millis();
  }

  if (now - lastRead > readPeriod){
    temperatura = dht.readTemperature();  // Leer la temperatura
    humedad = dht.readHumidity();        // Leer la humedad
    }
}
