#include <SPI.h>
#include <Wire.h>
#include <Ultrasonic.h>
#include <SD.h>
Ultrasonic ultrasonic(7);
int hauteur_capteur = 200; // défini la hauteur du capteur
int RangeInCentimeters = 0.0;
File Test_Projet_Var;

void setup() {
  Serial.begin(9600);
pinMode(10, OUTPUT);   // obligatoire pour le SPI
if (!SD.begin(10)) {
  Serial.println("Carte SD non détectée");
  while (1);
}
Serial.println("Carte SD OK");
}

void loop() {
RangeInCentimeters = ultrasonic.MeasureInCentimeters();
Test_Projet_Var = SD.open("Test_P.csv", FILE_WRITE);
  delay(500); // délai de 500 ms
  
  int epaisseur_cm = hauteur_capteur - RangeInCentimeters; // Calcul de l'épaisseur
  const int epaisseur_mm = epaisseur_cm * 10; // Conversion en mm
 
  Test_Projet_Var.println(epaisseur_mm);
  Serial.println(epaisseur_mm); // Afficher sur le moniteur série
  Test_Projet_Var.close();

  delay(1000); // Attendre 1 heure avant la prochaine mesure
}