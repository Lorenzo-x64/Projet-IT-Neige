# Projet-IT-Neige

# Sensors

- SEN0313 DIGIKEY Fabricant df robot (ip67) [Link](https://www.digikey.fr/fr/products/detail/dfrobot/SEN0313/11202720)

- JSN-SR04T (Non IP66) (accuracy 1cm)


# Snow Depth JSN-SR04T

## 📋 Table des matières
- [Installation](#installation)
- [Schéma](#schéma)  
- [Code Arduino](#code-arduino)
- [Calibration Neige](#calibration-neige)

## Installation {#installation}
Connecte VCC→5V, Trig→Pin9, Echo→Pin10...

## Schéma {#schéma}
![Schéma](schema.jpg)

## Code Arduino {#code-arduino}
```cpp
#define TRIG_PIN 9
// etc...

