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
  
  <p align="center">
    <strong>Solution autonome de mesure d'enneigement pour environnements extrêmes</strong>
  </p>
  
  <p align="center">
    <img src="https://img.shields.io/badge/Température--30°C_à_+50°C-blue?style=flat-square" alt="Température"/>
    <img src="https://img.shields.io/badge/Portée-7.5m-green?style=flat-square" alt="Portée"/>
    <img src="https://img.shields.io/badge/Autonomie-4_mois-orange?style=flat-square" alt="Autonomie"/>
    <img src="https://img.shields.io/badge/Étanchéité-IP67-lightblue?style=flat-square" alt="IP67"/>
  </p>
</div>

---

## 📋 Table des matières

- [🎯 Vue d'ensemble](#-vue-densemble)
- [✨ Caractéristiques principales](#-caractéristiques-principales)
- [🛠️ Hardware](#️-hardware)
  - [📸 Capteur ultrasonique](#-capteur-ultrasonique)
  - [🖥️ Microcontrôleur](#️-microcontrôleur)
  - [📡 Communications](#-communications)
  - [💾 Stockage](#-stockage)
  - [🔋 Alimentation](#-alimentation)
  - [🏗️ Matériaux & Boîtier](#️-matériaux--boîtier)
- [💻 Software](#-software)
- [📊 Comparatif des matériaux](#-comparatif-des-matériaux)
- [🚀 Installation & Déploiement](#-installation--déploiement)
- [🤝 Contribution](#-contribution)
- [📜 Licence](#-licence)

---

## 🎯 Vue d'ensemble

Ce projet propose une **station de mesure d'enneigement autonome** conçue pour fonctionner dans des conditions climatiques extrêmes (jusqu'à -30°C). Idéale pour :

- 🎿 **Stations de ski** : Suivi en temps réel de l'enneigement des pistes
- 🏔️ **Observation météorologique** : Collecte de données nivologiques
- 🚧 **Infrastructure** : Détection d'accumulation de neige sur toitures
- 🌲 **Recherche environnementale** : Étude de l'évolution du manteau neigeux

---

## ✨ Caractéristiques principales

| Caractéristique | Spécification |
|-----------------|---------------|
| **Plage de mesure** | 28 cm à 750 cm (7,5 m) |
| **Résolution** | 1 cm |
| **Température d'opération** | -30°C à +50°C |
| **Étanchéité** | IP67 (submersible) |
| **Autonomie** | 4 mois (mode normal) |
| **Fréquence de mesure** | Configurable (défaut : 30 sec) |
| **Connectivité** | 4G / LoRaWAN / Meshtastic |
| **Stockage local** | Carte SD haute endurance |

---

## 🛠️ Hardware

### 📸 Capteur ultrasonique

**Modèle** : SEN0313 (DF Robot) / A01NYUB

<details>
<summary><strong>📋 Spécifications techniques détaillées</strong></summary>

#### Caractéristiques principales
- **Type** : Capteur ultrasonique étanche IP67
- **Plage de mesure** : 28 cm à 750 cm
- **Résolution** : 1 cm
- **Précision** : ±1%
- **Angle de détection** : 70° (avec cône fourni)

#### Électrique
- **Tension d'alimentation** : 3,3 V à 5 V DC
- **Consommation** : <15 mA (actif) / <5 mA (veille)
- **Interface** : UART (9600 bps par défaut)

#### Environnemental
- **Température d'opération** : -15°C à +60°C
- **Étanchéité** : IP67 (immersion jusqu'à 1 m pendant 30 min)
- **Résistance** : Poussière, brouillard, fumée

#### Avantages
- ✅ Mesure directe en UART (pas de calcul de temps de vol)
- ✅ Cône amovible pour optimiser la directivité
- ✅ Meilleure pénétration que les HC-SR04 classiques
- ✅ Alimentation flexible (3,3V-5V)

#### Documentation
- 📖 [Guide officiel DF Robot](https://www.dfrobot.com/product-1934.html)
- 📄 [Datasheet PDF](https://wiki.dfrobot.com/A01NYUB%20Waterproof%20Ultrasonic%20Sensor%20SKU:%20SEN0313)

</details>

![Capteur SEN0313](https://github.com/lorenzor0912/Projet-IT-Neige/blob/f1702dfe2ce56fabe681698466927644a630968b/ReadMe_IMG/SEN0313.JPG)

**Lien d'achat** : [DFRobot Store](https://www.dfrobot.com/product-1934.html) | [Distributeurs locaux](https://www.dfrobot.com/distributors.html)

---

### 🖥️ Microcontrôleur

**Option retenue** : Solution ESP tout-en-un (ESP32 recommandé)

<details>
<summary><strong>🔍 Pourquoi l'ESP32 ?</strong></summary>

#### Avantages techniques
- ✅ **WiFi & Bluetooth** intégrés
- ✅ **Faible consommation** : modes deep sleep (<10 µA)
- ✅ **Port SD natif** : SPI/SDMMC
- ✅ **UART multiples** : communication capteur simplifiée
- ✅ **Température d'opération** : -40°C à +125°C
- ✅ **Prix abordable** : <5€ en volume

#### Alternatives évaluées
| Modèle | Avantages | Inconvénients |
|--------|-----------|---------------|
| **ESP32-S3** | Plus puissant, USB natif | Consommation légèrement supérieure |
| **ESP32-C3** | RISC-V, très économe | Moins de GPIO |
| **Arduino Mega** | Simplicité | Pas de WiFi natif, consommation élevée |

#### Configuration recommandée
```
ESP32-WROOM-32D
- Flash : 4 MB
- RAM : 520 KB
- Deep Sleep : 10 µA
- GPIO : 34 pins
```

</details>

**Référence suggérée** : ESP32-DevKitC ou ESP32-WROVER pour stockage PSRAM additionnel

---

### 📡 Communications

Le système supporte plusieurs modes de transmission selon le contexte de déploiement :

#### 🌐 4G/LTE (Mode principal)
- **Module** : SIM7600E-H ou SIM800L
- **Avantages** : 
  - Couverture étendue
  - Débit élevé (upload direct vers serveur)
  - Géolocalisation GPS intégrée
- **Inconvénients** : Consommation plus élevée (~100-500 mA en transmission)

#### 📻 LoRaWAN (Mode économe)
- **Module** : RFM95W ou LILYGO T-Beam
- **Avantages** :
  - Très faible consommation
  - Portée longue distance (>10 km en terrain dégagé)
  - Pas d'abonnement cellulaire
- **Inconvénients** : 
  - Infrastructure gateway requise
  - Débit limité (faible fréquence de transmission)

#### 🛰️ Meshtastic (Mode alternatif)
- **Module** : T-Beam / Heltec LoRa
- **Avantages** :
  - Réseau maillé décentralisé
  - Relais automatique entre stations
  - Excellent pour zones isolées
- **Inconvénients** : Nécessite plusieurs nœuds pour le maillage

<details>
<summary><strong>🔧 Tableau comparatif détaillé</strong></summary>

| Critère | 4G/LTE | LoRaWAN | Meshtastic |
|---------|--------|---------|------------|
| **Portée** | 10-20 km | 2-15 km | 5-50 km (maillé) |
| **Consommation** | 🔴 Élevée (100-500 mA) | 🟢 Faible (10-50 mA) | 🟢 Faible (20-80 mA) |
| **Coût opérationnel** | 🔴 ~10€/mois | 🟢 Gratuit | 🟢 Gratuit |
| **Infrastructure** | 🟢 Existante | 🟡 Gateway requis | 🟡 Multi-nœuds |
| **Latence** | 🟢 Temps réel | 🟡 Minutes | 🟡 Variable |
| **Recommandé pour** | Zones urbaines | Zones rurales | Zones isolées |

</details>

**Recommandation** : Débuter avec 4G pour validation, migrer vers LoRaWAN pour déploiements longue durée.

---

### 💾 Stockage

**Solution** : Carte microSD haute endurance

#### 📊 Estimation de l'espace nécessaire

Hypothèses :
- Fréquence de mesure : 1 mesure toutes les 30 secondes
- Durée : 4 mois (122 jours)
- Format de données : CSV avec timestamp + distance + température

```
Calcul :
- Mesures par jour : (24 × 60 × 60) / 30 = 2,880 mesures
- Total 4 mois : 2,880 × 122 = 351,360 mesures
- Taille par entrée : ~50 octets (horodatage ISO8601 + valeurs)
- Espace total : 351,360 × 50 ≈ 17.5 MB

Recommandation : Carte SD 8 Go minimum (marge de sécurité × 400)
```

#### 🛒 Cartes SD recommandées (haute endurance)

| Modèle | Capacité | Température | Endurance | Prix (indicatif) |
|--------|----------|-------------|-----------|------------------|
| **SanDisk Industrial XI** | 8-64 Go | -40°C à +85°C | Cycle d'écriture élevé | 13-30€ |
| **SanDisk High Endurance** | 32-256 Go | -25°C à +85°C | Optimisé vidéo surveillance | 15-45€ |
| **Samsung PRO Endurance** | 32-128 Go | -25°C à +85°C | Garantie 5 ans | 18-40€ |
| **Kingston High Endurance** | 32-128 Go | -25°C à +85°C | Spécifications militaires | 16-38€ |

**Critères de sélection prioritaires** :
1. ⚡ **Plage de température** : -40°C minimum
2. 🔄 **Cycles d'écriture** : >10,000 cycles P/E
3. 🛡️ **Durabilité** : Résistance aux chocs et vibrations
4. 📜 **Garantie** : Minimum 3 ans

**Lien d'achat** : [SanDisk Industrial XI 8Go](https://www.mouser.fr/ProductDetail/SanDisk/SDSDQAF3-008G-XI?qs=F5EMLAvA7IAdyu9puKxNsg%3D%3D) (recommandé)

---

### 🔋 Alimentation

#### Stratégie d'autonomie cible : **4 mois**

<details>
<summary><strong>⚡ Calcul de consommation détaillé</strong></summary>

#### Profil de consommation

**Mode actif (mesure)** :
- ESP32 : 80 mA
- Capteur SEN0313 : 15 mA
- Module 4G (transmission) : 250 mA (pic 2A)
- Carte SD (écriture) : 50 mA
- **Total actif** : ~400 mA pendant 5 secondes

**Mode veille (deep sleep)** :
- ESP32 : 10 µA
- Capteur (désactivé) : 5 µA
- **Total veille** : ~15 µA

#### Calcul quotidien (mesure toutes les 30 sec)

```
Mesures par jour : 2,880
Temps actif : 2,880 × 5 sec = 4 heures
Temps veille : 20 heures

Consommation jour :
- Actif : 400 mA × 4 h = 1,600 mAh
- Veille : 0.015 mA × 20 h = 0.3 mAh
Total : ~1,600 mAh / jour

Consommation 4 mois :
1,600 × 122 jours = 195,200 mAh ≈ 195 Ah
```

#### Solution batterie recommandée

**Option 1 : Batterie LiFePO4** (recommandée)
- Modèle : 12V 100Ah LiFePO4
- Capacité utilisable : ~90 Ah (90% DoD)
- Avantages : 
  - ✅ Excellente performance au froid
  - ✅ Durée de vie >3000 cycles
  - ✅ Sécurité chimique supérieure
- Prix : ~200-300€

**Option 2 : Panneaux solaires + Batterie hybride**
- Panneau : 50W monocristallin
- Batterie : 12V 50Ah LiFePO4
- Contrôleur : MPPT 10A avec protection gel
- Autonomie : Illimitée (si ensoleillement >4h/jour)
- Prix total : ~400-500€

**Option 3 : Power Bank haute capacité**
- Capacité : 100,000 mAh (370 Wh)
- USB-C PD avec régulateur buck 5V
- Moins cher mais durée de vie limitée
- Prix : ~150€

</details>

**Recommandation finale** : LiFePO4 12V 100Ah avec protection thermique pour déploiements hivernaux.

---

### 🏗️ Matériaux & Boîtier

Le boîtier doit résister à des conditions extrêmes : neige, UV, humidité, températures de -30°C, pendant 10 ans.

#### 🧪 Comparatif des matériaux d'impression 3D

| Critère | ASA-CF | PETG-CF | PET | ABS | PLA | ASA (std) | PC | PETG (std) | Nylon (PA) |
|--------------------------------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------|--------------| 
| **Résistance au froid (-30°C)** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🔴🔴 | 🟢🟢 | 🟢🟢 | 🟢🟢 | 🟢🟢 |
| **Durabilité UV (10 ans ext.)** | 🟢🟢 | 🟢 | 🔴 | 🔴 | 🔴🔴 | 🟢🟢 | 🟡 | 🟡 | 🟡 |
| **Résistance à l'humidité** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🔴 | 🟢🟢 | 🟡 | 🟢🟢 | 🔴 |
| **Stabilité dimensionnelle** | 🟢🟢 | 🟢🟢 | 🟢 | 🟡 | 🔴 | 🟢🟢 | 🟢 | 🟢🟢 | 🔴 |
| **Facilité d'impression** | 🔴 | 🔴 | 🟢 | 🟡 | 🟢🟢 | 🟡 | 🔴🔴 | 🟢🟢 | 🔴 |
| **Résistance mécanique** | 🟢🟢 | 🟢🟢 | 🟢 | 🟢 | 🟡 | 🟢 | 🟢🟢 | 🟢 | 🟢🟢 |

##### Légende
- 🟢🟢 Excellent
- 🟢 Bon
- 🟡 Moyen / Conditions requises
- 🔴 Faible / Déconseillé
- 🔴🔴 Très faible / À éviter

<details>
<summary><strong>📖 Notes techniques détaillées</strong></summary>

#### Résistance au froid
- **PLA** : Devient cassant en dessous de 0°C → ❌ **À ÉVITER**
- **PC & Nylon** : Restent flexibles même à -30°C
- **ASA/ASA-CF** : Température de fléchissement >100°C

#### Durabilité UV
- **ASA & ASA-CF** : 🏆 **Meilleure résistance** (usage automobile)
  - Contient stabilisants UV dans la matrice
  - Pas de jaunissement après 5+ ans d'exposition
- **PLA, ABS, PET** : 🔴 Dégradation rapide (jaunissement, fragilisation en <1 an)
- **Nylon** : Variable selon type (PA12 > PA6)

#### Résistance à l'humidité
- **Absorption d'eau** :
  - ASA-CF : ~0.3-0.5% (excellent)
  - PETG : Très hydrophobe (~0.2%)
  - Nylon PA6 : Jusqu'à 8% ⚠️ (gonflement, perte rigidité)
  - PLA : Gonflement + perte de propriétés mécaniques
- **Conséquence** : Le nylon PA6 nécessite un séchage systématique avant impression

#### Facilité d'impression
- **Facile** 🟢 : PLA, PETG standard
  - Température plateau : 50-60°C
  - Pas d'enceinte chauffée requise
  
- **Nécessite enceinte chauffée** 🔴 : ASA, ABS, PC, Nylon
  - Température enceinte : 40-60°C
  - Plateau : 80-110°C
  - Risque de warping élevé sans enceinte
  
- **Matériaux abrasifs (CF)** 🔴 : Nécessite buse renforcée
  - Buse acier trempé ou rubis obligatoire
  - Les fibres de carbone usent rapidement les buses laiton
  
- **PC & Nylon** 🔴🔴 : 
  - Température extrême : >250°C (hotend tout métal requis)
  - Séchage obligatoire : 6-12h à 70°C avant impression
  - Chambre fermée + plateau >100°C

</details>

#### 🏆 Recommandations finales

##### ✅ TOP 3 pour usage extérieur longue durée

1. **🥇 ASA-CF** (Champion toutes catégories)
   - **Meilleur compromis** : rigidité / UV / froid / humidité
   - Idéal pour pièces structurelles exposées
   - Coût : ~40-60€/kg
   - **⚠️ Attention** : Enceinte chauffée + buse renforcée requise

2. **🥈 ASA standard** (Alternative économique)
   - Si pas besoin de renfort carbone (pièces non-contraintes)
   - Résistance UV identique à l'ASA-CF
   - Plus facile à imprimer que la version CF
   - Coût : ~25-35€/kg

3. **🥉 PETG-CF** (Compromis sans enceinte)
   - Alternative si imprimante sans enceinte chauffée
   - Bon pour pièces internes du boîtier
   - Hydrophobe excellent
   - Coût : ~35-50€/kg

##### ❌ À ÉVITER en extérieur

| Matériau | Raison principale |
|----------|-------------------|
| **PLA** | 🔴🔴 Dégradation rapide, cassant au froid, absorbe l'humidité |
| **ABS** | 🔴 Jaunissement et fragilisation aux UV en <2 ans |
| **Nylon PA6** | 🔴 Absorption d'humidité excessive (8%), instabilité dimensionnelle |

#### 🎨 Design du boîtier

**Contraintes de conception** :
- Épaisseur parois : 3-4 mm minimum
- Joints toriques : groove 2mm pour joint 3mm Viton/EPDM
- Passe-câbles : Presse-étoupe PG7 IP68
- Ventilation : Valve Gore-Tex (respiration sans infiltration)
- Montage : Fixation rail DIN + trous Ø8mm pour vis inox

**Fichiers STL disponibles** : *(à venir dans `/hardware/enclosure/`)*

---

## 💻 Software

### 🔧 Fonctionnalités prévues

- [ ] **Acquisition de données**
  - Lecture UART du capteur SEN0313
  - Timestamping précis (RTC DS3231 externe)
  - Moyennage sur N échantillons (filtrage bruit)

- [ ] **Gestion de l'énergie**
  - Deep sleep ESP32 entre mesures
  - Wake-up timer configurable
  - Surveillance batterie (ADC + diviseur pont)

- [ ] **Stockage**
  - Écriture CSV sur carte SD
  - Rotation logs automatique (fichiers journaliers)
  - Protection buffer en cas de coupure

- [ ] **Communication**
  - Envoi périodique via 4G/LoRaWAN
  - Protocole MQTT ou HTTP POST
  - Retry logic avec backoff exponentiel

- [ ] **Configuration**
  - WebServer de configuration (mode AP)
  - Paramètres : fréquence mesure, seuils alarmes, APN
  - OTA (mise à jour firmware à distance)

### 📚 Stack technique

```
- Langage : C++ (Arduino framework / ESP-IDF)
- Librairies :
  ├─ SoftwareSerial (UART capteur)
  ├─ SD (stockage)
  ├─ WiFi / TinyGSM (4G)
  ├─ ArduinoJson (sérialisation)
  ├─ PubSubClient (MQTT)
  └─ RTClib (horloge temps réel)
```

**Code source** : *(à venir dans `/software/`)*

---

## 🚀 Installation & Déploiement

### 📦 Liste du matériel nécessaire

| Composant | Quantité | Prix unitaire | Lien |
|-----------|----------|---------------|------|
| Capteur SEN0313 | 1 | ~30€ | [DFRobot](https://www.dfrobot.com/product-1934.html) |
| ESP32-DevKitC | 1 | ~5€ | [AliExpress](https://www.aliexpress.com) |
| Module 4G SIM7600 | 1 | ~25€ | [Amazon](https://www.amazon.fr) |
| Carte SD 8Go Industrial | 1 | ~13€ | [Mouser](https://www.mouser.fr) |
| Batterie LiFePO4 12V 100Ah | 1 | ~250€ | [Spécialiste batteries] |
| Boîtier ASA-CF (impression) | 1 | ~15€ (filament) | À imprimer |
| Connectique étanche | Divers | ~10€ | [RS Components](https://fr.rs-online.com) |
| **TOTAL** | - | **~348€** | - |

### 🛠️ Étapes d'assemblage

1. **Impression du boîtier** (ASA-CF)
   - Télécharger les fichiers STL
   - Paramètres : 0.2mm layer, 4 perimeters, 40% infill
   - Temps : ~18h

2. **Montage électronique**
   - Souder les connexions UART (RX/TX/GND)
   - Installer le module SD
   - Connecter le module 4G

3. **Flashage firmware**
   ```bash
   # Via Arduino IDE ou PlatformIO
   pio run -t upload
   ```

4. **Configuration initiale**
   - Connecter au point d'accès WiFi "SnowSensor-XXXX"
   - Configurer : APN, fréquence mesure, serveur distant
   - Tester une séquence de mesure

5. **Installation sur site**
   - Monter le capteur à 2-3m du sol (support vertical)
   - Orienter perpendiculaire au sol
   - Enterrer la batterie ou placer en coffre isolé
   - Activer et vérifier première transmission

**Documentation complète** : Voir `/docs/installation_guide.md`

---

## 🤝 Contribution

Les contributions sont les bienvenues ! 

### Comment contribuer ?

1. 🍴 **Fork** le projet
2. 🌿 Créer une branche (`git checkout -b feature/amelioration`)
3. 💾 Commit vos changements (`git commit -m 'Ajout fonctionnalité X'`)
4. 📤 Push vers la branche (`git push origin feature/amelioration`)
5. 🔃 Ouvrir une **Pull Request**

### Zones d'amélioration prioritaires

- [ ] Optimisation consommation énergétique
- [ ] Implémentation LoRaWAN
- [ ] Dashboard web de visualisation
- [ ] Tests en conditions réelles -30°C
- [ ] Calibration automatique du capteur

---

## 📜 Licence

Ce projet est sous licence **MIT** - voir le fichier [LICENSE](LICENSE) pour plus de détails.

---

## 📞 Contact & Support

- **Auteur** : Lorenzo R.
- **Email** : *(à compléter)*
- **Issues** : [GitHub Issues](https://github.com/lorenzor0912/Projet-IT-Neige/issues)
- **Discussions** : [GitHub Discussions](https://github.com/lorenzor0912/Projet-IT-Neige/discussions)

---

<div align="center">
  
### 🏔️ Développé avec ❄️ par Sti2D Labs

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
  alt="Logo Sti2D Labs" 
  width="600" 
  height="600" />

<p align="center">
  <strong>⭐ Si ce projet vous plaît, n'hésitez pas à lui donner une étoile ! ⭐</strong>
</p>

</div>

<div align="right">
  <a href="#top">⬆️ Retour en haut</a>
</div>
