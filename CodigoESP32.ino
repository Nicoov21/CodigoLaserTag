#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>

#define PIN_LED 27        
#define NUMPIXELS 10    
#define LDR_PIN 34
#define LDR_PINT 35
#define LDR_PIND 33   
#define BUZZER_PIN 25

const char* ssid = "nombrewifi";
const char* password = "contraseñawifi";
const char* mqtt_server = "mqtt.eclipseprojects.io";
const char* mqttTopic = "st/iot/grupo7";

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

int vida = 100;
int muerto = 0;
int disparos = 0;
int ciclo = 0;
bool juegoIniciado = false;

void setup_wifi() {
  delay(10);
  Serial.println("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConexión WiFi establecida");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == 'r') { 
    vida = 100;
    muerto = 0;
    juegoIniciado = false;
    Serial.println("Reinicio del juego");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado a MQTT");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void actualizarLEDs(int vida) {
  int lucesVerdes = (vida / 10.0) + 0.5;
  int lucesTotales = NUMPIXELS;

  for (int i = 0; i < lucesVerdes; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0)); // Verde
  }
  for (int i = lucesVerdes; i < lucesTotales; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0)); // Rojo
  }
  pixels.show();
}
void Reseteo(int disparos){
  while(ciclo != 3){
    for (int i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, pixels.Color(0,0,0));
      pixels.show();
      delay(30);
    }
    for (int i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, pixels.Color(255,0,0));
      pixels.show();
      delay(30);
    }
    ciclo++;
  }
}

void publicarVida() {
  snprintf(msg, MSG_BUFFER_SIZE, "%d", vida);
  client.publish("st/iot/grupo7/vida", msg);
}
void publicarDisparos(){
  snprintf(msg, MSG_BUFFER_SIZE, "%d", disparos);
  client.publish("st/iot/grupo7/disparos", msg);
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pixels.begin();
  actualizarLEDs(vida);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (!juegoIniciado) {
    lcd.setCursor(0, 0);
    lcd.print("Dispara para");
    lcd.setCursor(0, 1);
    lcd.print("iniciar");
    int valorLDR = analogRead(LDR_PIN);
    int valorLDRD = analogRead(LDR_PIND);
    int ValorLDRT = analogRead(LDR_PINT);
    if (valorLDR < 2500 || valorLDRD < 2500 || ValorLDRT < 2500) { 
      juegoIniciado = true;
      lcd.setCursor(0, 0);
      lcd.print("El juego va ");
      lcd.setCursor(0, 1);
      lcd.print("a comenzar");
      delay(1000);
      publicarVida();
    }
    return;
  }


  if (muerto == 0) {
    int LDR = analogRead(LDR_PIN);
    int LDRD = analogRead(LDR_PIND);
    int LDRT = analogRead(LDR_PINT);
    if (LDR < 800) {
      vida -= 5;
      disparos++;
      tone(BUZZER_PIN, 1000); 
      delay(100);            
      noTone(BUZZER_PIN);  
      delay(100);
      publicarVida();
    }else if(LDRD < 800){
      vida -= 10;
      disparos++;
      tone(BUZZER_PIN, 1000); 
      delay(100);            
      noTone(BUZZER_PIN);   
      delay(100);
      publicarVida();
    }else if (LDRT < 800){
      vida -= 15;
      disparos++;
      tone(BUZZER_PIN, 1000); 
      delay(100);            
      noTone(BUZZER_PIN);   
      delay(100);
      publicarVida();
    }

    if (vida <= 0) {
      muerto = 1;
      vida = 0;
      publicarVida();
      publicarDisparos();
    }
    actualizarLEDs(vida);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Has muerto X X");
    lcd.setCursor(12, 1);
    lcd.print("O");
    delay(3000);
    Reseteo(10);
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reseteando");
    lcd.setCursor(0, 1);
    lcd.print("Valores");
    delay(2000);
    vida = 100;
    muerto = 0;
    disparos = 0;
    ciclo = 0;
    juegoIniciado = false;
    actualizarLEDs(vida);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vida: ");
  lcd.print(vida);
  delay(100);
}