#!/bin/bash
set -e

echo "=== Installazione EvoPlayer ==="

# Cartella sorgente (dove si trova questo script)
SRC="$(cd "$(dirname "$0")" && pwd)"

# Destinazione
DEST="/opt/EvoPlayer"

# Copia la cartella
echo "Copio i file in $DEST..."
sudo cp -r "$SRC" "$DEST"
sudo chmod +x "$DEST/build/EvoPlayer"

# Crea comando globale
echo "Creo comando evoplayer..."
sudo bash -c 'cat > /usr/local/bin/evoplayer << INNEREOF
#!/bin/bash
export EVOPLAYER_BASE=/opt/EvoPlayer
cd /opt/EvoPlayer
exec ./build/EvoPlayer "\$@"
INNEREOF'
sudo chmod +x /usr/local/bin/evoplayer

# Crea icona desktop
echo "Creo icona desktop..."
DESKTOP_FILE="$HOME/Scrivania/EvoPlayer.desktop"
cat > "$DESKTOP_FILE" << INNEREOF
[Desktop Entry]
Name=EvoPlayer
Comment=Hi-Fi Audio Player
Exec=env EVOPLAYER_BASE=/opt/EvoPlayer /opt/EvoPlayer/build/EvoPlayer
Icon=/opt/EvoPlayer/assets/evoplayer.svg
Type=Application
Terminal=false
Categories=Audio;Music;
INNEREOF
chmod +x "$DESKTOP_FILE"

echo ""
echo "=== Installazione completata! ==="
echo "Avvia con: evoplayer"
echo "Oppure doppio click sull'icona sul desktop"
