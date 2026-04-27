# ⚡ Bilan Énergétique et Profil de Consommation (Edge AI)

Ce document détaille l'analyse énergétique du projet **Oracle Météo**, une station météorologique intelligente basée sur l'Edge AI (carte STM32). L'objectif est de démontrer la viabilité énergétique du déploiement d'un réseau de neurones convolutif quantifié (int8) sur un système embarqué contraint, fonctionnant potentiellement sur batterie.

---

## 1. Profil Énergétique Théorique (Duty Cycle)

Le système repose sur un fonctionnement cyclique (Duty Cycle) avec une mesure et une inférence toutes les **15 minutes (900 secondes)**. Le profil se divise en deux états distincts : le mode Actif et le mode Veille.

### A. Mode Actif (Réveil, Mesure, IA, Envoi)
La carte se réveille pour acquérir les données, calculer la prédiction et l'envoyer.
* **Courant moyen estimé :** `~40 mA`
* **Durée totale d'activité :** `~100 ms (0,1 s)`
* **Détail du temps de traitement :**
  * Réveil système (Horloge/Oscillateur) : `~2 ms`
  * Lecture des capteurs I2C (HTS221 / LPS22HH) : `~18 ms`
  * **Inférence IA (CNN int8 via X-CUBE-AI)** : `~60 ms`
  * Envoi des données (Série / LoRa) : `~20 ms`

*Justification :* Les capteurs environnementaux "Ultra-Low Power" consomment moins de 20 µA. La quasi-totalité des 40 mA est absorbée par le CPU (ex: ARM Cortex) tournant à pleine fréquence (ex: 80 MHz) pour exécuter les multiplications matricielles. La quantification en `int8` permet de maintenir l'inférence sous la barre des 100 ms.

### B. Mode Veille (Sommeil profond)
Entre deux cycles, le système est placé en mode basse consommation (ex: mode `Stop 2` ou `Standby` sur STM32).
* **Courant moyen estimé :** `~5 µA`
* **Durée de veille :** `899,9 secondes` (15 min - 0,1 s)
* **Détail :**
  * CPU : Éteint (Rétention RAM active).
  * Périphériques : Éteints.
  * Horloge RTC (Real Time Clock) : Active `~3 µA` pour le réveil planifié.
  * Capteurs : Mode Power-Down `~2 µA`.

---

## 2. Bilan Électrique et Consommation Moyenne

Pour évaluer l'impact réel de l'IA, nous raisonnons en charge électrique (milliampères-secondes, **mAs**) par cycle de 15 minutes.

1. **Charge de l'Inférence IA seule :** `40 mA × 0,06 s = 2,4 mAs`
2. **Charge des périphériques (Capteurs + Radio) :** `40 mA × 0,04 s = 1,6 mAs`
3. **Charge du mode Veille :** `0,005 mA × 899,9 s = 4,5 mAs`

* **Charge Totale par cycle :** `8,5 mAs`
* **Consommation Courant Moyen :** `8,5 mAs / 900 s = 0,0094 mA` soit **9,4 µA**

**Conclusion Théorique :** Avec un courant moyen inférieur à 10 µA, le calcul de l'IA (qui représente 28% de la charge du cycle) n'est pas un frein à l'autonomie. Sur une batterie standard de type LiPo (ex: 2000 mAh), l'autonomie théorique dépasse largement la durée de vie chimique de la batterie (estimée à 5-10 ans par autodécharge). Le système Edge AI est donc parfaitement soutenable.

---

## 3. Protocole de Mesure Expérimentale

Pour valider ces valeurs théoriques sur banc d'essai, l'utilisation d'un multimètre classique est proscrite pour le mode Actif, car son taux de rafraîchissement est trop lent pour capturer un pic de 100 ms.

Voici les deux méthodes recommandées pour auditer ce système :

### Méthode 1 : Profiling dynamique à l'Oscilloscope (Résistance Shunt)
C'est la méthode de référence pour capturer le profil d'inférence de l'IA.
1. **Montage :** Insérer une résistance très précise de faible valeur (ex: `10 Ohms`, 1%, appelée "Shunt") en série sur la ligne d'alimentation `3.3V` de la STM32.
2. **Mesure :** Placer la sonde d'un oscilloscope aux bornes de cette résistance.
3. **Valeurs attendues :**
   * En veille : Une tension quasi nulle (`< 0,1 mV`).
   * Au réveil : Un pic de tension carré. Par la loi d'Ohm ($U = R 	imes I$), pour une consommation attendue de 40 mA, on doit observer un palier à **`400 mV`**.
   * On peut mesurer précisément la largeur de ce pic sur l'écran pour vérifier que l'exécution de l'IA dure bien **`~60 ms`**.

### Méthode 2 : Analyseur de puissance dédié (ST Power Shield)
Pour une mesure absolue de la charge en mAs :
1. **Matériel :** Utiliser une carte d'acquisition dédiée de type *STMicroelectronics X-NUCLEO-LPM01A* (ou un analyseur Joulescope).
2. **Logiciel :** Utiliser *STM32CubeMonitor-Power* sur PC.
3. **Protocole :** Alimenter la cible via le Power Shield à 3.3V. Lancer une acquisition à 100 kHz (100 000 échantillons/seconde) sur un cycle complet de 15 minutes.
4. **Validation :** Le logiciel calculera l'intégrale de la courbe et devra confirmer une consommation moyenne de l'ordre de **10 µA** par cycle.

---
*Ce rapport d'ingénierie démontre que la quantification int8 couplée à une gestion stricte des modes de sommeil permet de réaliser de la prédiction météorologique par Intelligence Artificielle sans compromettre les exigences d'un réseau IoT ultra-basse consommation.*
