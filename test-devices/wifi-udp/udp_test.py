#!/usr/bin/env python3
"""Test B (wifi-udp): manda paquetes UDP al ESP y cuenta los ecos.

El ESP (firmware esp_udp_echo) devuelve tal cual lo que reciba. Si no se pasa la
IP, se descubre sola: se manda un paquete a broadcast y el ESP responde desde su
propia IP (asi la aprendemos, sin tocar el firmware). Si tu router aisla a los
clientes, el broadcast no llega: pasa la IP a mano (la imprime el ESP por Serial).

Uso:  uv run udp_test.py                 # descubre el ESP por broadcast
      uv run udp_test.py 192.168.1.42    # o con la IP a mano
"""

import socket
import sys

PORT = 4210
COUNT = 5
TIMEOUT = 2.0


def discover(sock):
    """Manda a broadcast y devuelve la IP del primer ESP que responde."""
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.sendto(b"ping", ("255.255.255.255", PORT))
    try:
        _, addr = sock.recvfrom(1024)
        return addr[0]
    except socket.timeout:
        return None


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(TIMEOUT)

    host = sys.argv[1] if len(sys.argv) == 2 else discover(sock)
    if not host:
        sys.exit("No se encontro el ESP por broadcast. Pasa la IP: uv run udp_test.py <IP>")

    print(f"enviando {COUNT} paquetes a {host}:{PORT}...")
    ok = 0
    for i in range(COUNT):
        msg = f"ping {i}".encode()
        sock.sendto(msg, (host, PORT))
        try:
            data, _ = sock.recvfrom(1024)
        except socket.timeout:
            continue
        if data == msg:
            ok += 1

    if ok == COUNT:
        print(f"{ok}/{COUNT} ecos OK -> enlace funcionando")
    else:
        print(f"{ok}/{COUNT} ecos OK -> revisa IP, red o firewall")


if __name__ == "__main__":
    main()
