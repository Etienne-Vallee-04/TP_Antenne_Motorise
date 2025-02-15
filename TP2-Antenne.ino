//asservissement proportionelle
#include <Wire.h>
#define CMPS12_ADDRESS 0x60  // Adresse en 7 bits

// Constantes pour l'asservissement
const int SERVO_PIN = 2;      // Pin pour le servo-moteur
const float VITESSE_STOP = 1500;    // Impulsion en µs pour arrêt
const float VITESSE_MAX_CW = 2100;  // Impulsion en µs sens horaire
const float VITESSE_MAX_CCW = 900;  // Impulsion en µs sens anti-horaire

// Paramètres du contrôleur PI
const float Kp = 1.66;        // Gain proportionnel
float ki = 0.0;              // Terme intégral

// Variables pour la commande moteur
const float u_max = 600;  // Différence entre VITESSE_MAX et VITESSE_STOP (2100 - 1500 = 600)
const float u_min = -600; // Différence entre VITESSE_MIN et VITESSE_STOP (900 - 1500 = -600)

// Variables globales
float consigne = 180;        // Angle désiré (0-360°)
float position = 0;          // Position actuelle
unsigned long dernierTemps = 0;
const int DELAI_BOUCLE = 10;  // Délai de la boucle en ms

void setup() {
  //initialisation
  Serial.begin(9600);
  Wire.begin();
  delay(100);
  
  // Vérification de la communication I2C
  while (!isDeviceConnected(CMPS12_ADDRESS)) {
    Serial.println("CMPS12 non détecté. Nouvelle tentative...");
    delay(1000);
    Wire.begin();
    delay(100);
  }
  Serial.println("CMPS12 détecté !");
  
  pinMode(SERVO_PIN, OUTPUT);
  envoyerImpulsion(VITESSE_STOP);
  
  Serial.println("Entrez un angle entre 0 et 360 degrés:");
}

void loop() {
  // Contrôle de la vitesse de boucle
  if (millis() - dernierTemps < DELAI_BOUCLE) {
    return;
  }
  dernierTemps = millis();
  
  // Lecture de la position actuelle (angle 16 bits pour plus de précision)
  Wire.beginTransmission(CMPS12_ADDRESS);
  Wire.write(2);                // Registre angle 16 bits
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12_ADDRESS, 2);    // Demande 2 bytes
  unsigned char high = Wire.read();
  unsigned char low = Wire.read();
  position = ((high << 8) + low) / 10.0;  // Conversion en degrés
  
  // Calcul de l'erreur brute
  float erreur = consigne - position;
  
  // Normalisation de l'erreur (+180/-180)
  erreur = fmod(erreur, 360);
  if(erreur > 180) {
    erreur = erreur - 360;
  }
  
  // Mise à jour de ki (intégrale)
  ki += erreur * DELAI_BOUCLE / 20.0; // Conversion du délai en secondes
  
  // Calcul de la commande avec la nouvelle formule
  float u = erreur * Kp + ki;
  
  // Saturation de la commande
  if(u > u_max) {
    u = u_max;
    // Anti-windup : correction de ki
    ki = u_max - erreur * Kp;
  } else if(u < u_min) {
    u = u_min;
    // Anti-windup : correction de ki
    ki = u_min - erreur * Kp;
  }
  
  // Calcul de l'impulsion centrée sur VITESSE_STOP (1500)
  float impulsion = VITESSE_STOP + u;
  
  // Saturation finale de l'impulsion
  if(impulsion > VITESSE_MAX_CW) {
    impulsion = VITESSE_MAX_CW;
  } else if(impulsion < VITESSE_MAX_CCW) {
    impulsion = VITESSE_MAX_CCW;
  }
  
  // Application de la commande
  envoyerImpulsion(impulsion);
  
  // Affichage des informations
  Serial.print("Consigne: ");
  Serial.print(consigne);
  Serial.print("° Position: ");
  Serial.print(position);
  Serial.print("° Erreur: ");
  Serial.print(erreur);
  Serial.print("° Ki: ");
  Serial.print(ki);
  Serial.print(" Commande u: ");
  Serial.print(u);
  Serial.print(" Impulsion: ");
  Serial.println(impulsion);
  
  // Lecture des nouvelles consignes
  if (Serial.available() > 0) {
    consigne = Serial.parseFloat();
    consigne = fmod(consigne, 360);
    if (consigne < 0) consigne += 360;
    // Reset de ki lors d'un changement de consigne
    ki = 0;
    Serial.print("Nouvelle consigne: ");
    Serial.println(consigne);
    while(Serial.available()) Serial.read();
  }
}

// Vérification de la connexion I2C
bool isDeviceConnected(byte address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

// Fonction pour envoyer une impulsion au servo
void envoyerImpulsion(float largeur) {
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(largeur);
  digitalWrite(SERVO_PIN, LOW);
  delayMicroseconds(20000 - largeur);  // Complète la période de 20ms
}

//Tout ou rien
/*#include <Wire.h>
#define CMPS12_ADDRESS 0x60  // Adresse en 7 bits

// Constantes pour l'asservissement
const int SERVO_PIN = 2;      // Pin pour le servo-moteur
const int SEUIL_ERREUR = 5;   // Zone morte en degrés
const float VITESSE_STOP = 1500;    // Impulsion en µs pour arrêt
const float VITESSE_MIN_CW = 1510;  // Impulsion en µs sens horaire
const float VITESSE_MIN_CCW = 1490;  // Impulsion en µs sens anti-horaire
const float VITESSE_MAX_CW = 2100;  // Impulsion en µs sens horaire
const float VITESSE_MAX_CCW = 900;  // Impulsion en µs sens anti-horaire

// Variables globales
float consigne = 180;        // Angle désiré (0-360°)
float position = 0;        // Position actuelleS
unsigned long dernierTemps = 0;
const int DELAI_BOUCLE = 10;  // Délai de la boucle en ms

void setup() {
  Serial.begin(115200);  // Augmentation de la vitesse du port série
  Wire.begin();
  delay(100);
  
  // Vérification de la communication I2C
  while (!isDeviceConnected(CMPS12_ADDRESS)) {
    Serial.println("CMPS12 non détecté. Nouvelle tentative...");
    delay(1000);
    Wire.begin();
    delay(100);
  }
  Serial.println("CMPS12 détecté !");
  
  pinMode(SERVO_PIN, OUTPUT);
  
  Serial.println("Entrez un angle entre 0 et 360 degrés:");
}

void loop() {
  // Contrôle de la vitesse de boucle
  if (millis() - dernierTemps < DELAI_BOUCLE) {
    return;
  }
  dernierTemps = millis();
  
  // Lecture de la position actuelle (angle 16 bits pour plus de précision)
  Wire.beginTransmission(CMPS12_ADDRESS);
  Wire.write(2);                // Registre angle 16 bits
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12_ADDRESS, 2);    // Demande 2 bytes
  unsigned char high = Wire.read();
  unsigned char low = Wire.read();
  position = ((high << 8) + low) / 10.0;  // Conversion en degrés
  
  // Calcul de l'erreur
  float erreur = consigne - position;
  
  // Normalisation de l'erreur entre -180° et +180°
  if (erreur > 180) {
    erreur -= 360;
  } else if (erreur < -180) {
    erreur += 360;
  }
  
  // Asservissement tout ou rien
  if (abs(erreur) <= SEUIL_ERREUR) {
    // Dans la zone morte
    envoyerImpulsion(VITESSE_STOP);
  } else if (erreur > 0) {
    // Tourner dans le sens horaire
    envoyerImpulsion(VITESSE_MAX_CW);
  } else {
    // Tourner dans le sens anti-horaire
    envoyerImpulsion(VITESSE_MAX_CCW);
  }
  
  // Affichage des informations
  Serial.print("Consigne: ");
  Serial.print(consigne);
  Serial.print("° Position: ");
  Serial.print(position);
  Serial.print("° Erreur: ");
  Serial.print(erreur);
  Serial.println("°");
  
  // Lecture des nouvelles consignes
  if (Serial.available() > 0) {
    consigne = Serial.parseFloat();
    // Normalisation de la consigne entre 0° et 360°
    consigne = fmod(consigne, 360);
    if (consigne < 0) {
      consigne += 360;
    }
    Serial.print("Nouvelle consigne reçue: ");
    Serial.println(consigne);
  }
}

// Fonction de vérification de la connexion I2C
bool isDeviceConnected(byte address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

// Fonction pour envoyer une impulsion au servo
void envoyerImpulsion(float largeur) {
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(largeur);
  digitalWrite(SERVO_PIN, LOW);
  delayMicroseconds(20000 - largeur);  // Complète la période de 20ms
}*/

//Moteur Boucle Ouverte et Lecture de l'orientation
/*#include <Wire.h>
#define CMPS12_ADDRESS 0x60  // Adresse en 7 bits

void sequence(void);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();
  delay(100);
  // Vérification de la communication I2C
  while (!isDeviceConnected(CMPS12_ADDRESS)) {
    Serial.println("CMPS12 non détecté. Nouvelle tentative...");
    delay(1000);
    Wire.begin();
    delay(100);
  }
  Serial.println("CMPS12 détecté !");
  pinMode(2, OUTPUT);

  void sequence();
}

void loop() {
    // Lecture angle 8 bits (0-255 pour 0-360°)
    Wire.beginTransmission(CMPS12_ADDRESS);
    Wire.write(1);                // Registre angle 8 bits
    Wire.endTransmission(false);
    Wire.requestFrom(CMPS12_ADDRESS, 1);
    uint8_t angle8 = Wire.read();

    // Lecture angle 16 bits (0-3599 pour 0-359.9°)
    Wire.beginTransmission(CMPS12_ADDRESS);
    Wire.write(2);                // Registre angle 16 bits
    Wire.endTransmission(false);
    Wire.requestFrom(CMPS12_ADDRESS, 2);    // Demande 2 bytes
    unsigned char high = Wire.read();
    unsigned char low = Wire.read();
    float angle16 = ((high << 8) + low) / 10.0;  // Conversion en degrés

    // Affichage
    Serial.print(angle8);
    Serial.print(" | ");
    Serial.println(angle16);
    
    delay(500);
}

// Fonction de vérification de la connexion I2C
bool isDeviceConnected(byte address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

void sequence(void){
  //Séquence
  for (int i = 0; i < 100; i++) {
    digitalWrite(2, HIGH);
    delayMicroseconds(900);
    digitalWrite(2, LOW);
    delayMicroseconds(19100);
  }
  for (int i = 0; i < 100; i++) {
    digitalWrite(2, HIGH);
    delayMicroseconds(1490);
    digitalWrite(2, LOW);
    delayMicroseconds(18510);
  }
  for (int i = 0; i < 100; i++) {
    digitalWrite(2, HIGH);
    delayMicroseconds(1500);
    digitalWrite(2, LOW);
    delayMicroseconds(18500);
  }
  for (int i = 0; i < 100; i++) {
    digitalWrite(2, HIGH);
    delayMicroseconds(1510);
    digitalWrite(2, LOW);
    delayMicroseconds(18490);
  }
  for (int i = 0; i < 100; i++) {
    digitalWrite(2, HIGH);
    delayMicroseconds(2100);
    digitalWrite(2, LOW);
    delayMicroseconds(17900);
  }
}*/