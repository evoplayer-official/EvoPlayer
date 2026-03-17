════════════════════════════════════════════════════════════════
  MEMORIA_A_CORE.txt — EvoPlayer
  Versione: 1.1 | Aggiornare SOLO se cambia qualcosa di strutturale
════════════════════════════════════════════════════════════════

━━━ CHI SIAMO ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Marco  → creatore, visione creativa, direzione estetica
         NON ha competenze tecniche di programmazione
Claude → programmatore assoluto, guida tecnica, tutore Blender
         Lingua: sempre italiano
         Scrive TUTTO il codice — Marco non tocca i file

━━━ COME LAVORIAMO INSIEME ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
• Claude fornisce i comandi uno alla volta, chiari, pronti da incollare
• Marco esegue e riporta l'output — Claude analizza e procede
• I sed su righe lunghe/multiriga sono INAFFIDABILI
  → usare sempre python3 -c o riscrivere il file intero
• File lunghi → scrivere in parti con cat >> (più marcatori distinti)
• Mai usare EOF come marcatore — usare EOF1, EOF2, ENDOFFILE ecc.
• Al termine di ogni sessione: aggiornare MEMORIA_B_STATO.txt
• Fornire SEMPRE entrambi i file all'inizio di ogni sessione

━━━ ⚠️  REGOLE BACKUP — CRITICHE E INVIOLABILI ━━━━━━━━━━━━━━━
BACKUP IMMEDIATO dopo ogni funzionalità confermata da Marco.
Claude deve proporre il comando backup SUBITO, senza aspettare.

QUANDO fare backup:
  ✅ Compilazione ZERO errori ZERO warning
  ✅ Finestra si apre e funziona visivamente
  ✅ Marco conferma "funziona"
  → Claude dà IMMEDIATAMENTE il comando backup

QUANDO NON fare backup:
  ❌ MAI a metà sessione
  ❌ MAI con codice rotto
  ❌ MAI prima della conferma di Marco

PRIMA di ripristinare un backup:
  ⚠️  Verificare SEMPRE cosa si perde
  ⚠️  Salvare i file nuovi importanti (PNG, codice) in posizione sicura
  ⚠️  Chiedere conferma esplicita a Marco prima di procedere

ASSETS (PNG, GLB, BLEND):
  ⚠️  Il backup del codice NON include i file in assets/
  ⚠️  Prima di ripristinare → copiare assets/ in posizione sicura
  ⚠️  I PNG renderizzati sono lavoro prezioso — MAI perderli

Backup:    cp -r ~/EvoPlayer ~/EvoPlayer_Backups/vX.X_sessioneNN_desc
Ripristino: SOLO dopo aver salvato assets/ e confermato con Marco

━━━ VISIONE EVOPLAYER ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Player audio Linux con architettura a moduli fisici separati
ispirati agli stereo Kenwood anni '90.
L'utente assembla il proprio stereo hi-fi:
moduli si impilano, agganciano e spostano liberamente sul desktop
con magnetismo fisico e animazioni reali.

Moduli base:
  • Modulo 1 — Player        (in alto)
  • Modulo 2 — Equalizzatore (centro)
  • Modulo 3 — Libreria      (in basso)
Architettura aperta: plugin .so caricati dinamicamente

━━━ DECISIONI ARCHITETTURALI DEFINITIVE ━━━━━━━━━━━━━━━━━━━━━━
1. OGNI MODULO = FINESTRA INDIPENDENTE SUL DESKTOP
   Qt::FramelessWindowHint | WA_TranslucentBackground
   Drag libero | Snap magnetico | Separati o impilati

2. RENDERING PNG MODE (decisione sessione 15)
   Sfondo: PNG plancia fullscreen (player_panel.png)
   Overlay: PNG pulsanti/knob con alpha sopra la plancia
   Testo: FreeType OpenGL runtime sopra il display
   3D model: DISABILITATO (riservato per futuro upgrade)

3. PARALLASSE DINAMICO
   Mouse → camera ruota ±2.5° (lerp 0.05) — implementato

4. TESTO/DISPLAY → OpenGL runtime con FreeType (NON in Blender)

5. VU METER → monocromatico azzurro #D6EEFF

6. BUILD SYSTEM → qmake (mai CMake)

━━━ STANDARD PNG RENDERING — DEFINITIVO ━━━━━━━━━━━━━━━━━━━━━
Motore      : Cycles
Risoluzione : 2048×1600 px — UGUALE PER TUTTI I PNG
Sfondo      : Film → Transparent ON (canale alpha)
Camera      : Orthographic frontale
Luci        : 3 Area — Frontale(500,#E8F0FF) + Fill dx(150) + Rim(200)
C++         : carica PNG come texture OpenGL, scala automatica

━━━ ESTETICA — SKIN DEFAULT ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Ispirazione   : Kenwood anni '90 — hi-fi professionale
Colore base   : nero antracite profondo
Materiali     : "Black procedural scratched plastic" (BlenderKit)
LED           : azzurro ghiaccio #D6EEFF — knob, pulsanti, power, VU
               Cornici pulsanti trasporto: LED bianche emissive
Display       : "Black Window Glass" (BlenderKit) — vetro scuro glossy
Stile         : fotorealistico, spessore fisico, muscolare e delicato

━━━ PANNELLO FRONTALE PLAYER — LAYOUT DEFINITIVO ━━━━━━━━━━━━━
Sinistra-alto : Power button — semisfera + ⏻ emissivo + anello LED
Sinistra-basso: Display VU Meter — vetro + cornice LED + FFT OpenGL
Centro        : Display principale — vetro glossy + cornice LED
               Contenuto OpenGL: titolo, artista, tempo, bitrate
Destra        : 3 knob — BASSI | ACUTI | VOLUME (conici, LED azzurro)
Centro-basso  : 6 pulsanti trasporto:
               ◀◀ Rewind | ▶ Play | ⏸ Pause | ⏹ Stop | ▶▶ Fwd | ⏏ Open

━━━ COORDINATE PIXEL PLANCIA (2048×1600) ━━━━━━━━━━━━━━━━━━━━
Pulsanti transport (centro, Y dal top):
  Rewind  → X=400,  Y=437
  Play    → X=526,  Y=437
  Pause   → X=656,  Y=437
  Stop    → X=784,  Y=437
  Forward → X=908,  Y=437
  Open    → X=1034, Y=437
  Bordi Rewind: X=356..443, Y=408..467
  Hit zone: ±44px X, ±30px Y

━━━ SISTEMA MARCO — POP!_OS ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sistema    : Pop!_OS Linux (Ubuntu 24.04)
User       : snorky
Compilatore: g++ 13.3.0
Qt5        : 5.15.13 | qmake 3.1
libqmmp    : 1.6.2 in /usr/lib/qmmp/
             symlinks: /usr/lib/libqmmp-1.so → /usr/lib/qmmp/libqmmp-1.so
                       /usr/lib/libqmmp-1.so.1 → /usr/lib/qmmp/libqmmp-1.so.1
Plugin qmmp: /usr/lib/qmmp/plugins/Input/ e Output/ ✅
FreeType   : installato | Font: LiberationSans-Bold.ttf
GLEW/Assimp: installati
Blender    : 5.0.1 + BlenderKit
Progetto   : ~/EvoPlayer/

Warning normali da ignorare:
  • QSocketNotifier → normale su Wayland
  • GLEW "Unknown error" → falso positivo
  • "Failed to find mixer element" → normale su PipeWire
  • OutputALSA setupMixer → normale

━━━ STACK TECNOLOGICO ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Audio    : libqmmp 1.6.2
UI       : Qt5 5.15.13
Grafica  : OpenGL 3.3 Core + GLSL + Mesa
Font     : FreeType2 (OpenGL runtime)
Asset    : Blender 5.0.1 → PNG 2048×1600 con alpha
Linguaggio: C++17 | Build: qmake 3.1
Plugin   : .so caricati dinamicamente

━━━ COMANDI FONDAMENTALI ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Compilazione completa:
  cd ~/EvoPlayer && make clean && qmake EvoPlayer.pro && make -j4

Ricompila rapido:
  cd ~/EvoPlayer && make -j4

Avvio:
  cd ~/EvoPlayer && ./build/EvoPlayer

Errori:
  cd ~/EvoPlayer && make -j4 2>&1 | head -60

━━━ SISTEMA DI BACKUP ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Posizione: ~/EvoPlayer_Backups/
Formato  : vX.X_sessioneNN_descrizione_breve

Backup SOLO quando:
  ✅ ZERO errori ZERO warning | ✅ finestra apre e funziona
  ✅ Marco conferma → Claude dà SUBITO il comando backup
  ❌ MAI a metà sessione | ❌ MAI con codice rotto

PRIMA di ripristinare → salvare assets/ separatamente!

Backup   : cp -r ~/EvoPlayer ~/EvoPlayer_Backups/vX.X_sessioneNN_desc
Ripristino: cp -r ~/EvoPlayer_Backups/vX.X_... ~/EvoPlayer

════════════════════════════════════════════════════════════════
  Fine MEMORIA_A_CORE.txt
════════════════════════════════════════════════════════════════