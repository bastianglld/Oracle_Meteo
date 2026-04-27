import serial
import requests
import re
import time

# --- CONFIGURATION DES CLES API ---
API_KEY_TEMP = "5PH31ZBO1VEMGZFL"   # Channel 1 : Live + Températures
API_KEY_PLUIE = "Z952E8MUGPKMR9BI"  # Channel 2 : Pluie uniquement

PORT_SERIE = "/dev/cu.usbmodem102"
BAUD_RATE = 115200

try:
    ser = serial.Serial(PORT_SERIE, BAUD_RATE)
    print(f"✅ Connecté à la STM32 sur le port {PORT_SERIE}")
except Exception as e:
    print(f"❌ Erreur de connexion au port série: {e}")
    exit()

print("📡 En écoute (Double envoi vers ThingSpeak toutes les 15s)...")

payload_temp = {}
payload_pluie = {}
dernier_envoi = 0

while True:
    try:
        ligne = ser.readline().decode('utf-8', errors='ignore').strip()
        
        # 1. Capture du LIVE -> Channel 1
        if "[LIVE]" in ligne:
            # On extrait Température (1), Humidité (2) et Pression (3)
            match = re.search(r"T:\s*([\d.-]+)\s*C\s*\|\s*H:\s*([\d.-]+)\s*%\s*\|\s*P:\s*([\d.-]+)\s*hPa", ligne)
            if match:
                payload_temp['field1'] = match.group(1) # Température
                payload_temp['field2'] = match.group(3) # Pression (Field 2 sur ton interface)
                payload_temp['field3'] = match.group(2) # Humidité (Field 3 sur ton interface)

        # 2. Capture des prévisions J+1 à J+5
        elif "J+" in ligne:
            match = re.search(r"J\+(\d+)\s*\|\s*T:\s*([\d.-]+)C.*?Pluie:\s*([\d.-]+)%", ligne)
            
            if match:
                jour = int(match.group(1))
                temp = match.group(2)
                pluie = match.group(3)
                
                # Channel 1 : Fields 4 à 8 pour les températures prévues
                payload_temp[f'field{jour + 3}'] = temp
                
                # Channel 2 : Fields 1 à 5 pour la pluie prévue
                payload_pluie[f'field{jour}'] = pluie

                # 3. Déclencheur d'envoi quand on arrive à J+5
                if jour == 5:
                    temps_actuel = time.time()
                    
                    # On respecte la limite de 15 secondes de ThingSpeak
                    if temps_actuel - dernier_envoi >= 15:
                        
                        # --- Envoi Channel 1 ---
                        url_temp = f"https://api.thingspeak.com/update?api_key={API_KEY_TEMP}"
                        for field, value in payload_temp.items():
                            url_temp += f"&{field}={value}"
                        
                        # --- Envoi Channel 2 ---
                        url_pluie = f"https://api.thingspeak.com/update?api_key={API_KEY_PLUIE}"
                        for field, value in payload_pluie.items():
                            url_pluie += f"&{field}={value}"
                        
                        print("\n🚀 Envoi en cours...")
                        
                        req1 = requests.get(url_temp)
                        if req1.status_code == 200:
                            print(f"✅ Ch.1 (Temp/Press/Hum) OK ! (ID: {req1.text})")
                        else:
                            print(f"❌ Erreur Ch.1 : {req1.status_code}")
                            
                        req2 = requests.get(url_pluie)
                        if req2.status_code == 200:
                            print(f"✅ Ch.2 (Pluie) OK ! (ID: {req2.text})")
                        else:
                            print(f"❌ Erreur Ch.2 : {req2.status_code}")
                        
                        dernier_envoi = temps_actuel
                    
                    # On réinitialise pour la prochaine lecture
                    payload_temp = {}
                    payload_pluie = {}

    except KeyboardInterrupt:
        print("\nArrêt du pont série.")
        ser.close()
        break