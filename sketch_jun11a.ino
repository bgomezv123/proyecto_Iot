/*
Proyecto IoT
Equipo:
--Gomez Velaco Brian Jospeh
--Jacobo Castillo Andrew Pold 
--Chuctaya Ruiz Diego Moises
--Ramos Ticona Gilbert
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// Valores para la conexion WiFi
const char* ssid = "GOMEZ 2,4G";
const char* password = "holathiago123456789";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
//AWS este valor se optiene del portal de AWS
const int pinD7 = 13;
const int pinLDR = A0;
bool onOff = true;

 //MQTT broker ip (EndPoint obtenido del portal AWS)
const char * AWS_endpoint = "a34bt8gk372w9w-ats.iot.us-east-2.amazonaws.com";

//CallBack, se imprime en el Monitor Serie el mensaje recivido por el servicio Cloud
void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
 
 // Si el valor del payload es 1 se enciende el led, de lo contrario se apaga
  if ((char)payload[0] == '1') {
    onOff = true;
  } else {
    onOff = false;  
  }

 
}

WiFiClientSecure espClient;
//MQTT port 8883 - standard
PubSubClient client(AWS_endpoint, 8883, callback, espClient); 
long lastMsg = 0;
char msg[50];
int value = 0;
void setup_wifi() {
  delay(10);
  // Inicializamos Wifi
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}
void reconnect() {
  // Reconeccion si es necesario
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPthing")) {
      Serial.println("connected");

      client.publish("outTopic", "Hola, somos Brian, Andrew , Diego y Gilbert!!!!!!!! ");
      
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  // inicializamos el led del esp8266
  pinMode(LED_BUILTIN, OUTPUT);
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  // Cargamos los certificados
  File cert = SPIFFS.open("/cert.der", "r"); //verifica
  if (!cert) {
    Serial.println("Failed to open cert file");
  } else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
  // Cargamos llave privada
  File private_key = SPIFFS.open("/private.der", "r"); 
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  } else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
  // Cargamos CA 
  File ca = SPIFFS.open("/ca.der", "r"); 
  if (!ca) {
    Serial.println("Failed to open ca ");
  } else
    Serial.println("Success to open ca");
  delay(1000);
  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

unsigned long tiempoPrevio = 0; // Variable para almacenar el tiempo previo
unsigned long intervalo = 10000; 

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Lee el valor analógico del sensor LDR
  int valorLDR = analogRead(pinLDR);
  client.loop();
  //De acuerdo al valor analogico del sensor LDR establecemos el nivel de luminosidad del LED
  if(onOff){
      if(valorLDR < 20){
        analogWrite(pinD7, 0);
      }else if(valorLDR >= 20 && valorLDR < 50){
        analogWrite(pinD7, valorLDR/3);
      }
      else if(valorLDR >=50 && valorLDR < 60){
        analogWrite(pinD7, valorLDR*3);
      } else {
        analogWrite(pinD7, valorLDR*5);
      }
  }
  else{
    analogWrite(pinD7, 0);
  }

  //Contenamoes el siguiente string con el valor del sensor para imprimirlo en el Monitor Serie
  char mensaje[50] = "Valor del sensor : ";
  char strValorLDR[4];
  sprintf(strValorLDR, "%d", valorLDR);
  strcat(mensaje, strValorLDR);
  
   if (millis() - tiempoPrevio >= intervalo) {
      tiempoPrevio = millis();
      //Publicamos el mensaje del sensor en el servicio Cloud(AWS), con el topico llamado "outTopic"
      client.publish("outTopic", mensaje);
      //Imprimimos el mensaje en el monitor serie
      Serial.println(mensaje);
   }
 
  //Recibimos el mensaje enviado por el servicio cloud (AWS, con el topico llamado "inTopic"
  char recivedMsg = client.subscribe("inTopic",1);
  Serial.println(recivedMsg);

}
