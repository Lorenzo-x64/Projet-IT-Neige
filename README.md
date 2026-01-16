# Projet-IT-Neige

## Table des matières 📖

- [Hardware 🛠️](#hardware)
  - [Capteur 📸](#capteur)
  - [Carte 📺](#carte)
  - [Communications 📡](#communications)
  - [Stockage 💾](#Stockage)
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

| Carte / Module                        | ESP32       | 4G              | GPS          | microSD | Batterie 18650 + charge | Prix approx. (2026) | Remarques / quand choisir                                                                 |
|---------------------------------------|-------------|-----------------|--------------|---------|--------------------------|---------------------|-------------------------------------------------------------------------------------------|
| **LILYGO T-A7670G (avec GPS)**        | ESP32      | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 25-40 €            | Excellent choix global, bien supporté, tout intégré, idéal pour la simplicité et le prix |
| **Waveshare ESP32-S3-SIM7670G**       | ESP32-S3   | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 45-60 €            | Plus moderne (ESP32-S3 : plus de RAM/PSRAM/Flash, USB OTG, perf supérieures), interface caméra possible, bandes globales |
| **LILYGO T-SIM7000G** (ancienne)      | ESP32      | LTE-M / NB-IoT + 2G | Oui     | Oui    | Oui                     | ~30 €              | Moins rapide en data (pas full 4G), 2G bientôt obsolète en Europe → à éviter si besoin data rapide |
| **Walter (DPTechnics)**               | ESP32-S3   | LTE-M / NB-IoT | Oui (GNSS)  | Non (extension possible) | Non intégré     | 50-80 €            | Certifié industriel, très basse conso, certif CE/UKCA/global, mais pas de SD natif ni charge 18650 intégrée |
| **ESP32 + module SIM7600/SIM76xx séparé** | ESP32   | LTE Cat-4      | Externe     | Externe| Externe                 | Variable (40-80 €+) | Plus rapide en 4G (uploads/downloads), mais montage câblage + cher + plus compliqué     |

### Communications


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
