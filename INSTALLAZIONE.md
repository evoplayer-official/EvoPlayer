cat > ~/EvoPlayer/INSTALLAZIONE.md << 'EOF1'
# EvoPlayer — Guida Installazione

## Su un nuovo computer

### 1. Copia la cartella EvoPlayer
Copia tutta la cartella EvoPlayer sul nuovo computer
(chiavetta USB, rete, ecc.)

### 2. Apri il terminale nella cartella copiata
```bash
cd /percorso/dove/hai/copiato/EvoPlayer
```

### 3. Esegui lo script di installazione
```bash
./installa.sh
```

### 4. Fatto!
- Doppio click sull'icona sul Desktop per avviare
- Oppure da terminale: `evoplayer`

---

## Comandi utili

### Avvio
```bash
evoplayer
```

### Avvio da cartella di lavoro (sviluppo)
```bash
cd ~/EvoPlayer && ./build/EvoPlayer
```

### Compilazione completa
```bash
cd ~/EvoPlayer && make clean && qmake EvoPlayer.pro && make -j4
```

### Ricompila rapido
```bash
cd ~/EvoPlayer && make -j4
```

### Backup
```bash
cp -r ~/EvoPlayer ~/EvoPlayer_Backups/vX.X_sessioneNN_descrizione
```

### Ripristino backup
```bash
cp -r ~/EvoPlayer_Backups/vX.X_sessioneNN_descrizione ~/EvoPlayer
```

### Lista backup disponibili
```bash
ls ~/EvoPlayer_Backups/
```

---

## Requisiti sul nuovo computer
- Pop!_OS / Ubuntu 24.04
- Qt5, libqmmp, libGL, libGLEW, libfreetype, libvlc, libprojectM
- Installabili con:
```bash
sudo apt install qt5-default libqmmp-dev libglew-dev libfreetype-dev vlc libprojectm-dev
```
EOF1
echo "File creato OK"