# Projet-IT-Neige ❄️🌨️

<div align="center">
  <picture>

  <source 
      media="(prefers-color-scheme: dark)" 
      srcset="https://github.com/lorenzor0912/Projet-IT-Neige/blob/0a817e1d05e45fb6e63a99a292cdd9ac2ce48b34/ReadMe_IMG/It%20neige.svg" />
    
  <img 
      src="https://github.com/lorenzor0912/Projet-IT-Neige/blob/0a817e1d05e45fb6e63a99a292cdd9ac2ce48b34/ReadMe_IMG/It%20neige.svg" 
      alt="Logo principal" 
      width="400" 
      height="400" />
  </picture>
</div>


## Table des matières 📖

- [Hardware 🛠️](#hardware)
  - [Capteur 📸](#capteur)
  - [Carte 📺](#carte)
  - [Communications 📡](#communications)
  - [Stockage 💾](#Stockage)
  - [Batterie 🔋](#Batterie)
  - [Fonction Webcam Possible ? 🎥](#Webcam)
- [Software 🦠](#hardware) 

## Hardware 🛠️

### Capteur

SEN0313 par DF Robot aussi connu sous A01NYUB (identique) [DF Robot](https://www.dfrobot.com/product-1934.html)

<details>

<summary>Specifications Techniques</summary>


- Type : Capteur de distance ultrasonique étanche (waterproof & dustproof, IP67)

- Etanchéité : cretification IP67

- Courant : <15mA 

- Plage de mesure : 28 cm à 750 cm (soit jusqu'à 7,5 mètres)

- Resolution : 1mm 

- Interface de communication : UART (série asynchrone, très simple à utiliser)

- Tension d'alimentation : 3,3 V à 5 V (compatible Arduino, Raspberry Pi, etc.)

- Sortie : Valeur de distance directement disponible en UART (pas besoin de calculer le temps de vol soi-même)

- Autres points forts :

  - Livré avec un cône (bell mouth) amovible pour optimiser la portée et la directivité
  - Résistant à la poussière, au brouillard, à la fumée (meilleure pénétration que les HC-SR04 classiques)

</details>


![Image Alt](https://github.com/lorenzor0912/Projet-IT-Neige/blob/f1702dfe2ce56fabe681698466927644a630968b/ReadMe_IMG/SEN0313.JPG)

### Carte

All in One (Esp seulement)

| Carte / Module                        | ESP32       | 4G              | GPS          | microSD | Batterie 18650 + charge | Prix approx. (2026) | Remarques / quand choisir                                                                 |
|---------------------------------------|-------------|-----------------|--------------|---------|--------------------------|---------------------|-------------------------------------------------------------------------------------------|
| **LILYGO T-A7670G (avec GPS)**        | ESP32      | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 25-40 €            | Excellent choix global, bien supporté, tout intégré, idéal pour la simplicité et le prix |
| **Waveshare ESP32-S3-SIM7670G**       | ESP32-S3   | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 45-60 €            | Plus moderne (ESP32-S3 : plus de RAM/PSRAM/Flash, USB OTG, perf supérieures), interface caméra possible, bandes globales |
| **LILYGO T-SIM7000G** (ancienne)      | ESP32      | LTE-M / NB-IoT + 2G | Oui     | Oui    | Oui                     | ~30 €              | Moins rapide en data (pas full 4G), 2G bientôt obsolète en Europe → à éviter si besoin data rapide |
| **Walter (DPTechnics)**               | ESP32-S3   | LTE-M / NB-IoT | Oui (GNSS)  | Non (extension possible) | Non intégré     | 50-80 €            | Certifié industriel, très basse conso, certif CE/UKCA/global, mais pas de SD natif ni charge 18650 intégrée |
| **ESP32 + module SIM7600/SIM76xx séparé** | ESP32   | LTE Cat-4      | Externe     | Externe| Externe                 | Variable (40-80 €+) | Plus rapide en 4G (uploads/downloads), mais montage câblage + cher + plus compliqué     |



Est ce meme possible sur arduino?

### Comparaison : ESP32 tout-en-un vs Arduino classique + modules séparés

| Fonctionnalité                  | LILYGO T-A7670G (ou Waveshare ESP32-S3-SIM7670G) | Arduino Uno/Mega classique + modules séparés | Pourquoi l'Arduino classique est limité |
|---------------------------------|--------------------------------------------------|-----------------------------------------------------|------------------------------------------|
| **4G LTE (Cat-1 ou mieux)**    | Intégré (A7670/SIM7670)                         | Possible via shield SIM7600/SIM7670/A7670          | Oui possible (shields existent), mais souvent 5-10x plus cher que la carte tout-en-un |
| **GPS/GNSS**                    | Intégré                                         | Externe (module GPS + antenne) ou shield intégré   | Possible, mais + câblage + pins + conso |
| **microSD native**              | Slot intégré + ESP32 gère bien                  | Shield SD ou module SPI externe                    | Possible, mais conflits fréquents de pins SPI avec le modem 4G |
| **WiFi + Bluetooth**            | Intégré (ESP32/ESP32-S3)                        | Absent (sauf shield séparé)                        | Pas natif → besoin de modules supplémentaires (ESP8266/ESP32 comme co-processeur) |
| **Puissance processeur**        | Dual-core 240 MHz + 520 KB SRAM                 | 16 MHz mono-core + 2 KB SRAM (Uno)                 | Trop faible pour gérer 4G + GPS + logging SD + MQTT + JSON + OTA en simultané |
| **RAM / Flash**                 | 4-16 MB Flash + PSRAM possible                  | 32 KB Flash / 2 KB RAM                             | Impossible de faire des sketches complexes sans planter |
| **Consommation batterie**       | Optimisée pour IoT (deep sleep)                 | Très élevée si 4G + GPS actifs                     | Arduino Uno consomme ~50 mA idle, + modules = batterie vide vite |
| **Tout intégré (1 carte)**      | Oui                                             | Non (câblage + shields + breadboard)               | Beaucoup plus gros, fragile, cher à assembler |
| **Prix total (2026)**           | 25-60 € tout compris                            | 20 € Arduino + 40-80 € shield 4G/GPS + SD + GPS = 80-150 €+ | Plus cher et plus compliqué |
| **Support logiciel**            | TinyGSM, Arduino core ESP32 excellent           | AT commands manuels ou libs partielles             | Moins de tutoriels, bugs plus fréquents |

### Ce qui est possible avec un Arduino Uno/Mega

- Oui, **on** peut connecter un shield SIM7600 ou A7670 (exemples : DFRobot SIM7600G-H shield, TinySine, etc.) qui intègre souvent 4G + GPS + parfois un slot SD.

- Des exemples existent : envoi de SMS, appels vocaux, requêtes HTTP, lecture GPS NMEA via commandes AT.

- **On** peut aussi ajouter un module SD séparé (en SPI) et un GPS externe si le shield ne propose pas tout.

**Mais dans la réalité :**

- **Conflits de pins** : le bus SPI (pour la carte SD) et l’UART (pour le modem) se marchent souvent dessus → il faut fréquemment recourir à SoftwareSerial (lent et instable sur Uno/Mega).

- **RAM insuffisante** : parser les réponses JSON des commandes AT + stocker les coordonnées GPS + écrire des logs sur SD → crashes fréquents sur Uno (seulement 2 KB de RAM !).

- **Pas de multitâche** : l’ESP32 gère facilement plusieurs tâches simultanées (communication 4G + polling GPS + écriture SD + fallback WiFi), l’Arduino Uno/Mega non.

- **Pas de deep sleep efficace** pour une longue autonomie sur batterie.Ce qui est possible avec un Arduino Uno/Mega

Verdict:

- LILYGO T-A7670G

![Image Alt](https://github.com/lorenzor0912/Projet-IT-Neige/blob/37954bfae3c905698f36f1f2322b11fd72cf41e0/ReadMe_IMG/LILYGO%20T-A7670G.jpg)

ou

- Waveshare ESP32-S3-SIM7670G (plus chère mais esp32S3 donc plus puissant)

![Image Alt](https://github.com/lorenzor0912/Projet-IT-Neige/blob/77a881eeabc7f3ce35e2152226ed993d40b5088b/ReadMe_IMG/Waveshare%20ESP32-S3-SIM7670G%20.jpg)

***Mention Spécial a Waveshare ESP32-P4-Module High-performance Development Board puissance monstrueuse [ESP32-P4](https://www.waveshare.com/esp32-p4-module-dev-kit.htm?sku=30560)***


### Communications

- Via 4g
- possibitlité et intéret pour relais/émetteur [meshtastic](https://meshtastic.org/) ???
- Possibilité LoRaWan

### Stockage

Sachant que nos deux microcontrolleurs (qui représente le choix le plus economique puissant et logique) ont tous deux un port de carte sd il ne faut plus que coder et avoir une carte sd!

Estimation d'espace necessaire :

- On sait que:
  - le système doit tenir 4 mois
  - doit enregistrer   

### Batterie

Estimation de 

### Webcam

certains carte all in one on meme des mini camera (par ex waveshare) donc peut se reveler interessant

[Waveshare Demo](https://www.youtube.com/watch?v=z_u_RoW-mEs)

![Image Alt](https://github.com/lorenzor0912/Projet-IT-Neige/blob/d28ecf070d6103b8d8a0e5f1bddaf87ee4db1f34/ReadMe_IMG/Waveshare%20Cam.jpg)


<div style="line-height: 0.9; font-family: 'Courier New', Courier, monospace; white-space: pre; color: #d0d0d0;">
<pre>
   _____ _   _ ___  _____    _           _         
  / ____| | (_)__ \|  __ \  | |         | |        
 | (___ | |_ _   ) | |  | | | |     __ _| |__  ___ 
  \___ \| __| | / /| |  | | | |    / _` | '_ \/ __|
  ____) | |_| |/ /_| |__| | | |___| (_| | |_) \__ \
 |_____/ \__|_|____|_____/  |______\__,_|_.__/|___/
                                                   
                                                   
</pre>
</div>

<div align="right">
  <a href="#top">↑ Retour en haut</a>
</div>
