# Diplomová práce – Použití EMG pro řízení kolaborativního robota

Tento repozitář obsahuje materiály k diplomové práci na téma **"Použití EMG pro řízení kolaborativního robota"**. Projekt se zaměřuje na využití elektromiografických (EMG) signálů pro ovládání kolaborativního robota **ABB GoFa CRB 15000**.

## Cíl projektu

Cílem je vytvořit systém schopný detekovat svalovou aktivitu pomocí EMG senzoru a na základě zpracovaných dat řídit robota. Klíčové části systému:

- Získávání EMG signálu pomocí senzoru připojeného k Arduinu
- Zpracování signálu a vyhodnocení úrovně aktivity
- Přenos dat pomocí TCP/IP protokolu
- Ovládání robota v prostoru (3 DOF)

## Použité komponenty

- **Arduino UNO**
- **Grove Base Shield v2**
- **EMG senzor** (Grove EMG Sensor)
- **Ethernet modul** pro TCP/IP komunikaci
- **ABB GoFa CRB 15000** – kolaborativní robot

## Komunikace

Arduino funguje jako **TCP server**, který zpracovává EMG signály a podle nich vrací hodnoty aktivity. Robot ABB GoFa běží jako **TCP klient**, který se pravidelně dotazuje na hodnoty a reaguje změnou své pozice v prostoru.

## Licence

Tento projekt je zveřejněn pod licencí GNU GPL v3.
