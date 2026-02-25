# 7-Segment Clock (ESP8266) Stream Deck Controller

Ein Elgato Stream Deck Plugin zur Steuerung einer ESP8266-basierten 7-Segment-Uhr über das Netzwerk (IP).

## Funktionen

Dieses Plugin bietet verschiedene Aktionen, um die 7-Segment-Uhr direkt über das Stream Deck zu steuern:

- **Clock Color (Uhrfarbe):** Ändert die Farbe der 7-Segment-LED-Anzeige.
- **Clock Transition (Übergang):** Setzt die Übergangszeit und die Ziel-Uhrzeit für die Uhr.
- **Set Clock Time (Uhrzeit festlegen):** Setzt die Uhrzeit direkt auf einen bestimmten Wert.
- **Stop Clock (Uhr stoppen):** Stoppt die aktuelle Funktion der Uhr.
- **Start Clock (Uhr starten):** Startet den normalen Betrieb der Uhr.

## Globale IP-Einstellungen

Das Plugin unterstützt die Konfiguration einer globalen IP-Adresse. Das bedeutet, dass die Ziel-IP-Adresse der zu steuernden Uhr zentral festgelegt werden kann. Darüber hinaus besteht optional die Möglichkeit, bei den einzelnen Aktionen eine abweichende IP-Adresse festzulegen, um mehrere Uhren (Multi-Clock-Support) unabhängig voneinander zu steuern.

## Systemanforderungen

- **Stream Deck Software:** Mindestens Version 6.4
- **Betriebssystem:** macOS 10.15+ oder Windows 10+

## Entwicklung und Build

Dieses Projekt basiert auf Node.js (v20) und wird mit TypeScript entwickelt.

```bash
# In den Plugin-Ordner wechseln
cd 7segmentclockcontroller

# Abhängigkeiten installieren
npm install

# Plugin kompilieren (Build)
npm run build

# Im Entwicklungsmodus starten (automatischer Neustart bei Code-Änderungen)
npm run watch
```