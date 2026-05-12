#!/bin/bash
# Auto-deteccion del puerto serial del Arduino
# Prioriza ttyACM (UNO/Mega nativos) sobre ttyUSB (Nano/CH340/FTDI)
# Se puede pasar un filtro como argumento para seleccionar entre multiples dispositivos

set -euo pipefail

filter="${1:-}"

# Candidatos: solo dispositivos que existen Y son legibles
candidates=()
for dev in /dev/ttyACM* /dev/ttyUSB*; do
    [ -r "$dev" ] && [ -w "$dev" ] && candidates+=("$dev")
done

if [ ${#candidates[@]} -eq 0 ]; then
    echo "ERROR: No se encontraron dispositivos seriales accesibles (/dev/ttyACM* ni /dev/ttyUSB*)" >&2
    echo "       Verifica que el Arduino este conectado y que tu usuario tenga permisos (grupo dialout)." >&2
    # Listar los que existen aunque no sean accesibles, como diagnostico
    for dev in /dev/ttyACM* /dev/ttyUSB*; do
        [ -e "$dev" ] && echo "  Encontrado $dev (sin permisos de lectura/escritura)" >&2
    done
    exit 1
fi

if [ ${#candidates[@]} -eq 1 ]; then
    echo "${candidates[0]}"
    exit 0
fi

# Multiples candidatos: intentar identificar por filtro
if [ -n "$filter" ]; then
    for dev in "${candidates[@]}"; do
        if echo "$dev" | grep -qi "$filter"; then
            echo "$dev"
            exit 0
        fi
    done
    echo "ERROR: Filtro '$filter' no coincide con ningun dispositivo de: ${candidates[*]}" >&2
    exit 1
fi

# Multiples candidatos sin filtro: intentar heuristica con udevadm
# Puntuar cada dispositivo segun que tan probable es que sea un Arduino
best_dev=""
best_score=0
for dev in "${candidates[@]}"; do
    info=$(udevadm info --query=property --name="$dev" 2>/dev/null || true)
    score=0

    # CH340/CH341 (clones Nano/Mega) - muy comun
    echo "$info" | grep -qiE "1a86:" && score=$((score + 10))
    echo "$info" | grep -qiE "CH340|CH341" && score=$((score + 10))

    # FTDI (Nano/Uno originales con chip FTDI)
    echo "$info" | grep -qiE "0403:60" && score=$((score + 10))
    echo "$info" | grep -qiE "FT232|FTDI" && score=$((score + 10))

    # Arduino oficial (UNO/Mega con USB nativo via 16U2)
    echo "$info" | grep -qiE "2341:" && score=$((score + 20))
    echo "$info" | grep -qiE "arduino" && score=$((score + 20))

    # ATmega/AVR
    echo "$info" | grep -qiE "ATmega|16u2|atmel" && score=$((score + 5))

    if [ "$score" -gt "$best_score" ]; then
        best_score=$score
        best_dev="$dev"
    fi
done

if [ -n "$best_dev" ] && [ "$best_score" -gt 0 ]; then
    echo "$best_dev"
    exit 0
fi

# Ultimo recurso: listar y pedir al usuario
echo "Multiples dispositivos seriales encontrados, no se pudo determinar automaticamente:" >&2
for dev in "${candidates[@]}"; do
    info=$(udevadm info --query=property --name="$dev" 2>/dev/null || echo "desconocido")
    vendor=$(echo "$info" | grep "ID_VENDOR_FROM_DATABASE=" | cut -d= -f2)
    model=$(echo "$info" | grep "ID_MODEL_FROM_DATABASE=" | cut -d= -f2)
    echo "  $dev  ($vendor - $model)" >&2
done
echo "" >&2
echo "Usa: make PORT=/dev/ttyUSBx upload" >&2
echo "O:   detect_port.sh <filtro>   (ej: detect_port.sh USB1)" >&2
exit 1
