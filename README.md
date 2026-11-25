# 20 Minučių iki Paskaitos

Procedūrinio programavimo namų darbo projektas (C++17 + SFML 3.0.2).

## Apie žaidimą

Tai 2D *top-down* arkadinis vairavimo žaidimas, kuriame **esi dėstytojo Arto pavežėjas** ir turi nuvežti jį **5.0 km** per chaotišką miesto eismą į universitetą neišsekinus Arto kantrybės ir neviršijus **10 minučių** vėlavimo ribos.

- **Tavo automobilis (žalias pavežėjas)** – saugiai manevruok tarp kitų eismo dalyvių (Arto komentarai rodomi kalbos burbule šalia automobilio).
- **Įprasti automobiliai** – juda savo eismo juostomis.
- **Skubantys chuliganai (tamsūs sportiniai automobiliai)** – įjungia posūkio signalą ir staigiai persirikiuoja į gretimą juostą.
- **Kelio darbai (oranžinės užtvaros)** – stacionarios kliūtys kelyje.
- **Kavos puodeliai** – nudžiugina gale sėdintį Artą (+1 kantrybės taškas, +150 tšk., `-0.5 min` vėlavimo).

## Valdymas

- `A` / `D` arba `←` / `→` – judėjimas į kairę / dešinę (juostų keitimas)
- `W` / `S` arba `↑` / `↓` – greitinimas / stabdymas
- `P` arba `Esc` – pauzė
- `Enter` / `Space` / kairysis pelės klavišas – meniu ir pabaigos ekranų mygtukai

## Projekto struktūra (procedūrinė architektūra)

```text
sfml-game/
├── assets/
│   └── fonts/
│       └── CascadiaCode.ttf   # Šriftas su pilnu lietuviškų raidžių palaikymu
├── src/
│   ├── types.hpp              # Konstantos, enum class būsenos ir struct duomenys
│   ├── game_logic.hpp/.cpp    # Įvestis, judėjimas, kliūčių generavimas, susidūrimai
│   ├── renderer.hpp/.cpp      # Kelio, automobilių, HUD ir meniu piešimas
│   └── main.cpp               # Pagrindinis ciklas ir išteklių įkėlimas
├── CMakeLists.txt             # CMake konfigūracija (automatiškai parsiunčia SFML 3.0.2)
├── CMakePresets.json          # x64-debug ir x64-release nustatymai
├── build.bat                  # Kompiliavimo skriptas
└── run.bat                    # Kompiliavimo ir paleidimo skriptas
```

## Kompiliavimas ir paleidimas

### 1 būdas: Per skriptą (Windows + Visual Studio C++ įrankiai)
- Paleiskite **`run.bat`** (arba terminale `.\run.bat`), kad projektas automatiškai susikompiliuotų ir pasileistų.
- Release versijai:
  ```bat
  run.bat x64-release
  ```

### 2 būdas: Visual Studio
1. Visual Studio -> **Open a local folder** -> pasirinkite `sfml-game`.
2. Visual Studio automatiškai sukonfigūruos `CMakePresets.json` ir SFML.
3. Spauskite **F5** (paleisti `SFMLGame.exe`).
