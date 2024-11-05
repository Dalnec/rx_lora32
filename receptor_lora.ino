//Librerias OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//Librerias LoRa
#include <LoRa.h>
#include <SPI.h>
//Librerias WiFi
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//pines modulo LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// Frecuencia de Operacion LoRa
#define BAND 915E6

#define SCREEN_WIDTH 128  // Ancho de la pantalla OLED
#define SCREEN_HEIGHT 64  // Alto de la pantalla OLED

// Dirección I2C de la pantalla (puede variar, comúnmente 0x3C o 0x3D)
#define OLED_RESET -1  // Compartimiento de reset (-1 si no se usa un pin de reset)
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Cadena de texto para recibir datos del otro LoRa.
// String dataLora;
String dataLora;
String measured;
String percentage;
String battery;
// char measured;
// char percentage;
// char battery;

const char* ssid = "Redmi Note 10 Pro";
const char* password = "Qwerty1234";
const String serverName = "http://facturacionapi.tsi.pe/api/";
// const String serverName = "http://192.168.1.7:8000/api/";

// Timer variables
unsigned long lastTime = 0;
String districName;
int settingsTimerDelay;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  init_screen();
  init_lora();

  init_WiFi();
  init_data();
  show_init_info_display(districName, settingsTimerDelay);
}

void loop() {
  //analiza paquete
  int packageSize = LoRa.parsePacket();
  if (packageSize) {
    Serial.print("Paquete recibido ");
    //Leemos el paquete
    while (LoRa.available()) {
      //Guardamos cadena en variable
      dataLora = LoRa.readString();
      Serial.println(dataLora);
    }
    //Muestra intensidad de señal recibida
    int rssi = LoRa.packetRssi();
    Serial.print(" con RSSI ");
    Serial.println(rssi);
    treat_data(dataLora);
    // show_basic_display(dataLora, rssi);
    show_info_screen2(String(measured.toInt()) + " cm", percentage + "%", String(battery.toInt()) + "%", rssi);
  }
  if ((millis() - lastTime) > settingsTimerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      send_data(measured, percentage, String(battery.toInt()));
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void init_screen() {
  // Inicializa la pantalla con la dirección I2C
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Fallo en la asignación de SSD1306"));
    for (;;)
      ;  // Detiene la ejecución si la pantalla no se inicia correctamente
  }
  // Limpia la pantalla
  display.clearDisplay();
  // Configura el tamaño del texto
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
}

void init_lora() {
  Serial.println("INICIO RECEPTOR LoRa");
  delay(2000);
  //Definimos pines SPI
  SPI.begin(SCK, MISO, MOSI, SS);
  //Configuramos LoRa para enviar
  LoRa.setPins(SS, RST, DIO0);

  //Intenta transmitir en la banda elegida
  if (!LoRa.begin(BAND)) {
    //Si no puede transmitir, marca error
    Serial.println("Error iniciando LoRa");
    while (1)
      ;
  }
  //Mensaje de todo bien en puerto serial
  Serial.println("Inicio exitoso LoRa!");
  display.setCursor(0, 10);
  //Mensaje de todo bien en pantalla OLED
  display.print("Inicio exitoso LoRa!");
  display.display();
  delay(2000);
}

void show_basic_display(String dataLora, int rssi) {
  // Mostramos informacion obtenida
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Receptor LoRa");
  display.setCursor(0, 20);
  display.print("Paquete recibido:");
  display.setCursor(0, 30);
  display.print(dataLora);
  display.setCursor(0, 40);
  display.print("RSSI:");
  display.setCursor(30, 40);
  display.print(rssi);
  display.display();
}

void show_init_info_display(String distric, int interval) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(5, 5);
  display.print("Barrio/OTB:");
  display.setCursor(8, 20);
  display.print(distric);
  display.setCursor(5, 40);
  display.print("Intervalo:");
  display.setCursor(35, 55);
  display.print(interval);
  display.print(" sec");
  display.display();
  delay(5000);
}

void show_info_screen2(String distancia, String porcentaje, String batteryLevel, int rssi) {
  //Limpia pantalla
  display.clearDisplay();
  //Posicionamos en siguiente renglon
  display.setTextSize(1);
  display.setCursor(3, 5);
  display.print(rssi);
  display.setTextSize(2);
  display.setCursor(45, 5);
  display.print(distancia);
  display.setCursor(20, 25);
  display.print("AGUA");
  display.setCursor(75, 25);
  display.print(porcentaje);
  display.setCursor(45, 45);
  display.print(batteryLevel);
  display.display();
}

void treat_data(String dataLora) {
  char *data = NULL;
  int str_len = dataLora.length() + 1;
  char char_array[str_len];
  dataLora.toCharArray(char_array, str_len);
  data = strtok(char_array, ",");
  if (data != NULL) {
    if (atoi(data) > 0) {
      measured = data;
    } else {
      measured = "0";
    }
    // measured = atoi(data);  // Convertir a entero
  }

  data = strtok(NULL, ",");
  if (data != NULL) {
    if (atoi(data) > 100) {
      percentage = 100;
    } else {
      if (atoi(data) < 0) {
        percentage = "0";
      } else {
        percentage = data;
      }
      // percentage = atoi(data);
    }
  }

  data = strtok(NULL, ",");
  if (data != NULL) {
    if (atoi(data) > 100) {
      battery = 100;
    } else {
      if (atoi(data) < 0) {
        battery = "0";
      } else {
        battery = data;
      }
    }
    // battery = atoi(data);
  }
}

void init_WiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connection failed with status: ");
    Serial.println(WiFi.status());
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void init_data() {
  WiFiClient initClient;
  HTTPClient initHttp;

  initHttp.begin(initClient, serverName + "distric/1/");
  int initHttpResponseCode = initHttp.GET();
  delay(500);

  if (initHttpResponseCode > 0) {
    String payload = initHttp.getString();

    // Parsear JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("Error parsing JSON: ");
      Serial.println(error.c_str());
    } else {
      const char* name = doc["name"];
      districName = name;
      Serial.print("BARRIO/OTB: ");
      Serial.println(name);

      int interval_time_device = doc["settings"]["interval_time_device"];
      settingsTimerDelay = interval_time_device;
      Serial.print("Tiempo envio datos: ");
      Serial.println(interval_time_device);
    }
  } else {
    Serial.print("Error on initHttp request: ");
    Serial.println(initHttpResponseCode);
  }

  initHttp.end();
}

void send_data(String measured, String percentage, String battery) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName + "monitoring/");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST("{\"measured\":\"" + measured + "\", \"percentage\":\"" + percentage + "\", \"battery\":\"" + battery + "\"}");
  delay(400);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  // Serial.println(http.getString());
  if (httpResponseCode == 201) {
    String payload = http.getString();
    measured = "0";
    percentage = "0";
    battery = "0";
    // Parsear JSON
    StaticJsonDocument<200> resp;
    DeserializationError error = deserializeJson(resp, payload);
    if (error) {
      Serial.print("Error parsing JSON: ");
      Serial.println(error.c_str());
    } else {
      settingsTimerDelay = resp["interval_time_device"];
      Serial.print("settingsTimerDelay: ");
      Serial.println(settingsTimerDelay);
    }
  }
  http.end();
}