#!/bin/bash
# NXL Field Timer — Relay Server launcher (Mac)
# Double-click or run from Terminal

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

echo ""
echo " ================================================"
echo "  NXL PAINTBALL TIMER — Relay Server"
echo " ================================================"
echo ""

# Check for portable node next to this script
if [ -f "$DIR/node-mac" ]; then
    echo " Using portable Node.js..."
    chmod +x "$DIR/node-mac"
    "$DIR/node-mac" "$DIR/server.js"

# Fall back to system node
elif command -v node &>/dev/null; then
    echo " Using system Node.js..."
    node "$DIR/server.js"

else
    echo " ERROR: Node.js not found!"
    echo ""
    echo " Install Node from https://nodejs.org (LTS)"
    echo " Then run this script again."
    read -p " Press Enter to close..."
fi
