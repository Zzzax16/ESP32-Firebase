#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

#define WIFI_SSID "INNO-Carmen Proanio"
#define WIFI_PASSWORD "LPVC1966"
#define API_KEY "AIzaSyCs9998eFyNORaiDXo9b2eZuVyGXYrCxvE"
#define DATABASE_URL "https://desarrollo-de-apps-movil-f18ce-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pines
const int ledPin = 27;
const int botonAbrir = 26;
const int botonCerrar = 33;

bool estadoPuerta = false; // false = cerrada, true = abierta
bool estadoAnteriorAbrir = HIGH;
bool estadoAnteriorCerrar = HIGH;
unsigned long tiempoUltimaLectura = 0;
const int retardoRebote = 300;

void parpadearLED(int veces);
String obtenerFechaHora();
void leerPulsadores();
void consultarFirebase();
void abrirPuerta();
void cerrarPuerta();
void actualizarFirebase(bool estado);
void registrarHistorial(String accion);

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(botonAbrir, INPUT_PULLUP);
  pinMode(botonCerrar, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "pato@admin.com";
  auth.user.password = "123456";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  leerPulsadores();
  consultarFirebase();
  delay(500);
}

void leerPulsadores() {
  bool lecturaActualAbrir = digitalRead(botonAbrir);
  bool lecturaActualCerrar = digitalRead(botonCerrar);

  if (estadoAnteriorAbrir == HIGH && lecturaActualAbrir == LOW && (millis() - tiempoUltimaLectura) > retardoRebote) {
    parpadearLED(1);
    abrirPuerta();
    tiempoUltimaLectura = millis();
  }

  if (estadoAnteriorCerrar == HIGH && lecturaActualCerrar == LOW && (millis() - tiempoUltimaLectura) > retardoRebote) {
    parpadearLED(2);
    cerrarPuerta();
    tiempoUltimaLectura = millis();
  }

  estadoAnteriorAbrir = lecturaActualAbrir;
  estadoAnteriorCerrar = lecturaActualCerrar;
}

void abrirPuerta() {
  digitalWrite(ledPin, HIGH);
  estadoPuerta = true;
  actualizarFirebase(true);
  registrarHistorial("Puerta abierta");
  Serial.println("Puerta abierta - LED encendido");
}

void cerrarPuerta() {
  digitalWrite(ledPin, LOW);
  estadoPuerta = false;
  actualizarFirebase(false);
  registrarHistorial("Puerta cerrada");
  Serial.println("Puerta cerrada - LED apagado");
}

void consultarFirebase() {
  if (Firebase.RTDB.getBool(&fbdo, "/ESTADO_OBJETO/estado")) {
    bool estado = fbdo.boolData();
    if (estado != estadoPuerta) {
      if (estado) {
        parpadearLED(1);
        abrirPuerta();
      } else {
        parpadearLED(2);
        cerrarPuerta();
      }
    }
  } else {
    Serial.printf("Error leyendo Firebase: %s\n", fbdo.errorReason().c_str());
  }
}

void actualizarFirebase(bool estado) {
  Firebase.RTDB.setBool(&fbdo, "/ESTADO_OBJETO/estado", estado);
}

void registrarHistorial(String accion) {
  String fechaHora = obtenerFechaHora();
  String registro = accion;
  registro += " - ";
  registro += fechaHora;

  String ruta = "/HISTORIAL/";
  ruta += String(millis());

  Firebase.RTDB.setString(&fbdo, ruta, registro);
}

void parpadearLED(int veces) {
  for (int i = 0; i < veces; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
}

String obtenerFechaHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Hora no disponible";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}