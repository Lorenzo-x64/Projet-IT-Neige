Pour ce projet, on va partir sur du **LoRa** (technologie radio longue portée basse consommation) combiné à **Meshtastic**, pour créer un réseau maillé décentralisé, autonome et sans dépendance à internet ni aux opérateurs mobiles.

En clair : LoRa pour la portée et la fiabilité et surtout la conso, Meshtastic pour le firmware open-source qui gère le mesh, les messages texte, la localisation et les alertes entre les nœuds.


**Attention le projet Meshtastic n'accepte pas toutes les fréquences LoRa.**

- LoRa classique (comme LoRaWAN) peut fonctionner sur **8 bandes différentes** : 433 MHz, 868 MHz (Europe), 915 MHz (Amérique), 2.4 GHz, etc.

- **Meshtastic, lui, n'utilise que certaines fréquences précises**, selon ta région :  
  – Europe → **uniquement 863-870 MHz** (bande EU_868)  
  – USA/Canada/Australie → **uniquement 902-928 MHz** (bande US_915)  
  – Certains pays (Chine, NZ, etc.) → 433 MHz ou 2.4 GHz

Donc :  
Même si une puce LoRa est techniquement capable de faire 433, 868, 915 ou 2.4 GHz…  
**Meshtastic la bloquera ou ne la supportera pas si tu n'es pas dans la bonne bande légale de ton pays.**

En résumé :  
LoRa = toutes les fréquences possibles  
Meshtastic = **seulement la fréquence autorisée dans ton pays** (et il refuse les autres pour rester légal).

|Région Meshtastic|Bande principale|Fréquence autorisée|Fréquence par défaut (reset)|Puissance max|Duty cycle|Utilisation en France|
|---|---|---|---|---|---|---|
|**EU_868**|868 MHz|869.4 – 869.65 MHz|869.525 MHz|+27 dBm (500 mW ERP)|10 % par heure|**Très recommandée** – La plus utilisée, meilleure portée, réseau le plus actif|
|**EU_433**|433 MHz|433.0 – 434.0 MHz|433.875 MHz (slot 4)|+12 dBm (16 mW)|10 % par heure|Possible, mais **moins utilisée** – Portée plus courte, peu de nœuds|

Différentes cartes retenues

https://www.passion-radio.fr/ordinateur-miniature/techo-2644.html
TTGO T-Echo un peu chère module multi-GNSS L76K, qui supporte GPS (intégré) mais pas de port micro sd et pas de gpio sur cet donc éliminée car impossible de mettre carte sd 

https://www.passion-radio.fr/materiel-wifi/tbeam-sx1262-2823.html
ok gps pas de micro sd donc module externe comme https://www.waveshare.com/micro-sd-storage-board.htm ou similaire

https://www.passion-radio.fr/module/ttgo-sx1276-2419.html
oui gps non carte sd meme qu'au dessus ATTENTION CARTE VIELLISSANTE MESHTASTIC CONSEILLE MODELE PLUS RECENT

pur finir heltec v4 https:

[DFRobot](https://www.amazon.fr/-/en/dp/B0G24ZJ75L?crid=ID44N7A8V0U0&dib=eyJ2IjoiMSJ9.cLUM8BE_x4wBvMF4vnH998QRvddFub9LF_07_KiRJfnx5brVJxHmIlbKEIJShKAdK1d7EJ_VCMVHbZUsFotjPPV39WEFSPRTWqk24WuwVoC8i0b-0a3K9z1FUOK1fvDXTtWw055N5W9gXOK2GiMacn5QTxIM0fZpMJ9SG-mICtnsVS7c_bY35lLomO1M_Jo9j2XyTqIG2ucwgTN4wglz8lfCKSMAn5_Oh-yUJZfeqdGOq4BUYyivlL0ibzgqJzvbjpZiqijQVwQGihjoF1fMK8L7UczH1_sohhRMCvH0lzc.0s1PGfpkV6Ep6MG-BXGkwLOeOFvzfxw2zLzfgexDOe8&dib_tag=se&keywords=heltec+v4+868&qid=1770144402&sprefix=heltec+v4+868%2Caps%2C235&sr=8-1)

(a noter manque gps et carte sd)


Pour le capteur principal  https://www.dfrobot.com/product-1934.html

carte sd Carte SD 8Go Industrial -40 85 deg


module gps si besoin ref: u-blox NEO-6M (très courant) une vightaine d'euro
  - precision extreme : Module GNSS 5 click MIKROE2670 pour géolocalisation précise
