# Capteur d'enneigement Matériel évalué ❄️🌨️

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
    <img src="https://img.shields.io/badge/Étanchéité-IP66-lightblue?style=flat-square" alt="IP66"/>
  </p>
</div>

---

### Différence entre LoRa et Meshtastic

Bien que LoRa puisse techniquement fonctionner sur 8 bandes différentes (433 MHz, 868 MHz, 915 MHz, 2.4 GHz, etc.), **Meshtastic n'autorise que certaines fréquences spécifiques selon la région** pour rester conforme à la législation locale.

**Récapitulatif par région :**
- Europe : uniquement 863-870 MHz (bande EU_868)
- USA/Canada/Australie : uniquement 902-928 MHz (bande US_915)
- Certains pays (Chine, NZ, etc.) : 433 MHz ou 2.4 GHz

**Important :** Même si une puce LoRa est techniquement capable de fonctionner sur plusieurs fréquences, Meshtastic la bloquera si elle n'est pas dans la bande légale du pays.

### Bandes utilisables en France

| Région Meshtastic | Bande principale | Fréquence autorisée | Fréquence par défaut (reset) | Duty cycle | Utilisation en France |
|---|---|---|---|---|---|
| **EU_868** | 868 MHz | 869.4 – 869.65 MHz | 869.525 MHz | +27 dBm (500 mW ERP) | **Très recommandée** – La plus utilisée, meilleure portée, réseau le plus actif |
| **EU_433** | 433 MHz | 433.0 – 434.0 MHz | 433.875 MHz (slot 4) | +12 dBm (16 mW) | Possible, mais **moins utilisée** – Portée plus courte, peu de nœuds |

---

## Matériel évalué

### Cartes LoRa compatibles Meshtastic

#### ~~TTGO T-Echo~~
- Lien : https://www.passion-radio.fr/ordinateur-miniature/techo-2644.html
- GPS : Oui (module multi-GNSS L76K intégré)
- Carte micro SD : Non
- GPIO disponibles : Non
- **Statut : Éliminée** (impossible d'ajouter une carte SD)

#### T-Beam SX1262
- Lien : https://www.passion-radio.fr/materiel-wifi/tbeam-sx1262-2823.html
- GPS : Oui
- Carte micro SD : Non (nécessite module externe comme https://www.waveshare.com/micro-sd-storage-board.htm)
- **Statut : Retenue** (avec module SD externe)

#### ~~TTGO LoRa32 SX1276~~
- Lien : https://www.passion-radio.fr/module/ttgo-sx1276-2419.html
- GPS : Oui
- Carte micro SD : Non (nécessite module externe)
- **Attention : Carte vieillissante**, Meshtastic conseille un modèle plus récent
- **Statut : Déconseillée**

#### Heltec V4
- Lien : https://www.amazon.fr/-/en/dp/B0G24ZJ75L?crid=ID44N7A8V0U0&dib=eyJ2IjoiMSJ9.cLUM8BE_x4wBvMF4vnH998QRvddFub9LF_07_KiRJfnx5brVJxHmIlbKEIJShKAdK1d7EJ_VCMVHbZUsFotjPPV39WEFSPRTWqk24WuwVoC8i0b-0a3K9z1FUOK1fvDXTtWw055N5W9gXOK2GiMacn5QTxIM0fZpMJ9SG-mICtnsVS7c_bY35lLomO1M_Jo9j2XyTqIG2ucwgTN4wglz8lfCKSMAn5_Oh-yUJZfeqdGOq4BUYyivlL0ibzgqJzvbjpZiqijQVwQGihjoF1fMK8L7UczH1_sohhRMCvH0lzc.0s1PGfpkV6Ep6MG-BXGkwLOeOFvzfxw2zLzfgexDOe8&dib_tag=se&keywords=heltec+v4+868&qid=1770144402&sprefix=heltec+v4+868%2Caps%2C235&sr=8-1
- GPS : Non
- Carte micro SD : Non
- **Statut : Retenue** (nécessite ajout GPS et carte SD)

#### ~~Heltec Mesh Node T114~~
- Lien : https://heltec.org/project/mesh-node-t114/
- Puce : Nordic (très basse consommation)
- **Statut : Très difficile à trouver**

---

## Composants additionnels

### Capteurs
- Capteur principal : https://www.dfrobot.com/product-1934.html

### Stockage
- Carte SD 8 Go SanDisk Industrial (-40°C à +85°C)

### Modules GPS

#### Option standard
- Modèle : u-blox NEO-6M (très courant)
- Prix : environ 20 euros

#### Option haute précision
- Modèle : Module GNSS 5 click MIKROE2670
- Usage : Géolocalisation précise
