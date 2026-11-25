# 20 Minučių iki Paskaitos

Procedūrinio programavimo namų darbo projektas (**C++17 + SFML 3.0.2**, griežtai procedūrinė architektūra: `struct` + laisvos funkcijos).

## Apie žaidimą

Tai 2D *top-down* arkadinis vairavimo žaidimas, kuriame **esi dėstytojo pavežėjas** ir turi nuvežti jį per chaotišką Vilniaus eismą į universitetą neišsekinus dėstytojo kantrybės ir neviršijus pasirinkto maršruto vėlavimo ribos.

### Maršrutai
1. **Antakalnis → Saulėtekis** (Lengvas – 3.0 km, riba 8 min.)
2. **Centras → Saulėtekis** (Vidutinis – 5.0 km, riba 10 min.)
3. **Pašilaičiai → Saulėtekis** (Sunkus / piko valanda – 7.0 km, riba 12 min.)

### Eismo dalyviai ir bonusai
- **Tavo automobilis (žalias pavežėjas)** – šalia jo kalbos burbule rodomi gyvi dėstytojo komentarai ir emocijos.
- **Įprasti automobiliai** – važiuoja savo eismo juostomis.
- **Sportiniai automobiliai (`ChaoticDriver`)** – pypia, mirksi posūkiu ir staigiai persirikiuoja.
- **Vilniaus troleibusai (`Trolleybus`)** – ilgi ir lėti, užima daug vietos.
- **Paspirtukininkai (`Scooter`)** – siauri, greiti ir vingiuoja juostoje.
- **Kelio darbai (`Roadworks`)** – stacionarios oranžinės užtvaros.
- **Bonusai:**
  - **Kava** – +1 dėstytojo kantrybės (arba +150 tšk.) ir `-0.5 min` vėlavimo.
  - **Žalia banga** – 3.5 s turbo greitis ir laikinas imunitetas avarijoms.
  - **USB su skaidrėmis** – +300 taškų.

## Valdymas

- `A` / `D` arba `←` / `→` – juostų keitimas
- `W` / `S` arba `↑` / `↓` – greitinimas / stabdymas
- `P` arba `Esc` – pauzė
- `M` – įjungti / išjungti garsą ir muziką
- `1` / `2` / `3`, `Enter`, pelė – meniu, maršrutų ir rekordų naršymas

## Projekto struktūra (procedūrinė architektūra)

```text
sfml-game/
├── assets/
│   ├── audio/                 # Garso efektai (.wav) ir foninė muzika
│   ├── fonts/                 # CascadiaCode.ttf (lietuviškos raidės)
│   └── textures/              # Automobilių, troleibuso, paspirtuko, bonusų ir dėstytojo portretų PNG
├── src/
│   ├── types.hpp              # Konstantos, enum class būsenos ir struct duomenys
│   ├── resources.hpp/.cpp     # Šrifto, tekstūrų ir garsų įkėlimas (su atsarginiu piešimu)
│   ├── effects.hpp/.cpp       # Susidūrimo/bonusų dalelės ir ekrano drebėjimas
│   ├── storage.hpp/.cpp       # Rekordų išsaugojimas faile ir vardo įvesties validacija
│   ├── game_logic.hpp/.cpp    # Įvestis, maršrutai, judėjimas, kliūtys, bonusai, susidūrimai
│   ├── renderer.hpp/.cpp      # Kelio, sprites, dalelių, HUD, dėstytojo burbulo ir meniu piešimas
│   └── main.cpp               # Pagrindinis ciklas
├── CMakeLists.txt             # CMake konfigūracija (automatiškai parsiunčia SFML 3.0.2)
├── CMakePresets.json          # x64-debug ir x64-release nustatymai
├── build.bat                  # Kompiliavimo skriptas
└── run.bat                    # Kompiliavimo ir paleidimo skriptas
```

## Kompiliavimas ir paleidimas

### 1 būdas: Per skriptą (Windows + Visual Studio C++ įrankiai)
- Paleiskite **`run.bat`** (arba terminale `.\run.bat`).
- Release versijai:
  ```bat
  run.bat x64-release
  ```

### 2 būdas: Visual Studio
1. Visual Studio → **Open a local folder** → pasirinkite `sfml-game`.
2. Visual Studio automatiškai sukonfigūruos `CMakePresets.json` ir SFML.
3. Spauskite **F5** (paleisti `SFMLGame.exe`).
