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
// Tipo de sensor
#define DHTTYPE DHT11  // tipo de DHT
#define DHT_PIN 21     // DHT en PIN D21

/*========= VARIABLES =========*/
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

/*========= FUNCIONES =========*/

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
  //xTaskCreate(secador, "secador", 2048, NULL , 1, NULL); //  xTaskCreate(nombre, descripcion, tamanio en memoria, parametros, nivel de prioridad, id);
  //xTaskCreate(humidificador, "humidificador", 2048, NULL, 1, NULL);
  xTaskCreate(celdaPeltier, "celdaPeltier", 2048, NULL, 1, NULL);
  
}

/*========= BUCLE PRINCIPAL =========*/

void loop() {

  // === Realizar las tareas asignadas al dispositivo ===

  
}
