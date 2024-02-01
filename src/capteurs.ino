#include <NewPing.h>
#include <DHT.h>
#include <MKRWAN.h>

// Port digital connecté au capteur
#define DHTPin 8
DHT dht(DHTPin, DHT22);

// LoRa
LoRaModem modem(Serial1);
String appEui = "0000000000000000";
String appKey = "BAEA325C357D251C06773220585291BE";

#define TRIGGER_PIN  1
#define ECHO_PIN     0
#define MAX_DISTANCE 100 // Maximum distance we want to measure (in centimeters).
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
unsigned long sonarDernierScan = 0; // ms
int sonarScanTemps = 50; // ms
int stopDistance = 10; // cm

unsigned long dhtDernierEnvoie = 0; // ms
int dhtEnvoieTemps = 20000; // ms

bool buttonPressed = false;

// Fonction pour initialiser les capteurs et la connexion LoRa
void SetupCapteur() {
  dht.begin();

  // Configuration du bouton 
  pinMode(9, INPUT);

  modem.begin(US915);

  // Connexion LORA
  Serial.print("Identifiant EUI: ");
  Serial.println(modem.deviceEUI());

  int connected = modem.joinOTAA(appEui, appKey);
  while(!connected) {
    connected = modem.joinOTAA(appEui, appKey);
  }



  modem.minPollInterval(60);
}

// Fonction qui permet de récupérer et d'envoyer des données selon la fonction millis()
void LoopCapteur() {
  // Capteur - Sonar

  // S'il faut lancer un scan
  if(millis() - sonarDernierScan >= sonarScanTemps) {
    sonarDernierScan = millis();

    // Si le robot n'est pas arrêté, et qu'il n'est pas entrain de contourner un obstacle
    if(!EstArrete() && !contournement) {
      if(VerifierObstacle()) {
        //DebutContournement();
      }
    }
  }

  // Capteur - DHT22
  // Récupération de l'humidité et de la température du capteur
  if(millis() - dhtDernierEnvoie >= dhtEnvoieTemps) {
    dhtDernierEnvoie = millis();

    float humidite = dht.readHumidity();
    float temperatureCelsus = dht.readTemperature();

    EnvoyerDonnees(humidite, temperatureCelsus);
  }

  // Capteur - Bouton
  // Lire l'état actuel du bouton
  int etatBouton = digitalRead(9);
  if(etatBouton == HIGH && !buttonPressed) {
    buttonPressed = true;
    DemarrerProgramme();
  } else if(etatBouton == LOW) {
    buttonPressed = false;
  }
}

// Fonction qui perlet de vérifier si un obstacle est devant le robot ou non
bool VerifierObstacle() {
  int distance = sonar.ping_cm(); // Mesure de la distance avec le capteur
  return distance <= stopDistance && distance != 0;
}

// Fonction qui permet d'envoyer des données sur TTN
void EnvoyerDonnees(float humidite, float temperature) {
  int err;
  modem.beginPacket();
  modem.print(humidite);
  modem.print(";");
  modem.print(temperature);
  err = modem.endPacket(true);
}