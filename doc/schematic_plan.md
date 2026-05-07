# Schematic Plan: supply.kicad_sch

## Bereits erledigt ✅

- TP4056 TEMP-Pin → VCC (war fälschlich auf GND → Laden dauerhaft deaktiviert)
- TP4056 EPAD → GND
- CC1/CC2 (USB-C) → 5.1kΩ Pull-downs (R15, R16)
- AMS1117 entfernt → AP2112K-3.3 (U12, SOT-23-5) eingesetzt
- D5 + D6 (SS14) platziert, VBAT-Labels gesetzt, AP2112K EN→VIN verdrahtet
- Schaltplan verifiziert: Dioden-OR-Topologie korrekt

---

## Noch zu tun: D5 durch LTC4412 + SI2305 ersetzen

**Problem:** D5 (SS14, Vf≈0.3V) im Batteriepfad lässt VPWR auf 3.4V fallen → AP2112K braucht ≥3.55V.  
**Lösung:** D5 entfernen, LTC4412 (U13) + SI2305 (Q1) einsetzen → nur ~20mV Drop.

### Schritt 1 — D5 löschen

In KiCad: D5 (SS14, Batteriepfad, y=19.05) löschen inkl. angeschlossene Drähte.

### Schritt 2 — SI2305 (Q1) platzieren

- Bauteil: `Device:Q_PMOS_GSD` oder passendes SOT-23 P-MOSFET Symbol
- Verdrahtung:

| Pin | Verbindung |
|---|---|
| Source | VBAT-Netz |
| Drain | VPWR-Knoten (wo vorher D5-Kathode war) |
| Gate | LTC4412 GATE-Pin |

### Schritt 3 — LTC4412 (U13) platzieren

- Package: TSOT-23-6 (Aufdruck auf Bauteil: **LTA2**)
- Verdrahtung:

| LTC4412 Pin | Verbindung |
|---|---|
| VIN | VBAT |
| SENSE | VBAT (direkt an VIN, kein Shunt-Widerstand) |
| GATE | SI2305 Gate |
| CTL | VIN (dauerhaft aktiv, kein Shutdown) |
| GND | GND |
| STAT | offen lassen oder Pull-up zu VIN (optional) |

### Schritt 4 — ERC prüfen

- KiCad → Tools → Electrical Rules Checker
- Sicherstellen: kein floating Pin, VBAT-Netz durchgehend verbunden

---

## Topologie-Übersicht

```
USB 5V ──── D6 (SS14, Vf≈0.3V) ─────────────────────┐
                                                      ├──> VPWR ──> AP2112K-3.3 ──> +3.3V
VBAT ──── LTC4412 (U13) + SI2305 (Q1) (Vf≈20mV) ───┘
  │
  └──── TP4056 (lädt LiPo wenn USB angeschlossen)
```

- **USB angeschlossen:** D6 leitet (VPWR ≈ 4.7V), SI2305 wird durch LTC4412 gesperrt
- **Kein USB:** SI2305 leitet (VPWR ≈ VBAT − 20mV ≈ 3.68V), D6 gesperrt

---

## Bauteilreferenz

| Ref | Bauteil | Package | Hinweis |
|---|---|---|---|
| U5 | TP4056-42-ESOP8 | ESOP-8 | LiPo-Lader, bereits vorhanden |
| D6 | SS14 | SMA | USB-Pfad, bleibt unverändert |
| U12 | AP2112K-3.3 | SOT-23-5 | LDO 3.3V, bereits gesetzt |
| U13 | LTC4412ES6 | TSOT-23-6 | Aufdruck: **LTA2**, vorhanden |
| Q1 | SI2305 | SOT-23 | **20V-Variante (MCC)**, nicht Vishay Si2305CDS (8V)! |

---

## Datasheets (in doc/)

- `ltc4412.pdf` — LTC4412 PowerPath Controller
- `SI2305(SOT-23).pdf` — SI2305 P-MOSFET
- `TP4056.pdf` — LiPo Charger
