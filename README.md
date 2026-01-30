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


### Matériaux




---

### Comparaison filaments extrêmes : -30°C / 10 ans neige/UV/humidité ❄️⛄

| Critère | ASA-CF | PETG-CF | PET | ABS | PLA | ASA (std) | PC | PETG (std) | Nylon (PA) |
|--------------------------------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------| 
| **Résistance au froid (-30°C)** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🔴🔴 | 🟢🟢 | 🟢🟢 | 🟢🟢 | 🟢🟢 |
| **Durabilité UV (10 ans ext.)** | 🟢🟢 | 🟢 | 🔴 | 🔴 | 🔴🔴 | 🟢🟢 | 🟡 | 🟡 | 🟡 |
| **Résistance à l'humidité** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🔴 | 🟢🟢 | 🟡 | 🟢🟢 | 🔴 |
| **Stabilité dimensionnelle** | 🟢🟢 | 🟢🟢 | 🟢 | 🟡 | 🔴 | 🟢🟢 | 🟢 | 🟢🟢 | 🔴 |
| **Facilité d'impression** | 🔴 | 🔴 | 🟢 | 🟡 | 🟢🟢 | 🟡 | 🔴🔴 | 🟢🟢 | 🔴 |
| **Résistance mécanique** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🟡 | 🟢 | 🟢🟢 | 🟢 | 🟢🟢 |

## Légende
- 🟢🟢 Excellent
- 🟢 Bon
- 🟡 Moyen / Conditions requises
- 🔴 Faible / Déconseillé
- 🔴🔴 Très faible / À éviter

### Résistance au froid
- **PLA** : Devient cassant en dessous de 0°C
- **PC & Nylon** : Restent flexibles même à -30°C

### Dur

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
