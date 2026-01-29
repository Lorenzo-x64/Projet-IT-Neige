# Capteur d'enneigement ❄️🌨️

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
  - [Matériaux](#Matériaux)
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

- Resolution : 1cm 

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

Voici une version améliorée de vos tableaux pour un README GitHub plus fluide et naturel :

---

### Comparaison des cartes ESP32 avec 4G/GPS intégrés

| Carte / Module                        | ESP32       | 4G              | GPS          | microSD | Batterie 18650 + charge | Prix approx. (2026) | Quand choisir cette carte ?                                                                 |
|---------------------------------------|-------------|-----------------|--------------|---------|--------------------------|---------------------|---------------------------------------------------------------------------------------------|
| **LILYGO T-A7670G (avec GPS)**        | ESP32      | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 25-40 €            | **Meilleur rapport qualité/prix**, tout intégré, bien documenté, parfait pour débuter |
| **Waveshare ESP32-S3-SIM7670G**       | ESP32-S3   | LTE Cat-1      | Oui (GNSS)  | Oui    | Oui                     | 45-60 €            | Version **plus moderne** : plus de RAM/PSRAM, meilleures perfs, support caméra, bandes globales |
| **LILYGO T-SIM7000G** (ancienne génération) | ESP32 | LTE-M / NB-IoT + 2G | Oui     | Oui    | Oui                     | ~30 €              | Débit data limité, réseau 2G en fin de vie en Europe → **à éviter** sauf budget très serré |
| **Walter (DPTechnics)**               | ESP32-S3   | LTE-M / NB-IoT | Oui (GNSS)  | Non (extension possible) | Non intégré     | 50-80 €            | **Certifié industriel**, ultra basse conso, certifs CE/UKCA, mais pas de SD ni charge batterie intégrée |
| **ESP32 + module SIM7600/SIM76xx séparé** | ESP32   | LTE Cat-4      | Externe     | Externe| Externe                 | 40-80 €+ (variable) | Débit 4G plus rapide, mais **câblage complexe**, plus cher, réservé aux utilisateurs avancés |

---

### Est-ce possible avec un Arduino classique ?


| Critère                         | LILYGO T-A7670G ou Waveshare ESP32-S3 | Arduino Uno/Mega + modules séparés | Limitations de l'Arduino classique |
|---------------------------------|----------------------------------------|------------------------------------|------------------------------------|
| **4G LTE (Cat-1 ou mieux)**    | ✅ Intégré (A7670/SIM7670)             | ⚠️ Possible via shield SIM7600/A7670 | Shields 4G existent mais coûtent **5 à 10× plus cher** que la carte ESP32 complète |
| **GPS/GNSS**                    | ✅ Intégré                             | ⚠️ Module externe ou shield        | Possible, mais câblage supplémentaire + conflits de pins + consommation accrue |
| **microSD**                     | ✅ Slot intégré, support natif         | ⚠️ Shield SD ou module SPI externe | Conflits fréquents sur le bus SPI avec le modem 4G |
| **WiFi + Bluetooth**            | ✅ Intégré (ESP32/ESP32-S3)           | ❌ Absent (nécessite module externe) | Pas de WiFi natif → besoin d'un ESP8266/ESP32 comme co-processeur |
| **Puissance processeur**        | ✅ Dual-core 240 MHz + 520 KB SRAM    | ❌ 16 MHz mono-core + **2 KB SRAM** (Uno) | **Trop faible** pour gérer 4G + GPS + SD + MQTT + JSON simultanément |
| **Mémoire (RAM / Flash)**       | ✅ 4-16 MB Flash + PSRAM disponible   | ❌ 32 KB Flash / 2 KB RAM          | Impossible de faire tourner des sketches complexes sans crasher |
| **Autonomie batterie**          | ✅ Deep sleep optimisé pour l'IoT     | ❌ Consommation élevée             | Arduino Uno consomme ~50 mA au repos, + modules 4G/GPS = batterie vide rapidement |
| **Tout intégré (1 seule carte)**| ✅ Oui                                | ❌ Non (câblage + shields multiples) | Montage encombrant, fragile, difficile à transporter |
| **Prix total (2026)**           | ✅ **25-60 € tout compris**           | ❌ 80-150 €+ (20 € Arduino + 40-80 € shields 4G/GPS + modules SD) | Plus cher ET plus compliqué à mettre en œuvre |
| **Support logiciel**            | ✅ TinyGSM, Arduino core ESP32 mature | ⚠️ Commandes AT manuelles, libs partielles | Moins de tutoriels, plus de bugs, communauté moins active |

---

### En résumé

**Pour un projet 4G + GPS + SD :**
  - ✅ **ESP32 tout-en-un** (LILYGO, Waveshare) = simple, rapide, économique, fiable
  - ❌ **Arduino Uno/Mega** = possible techniquement, mais complexe, cher et peu adapté

---

### Ce qui est possible avec un Arduino Uno/Mega

### En théorie, ça marche !

Oui, on peut tout à fait connecter un shield SIM7600 ou A7670 (comme le DFRobot SIM7600G-H, TinySine, etc.) sur un Arduino Uno ou Mega. Ces shields intègrent généralement la 4G, le GPS et parfois même un slot pour carte SD.

Plein d'exemples de projets fonctionnels existent : envoi de SMS, appels vocaux, requêtes HTTP, lecture des trames GPS NMEA via commandes AT. On peut également ajouter un module SD séparé (en SPI) et un GPS externe si le shield ne propose pas toutes ces fonctionnalités.

### Mais en pratique, on se heurte vite à des limites...

Même si c'est techniquement faisable, plusieurs problèmes apparaissent rapidement en conditions réelles :

- **Conflits de pins** : Le bus SPI (pour la carte SD) et l'UART (pour le modem) se marchent souvent dessus. On doit alors recourir à SoftwareSerial, qui est lent et peu fiable, surtout sur l'Uno.

- **RAM insuffisante** : Parser les réponses JSON des commandes AT + stocker les coordonnées GPS + écrire des logs sur SD... tout ça provoque des crashes fréquents sur l'Uno qui ne dispose que de 2 KB de RAM.

- **Pas de multitâche** : Contrairement à l'ESP32 qui gère facilement plusieurs tâches simultanées (communication 4G + polling GPS + écriture SD + fallback WiFi), l'Arduino Uno/Mega doit tout faire séquentiellement.

- **Pas de deep sleep efficace** : Difficile d'obtenir une bonne autonomie sur batterie sans gestion avancée de la consommation.

---

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
  - toute 30 sec estimation sur 3 mois de 552 Ko

**Carte SD Haute endurance**

  - [Sd Sandisk Industrial XI 8Go de 85° a -40 13€ ](https://www.mouser.fr/ProductDetail/SanDisk/SDSDQAF3-008G-XI?qs=F5EMLAvA7IAdyu9puKxNsg%3D%3D)
  - SanDisk High Endurance microSDXC
  - Lexar microSDHC/microSDXC UHS-I High Endurance Card
  - Samsung PRO Endurance microSDXC UHS-I U3
  - PNY Pro Elite™ High Endurance C10 U3 V30 A2 MicroSDXC Memory Card
  - Kingston High Endurance microSDXC95R
  - Micro SD KIOXIA EXCERIA High Endurance UHS-I C10 R98





---
### Batterie

Estimation de 






---

### Webcam

certains carte all in one on meme des mini camera (par ex waveshare) donc peut se reveler interessant

[Waveshare Demo](https://www.youtube.com/watch?v=z_u_RoW-mEs)

![Image Alt](https://github.com/lorenzor0912/Projet-IT-Neige/blob/d28ecf070d6103b8d8a0e5f1bddaf87ee4db1f34/ReadMe_IMG/Waveshare%20Cam.jpg)




---


### Matériaux




---

### Comparaison filaments extrêmes : -30°C / 10 ans neige/UV/humidité ❄️⛄

| Critère                              | ASA-CF       | PETG-CF      | PET          | ABS          | PLA          | ASA (std)    | PC           | PETG (std)   | Nylon (PA)   |
|--------------------------------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|
| Résiste bien à -30°C (pas cassant)   | ✅✅         | ✅✅         | ✅           | ✅           | ❌❌         | ✅✅         | ✅✅ (souplissime) | ✅✅         | ✅✅ (flexible) |
| Durée 10 ans UV + intempéries/neige  | ✅✅ (UV top + CF boost) | ✅ (bon UV, CF aide) | ❌ (dégrade) | ❌ (jaunit/craque) | ❌❌ (détruit vite) | ✅✅ (UV leader) | ✅ (bon, mais moins UV) | ❌ (jaunit après années) | ❌ / ✅ (si PA12, bon si protégé) |
| Étanchéité / neige & humidité        | ✅✅ (~0.3-0.5% absorption) | ✅✅ (hydrophobe) | ✅           | ✅           | ❌ (gonfle)  | ✅           | ✅ (mais hygro) | ✅✅ (très hydrophobe) | ❌ (absorbe beaucoup, sauf PA12) |
| Reste étanche/dimension stable longtemps | ✅✅         | ✅✅         | ✅           | ✅ (shrink)  | ❌           | ✅✅         | ✅           | ✅✅         | ❌ (sauf PA12) |
| Facile à imprimer                    | ❌ (boîtier + nozzle hard) | ❌ (abrasif) | ✅           | ❌ (warping) | ✅✅ (facile) | ❌ (boîtier) | ❌❌ (dur)   | ✅✅ (facile) | ❌ (séchage + boîtier) |
| Rigidité / résistance chocs au froid | ✅✅ (très rigide) | ✅✅ (boost CF) | ✅           | ✅           | ✅ / ❌ (cassant) | ✅           | ✅✅ (top chocs) | ✅           | ✅✅ (abrasion + flex) |

**Verdict rapide pour ton usage (-30°C, neige, 10 ans dehors)**  
✅ **ASA-CF** → Le gagnant global : UV imbattable (10+ ans dehors sans jaunir/craquer), faible absorption humidité, tient -30°C sans casser, rigidité boostée par CF. Idéal pour pièces exposées neige/soleil (ex: boîtiers, supports extérieurs).  

✅ **PETG-CF** → Très bon compromis : super hydrophobe (ne gonfle pas en neige), flexible au froid, UV correct (mieux que PETG std), facile relatif. Moins cher/simple que ASA-CF.  

✅ **PC** → Si priorises chocs violents au froid extrême (reste souple, pas cassant).  

✅ **ASA (std)** → Si pas besoin de CF ultra-rigide, c'est le roi UV/long terme sans complications.  

❌ **PLA / ABS / PET** → À éviter pour 10 ans dehors ou froid extrême (cassent/jaunissent/dégradent).  

❌ **Nylon** → OK si PA12 (faible humidité), sinon absorbe trop l'eau en neige → gonfle/déforme.  

Pour max étanchéité/UV sur 10 ans, coating époxy ou peinture UV après impression. ASA-CF + coating = quasi-indestructible en environnement neigeux !


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

 <img 
      src="https://github.com/Lorenzo-x64/Projet-IT-Neige/blob/a68dd4287c40711deb7713e88d299c58865ecca4/ReadMe_IMG/Sti%20Labs.svg" 
      alt="Logo principal" 
      width="600" 
      height="600" />

<div align="right">
  <a href="#top">↑ Retour en haut</a>
</div>
