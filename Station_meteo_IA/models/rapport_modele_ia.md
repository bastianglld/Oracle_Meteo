# 🧠 Rapport d'Architecture et d'Analyse du Modèle IA (Oracle Météo)

Ce document détaille la conception, l'architecture et l'optimisation du réseau de neurones embarqué au cœur du projet **Oracle Météo**. L'objectif de ce modèle est de réaliser des prédictions météorologiques locales sur 5 jours, directement sur un microcontrôleur STM32 (Edge AI), sans dépendre d'un calcul dans le Cloud.

---

## 1. Objectif et Approche

Le modèle est conçu pour analyser une série temporelle courte (les données météorologiques locales des 24 dernières heures) afin d'en déduire une tendance à moyen terme (les 5 prochains jours).
Il s'agit d'un problème d'apprentissage supervisé mixte : 
* **Régression** pour la température, l'humidité et la pression.
* **Classification binaire** pour la prédiction des précipitations (Pluie / Pas de pluie).

---

## 2. Structure des Données (Entrées / Sorties)

### 📥 Entrées (Inputs)
Le modèle prend en entrée un tenseur tridimensionnel représentant une fenêtre glissante temporelle.
* **Format :** `[1, 24, 3]` (Batch_size, Sequence_length, Features)
* **Profondeur temporelle :** 24 heures (1 mesure par heure).
* **Variables (Features) :** 1. Température (°C)
  2. Humidité Relative (%)
  3. Pression Atmosphérique (hPa)
* *Note :* Les données sont normalisées entre 0 et 1 (MinMaxScaler) avant d'être fournies au modèle.

### 📤 Sorties (Outputs)
Le modèle prédit un vecteur plat contenant les prévisions pour les 5 jours à venir.
* **Format :** `[1, 20]` (5 jours × 4 paramètres)
* **Variables pour chaque jour (J+1 à J+5) :**
  1. Température moyenne
  2. Humidité moyenne
  3. Pression moyenne
  4. Probabilité de précipitations (> 0.5 = Pluie)

---

## 3. Architecture du Réseau de Neurones

Face aux contraintes de mémoire (RAM/Flash) du microcontrôleur STM32, l'architecture choisie est un **Réseau de Neurones Convolutif à une dimension (CNN 1D)**. Les CNN 1D sont excellents pour extraire des motifs temporels (cycles jour/nuit, baisses de pression) tout en nécessitant beaucoup moins de paramètres qu'un réseau récurrent (LSTM/GRU).

**Topologie du modèle (Keras/TensorFlow) :**
1. **Input Layer :** `(24, 3)`
2. **Conv1D :** 32 filtres, taille de noyau (kernel) = 3, activation = `ReLU`. *(Extraction des micro-variations).*
3. **MaxPooling1D :** Taille = 2. *(Réduction de la dimensionnalité temporelle, garde les signaux les plus forts).*
4. **Conv1D :** 16 filtres, taille de noyau = 3, activation = `ReLU`. *(Extraction des tendances macroscopiques).*
5. **Flatten :** Aplatissement du tenseur pour la transition vers les couches denses.
6. **Dense (Hidden) :** 64 neurones, activation = `ReLU`.
7. **Dense (Output) :** 20 neurones, activation = `Sigmoid`. *(La fonction Sigmoïde contraint les sorties entre 0 et 1, ce qui correspond parfaitement à nos données dé-normalisables et à nos probabilités de pluie).*

---

## 4. Optimisation pour l'Embarqué : La Quantification Int8

Le fichier final exporté est `meteostat_cnn_5jours_int8.tflite`. Le suffixe **int8** est la clé de voûte de notre architecture matérielle.

### Qu'est-ce que la quantification post-entraînement (PTQ) ?
Lors de l'entraînement sur PC, les poids du modèle sont encodés en virgule flottante sur 32 bits (`float32`). Lors de l'exportation vers TensorFlow Lite, nous avons appliqué une conversion pour mapper ces nombres complexes vers des nombres entiers sur 8 bits (`int8`, de -128 à 127).

### Les 3 avantages majeurs pour la STM32 :
1. **Empreinte mémoire divisée par 4 :** Un paramètre passe de 4 octets à 1 octet. Le modèle rentre facilement dans la mémoire Flash de la carte.
2. **Accélération matérielle :** L'Unité Logique et Arithmétique (ALU) du processeur ARM Cortex calcule des multiplications d'entiers beaucoup plus vite que des nombres à virgule. Le temps d'inférence est mesuré à **~60 millisecondes**.
3. **Efficacité énergétique :** Des calculs plus rapides signifient que la carte peut retourner en mode "Veille" (Sleep) plus rapidement, préservant la batterie.

### Paramètres de Calibration :
Pour que la carte C sache comment interpréter ces "entiers", l'interpréteur TFLite génère des échelles de conversion.
Exemple d'intégration logicielle (Dénormalisation de la couche d'entrée) :
`Valeur_Float = (Valeur_Int8 - Zero_Point) * Scale`

---

## 5. Fiabilité et Évaluation (Métriques)

L'audit du modèle sur un jeu de données de validation (20% des données, jamais vues lors de l'entraînement) démontre la viabilité de l'approche :

* **Stabilité de l'apprentissage :** La courbe de perte (MSE) montre que la perte de validation suit étroitement la perte d'entraînement. **Absence d'overfitting** : le modèle a appris la "physique" de la météo, il ne l'a pas apprise par cœur.
* **Corrélation Temporelle :** Sur l'échantillon test, le modèle anticipe correctement les cycles diurnes et nocturnes, ainsi que les baisses soudaines liées aux fronts froids, avec une marge d'erreur faible (environ 1 à 2°C).
* **Classification (Matrice de Confusion) :** La prédiction des précipitations, très complexe avec peu de capteurs, montre une grande fiabilité pour affirmer l'absence de pluie (True Negatives importants).

## 6. Conclusion

Le modèle `meteostat_cnn_5jours_int8.tflite` prouve qu'il est possible de compresser la compréhension d'un système chaotique (la météorologie locale) dans un fichier de quelques kilo-octets. Couplé à l'outil X-CUBE-AI de STMicroelectronics, ce réseau de neurones quantifié remplit parfaitement le cahier des charges d'un système **Edge AI, IoT et basse consommation**.
