# Guida Creazione Skin — EvoPlayer

## Cos'è una skin
Una skin è un tema grafico completo per EvoPlayer.
Chiunque con Blender può crearne una personalizzata!

## Cosa ti serve
- Blender (qualsiasi versione recente)
- Fantasia e creatività
- 17 PNG da renderizzare

## I 17 PNG da creare
Tutti i PNG devono avere lo sfondo TRASPARENTE (canale alpha).

**Plancia (1 PNG):**
- player_panel.png — la plancia frontale completa

**Pulsanti transport (12 PNG):**
- btn_rewind_normal.png + btn_rewind_hover.png
- btn_play_normal.png + btn_play_hover.png
- btn_pause_normal.png + btn_pause_hover.png
- btn_stop_normal.png + btn_stop_hover.png
- btn_forward_normal.png + btn_forward_hover.png
- btn_open_normal.png + btn_open_hover.png

**Power button (2 PNG):**
- power_normal.png — LED spento
- power_on.png — LED acceso

**Knob (3 PNG):**
- knob_volume.png
- knob_bass.png
- knob_mid.png

## Impostazioni Blender obbligatorie
- Motore di render: Cycles
- Film → Transparent: ON (indispensabile per l'alpha)
- Camera: Orthographic frontale
- Sfondo: trasparente — nessun colore di sfondo

## Libertà creativa
Puoi cambiare TUTTO esteticamente:
- Forma e dimensione della plancia
- Stile pulsanti (quadrati, rotondi, vintage, sci-fi...)
- Materiali (metallo, plastica, legno, carbonio...)
- Posizione di ogni elemento
- Colori LED e display

## Il file skin.ini
Ogni skin ha un file skin.ini che dice a EvoPlayer
dove si trova ogni elemento sulla tua plancia.

Esempio skin.ini:

    [panel]
    image=player_panel.png
    width=2030
    height=537

    [knob_volume]
    x=1798
    y=329
    size=276

    [button_play]
    x=526
    y=437

## Struttura cartelle

    skins/
        mia_skin/
            skin.ini
            player_panel.png
            buttons/
                btn_play_normal.png
                btn_play_hover.png
                ... (tutti i 12 pulsanti)
                power_normal.png
                power_on.png
            knobs/
                knob_volume.png
                knob_bass.png
                knob_mid.png

## Come caricare la skin
1. Copia la cartella skin in ~/EvoPlayer/skins/
2. Apri EvoPlayer
3. Click su Power → Impostazioni → Skin
4. Seleziona la tua skin dalla lista
5. La skin si carica immediatamente!

## Troubleshooting
- PNG non visibile → controlla che Film→Transparent sia ON in Blender
- Elemento fuori posto → controlla le coordinate nel skin.ini
- Skin non appare nella lista → controlla che skin.ini esista nella cartella
- Knob non ruota bene → il PNG deve essere quadrato (es. 512x512)

---
EvoPlayer — fatto con amore da Marco e Claude ❤️
