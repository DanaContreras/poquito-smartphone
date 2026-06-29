#!/usr/bin/env python3
"""
Envia un archivo de audio al firmware test_audio_stream por UART.

Acepta cualquier formato soportado por ffmpeg (mp3, wav, ogg, flac, m4a...).

El firmware lee cada byte y lo escribe al DAC MCP4725 en tiempo real. El
sample rate efectivo lo fija el propio UART: BAUD/10 muestras por segundo.

Requisitos:
  - pyserial, typer, rich  (uv sync desde la raiz del repo)
  - ffmpeg                 (paquete del sistema)

Uso tipico:
  uv run stream_audio.py audio.mp3
  uv run stream_audio.py audio.wav --port /dev/ttyUSB0
"""

import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

try:
    import serial
except ImportError:
    sys.exit("Falta pyserial. Instalalo con: uv sync (desde la raiz del repo)")

try:
    import typer
    from rich.console import Console
    from rich.panel import Panel
    from rich.progress import (
        BarColumn,
        Progress,
        TextColumn,
        TimeElapsedColumn,
        TimeRemainingColumn,
    )
    from rich.table import Table
except ImportError:
    sys.exit(
        "Faltan dependencias. Instalalas con: uv sync (desde la raiz del repo)"
    )

SYNC_REQ = b"\xaa"
SYNC_ACK = 0x55
DONE_ACK = 0x44

DEFAULT_BAUD = 115200
RESET_WAIT_S = 2.5
CHUNK = 512  # bytes por escritura; el write() bloqueante marca el ritmo real

console = Console()
app = typer.Typer(add_completion=False, rich_markup_mode="rich")


def detect_port() -> str:
    # Raiz del repo: test-devices/mcp4725_pam8403/stream_audio.py -> parents[2]
    project_root = Path(__file__).resolve().parents[2]
    script = project_root / "scripts" / "detect_port.sh"
    result = subprocess.run([str(script)], capture_output=True, text=True, check=True)
    port = result.stdout.strip()
    if not port:
        console.print(
            "[red]No se detecto ningun puerto Arduino.[/] Pasa [bold]--port[/] manualmente."
        )
        raise typer.Exit(1)
    return port


def convert_to_pcm(input_path: Path, sample_rate: int) -> bytes:
    if not shutil.which("ffmpeg"):
        console.print(
            "[red]ffmpeg no esta instalado.[/] Instalalo con tu gestor de paquetes."
        )
        raise typer.Exit(1)

    cmd = [
        "ffmpeg",
        "-hide_banner",
        "-loglevel",
        "error",
        "-i",
        str(input_path),
        "-ac",
        "1",
        "-ar",
        str(sample_rate),
        "-sample_fmt",
        "u8",
        "-f",
        "u8",
        "-",
    ]
    result = subprocess.run(cmd, capture_output=True, check=False)
    if result.returncode != 0:
        console.print("[red]ffmpeg fallo:[/]")
        console.print(result.stderr.decode(errors="replace"))
        raise typer.Exit(1)
    return result.stdout


def stream(port: str, baud: int, pcm: bytes) -> None:
    sample_rate = baud // 10
    duration_s = len(pcm) / sample_rate

    try:
        ser = serial.Serial(port, baud, timeout=5)
    except serial.SerialException as exc:
        console.print(f"[red]No se pudo abrir el puerto[/] [bold]{port}[/]: {exc}")
        raise typer.Exit(1)

    with ser:
        with console.status(f"[cyan]Esperando reset del Arduino ({RESET_WAIT_S}s)..."):
            time.sleep(RESET_WAIT_S)
            ser.reset_input_buffer()

        ser.write(SYNC_REQ)
        ser.flush()
        ack = ser.read(1)
        if not ack or ack[0] != SYNC_ACK:
            console.print(
                f"[red]No llego SYNC_ACK[/] (recibido: {ack!r}). Verifica que el firmware "
                "[bold]test_audio_stream[/] este cargado."
            )
            raise typer.Exit(1)

        ser.write(len(pcm).to_bytes(4, "little"))

        progress = Progress(
            TextColumn("[bold cyan]Streaming"),
            BarColumn(),
            TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
            TimeElapsedColumn(),
            TextColumn("/"),
            TimeRemainingColumn(),
            console=console,
        )
        start = time.monotonic()
        with progress:
            task = progress.add_task("stream", total=len(pcm))
            for i in range(0, len(pcm), CHUNK):
                ser.write(pcm[i : i + CHUNK])
                progress.advance(task, min(CHUNK, len(pcm) - i))
            ser.flush()

        ack = ser.read(1)
        elapsed = time.monotonic() - start
        if not ack or ack[0] != DONE_ACK:
            console.print(
                f"[yellow]No llego DONE_ACK[/] (recibido: {ack!r}). El audio puede haberse cortado."
            )
            raise typer.Exit(1)
        console.print(
            f"[bold green]Completado[/] en {elapsed:.2f}s (esperado ~{duration_s:.2f}s)."
        )


def _info_panel(port: str, audio_file: Path, sample_rate: int) -> Panel:
    table = Table.grid(padding=(0, 2))
    table.add_column(style="bold")
    table.add_column()
    table.add_row("Puerto", port)
    table.add_row("Archivo", audio_file.name)
    table.add_row("Formato", f"{sample_rate} Hz · mono · 8-bit")
    return Panel(
        table, title="[bold]Poquito · Audio Streamer", border_style="cyan", expand=False
    )


@app.command()
def main(
    audio_file: Path = typer.Argument(
        ...,
        exists=True,
        dir_okay=False,
        readable=True,
        help="Archivo (.wav, .ogg, .mp3, etc) a reproducir",
    ),
    port: Optional[str] = typer.Option(
        None, "--port", "-p", help="Puerto serial (auto-detecta si se omite)"
    ),
    baud: int = typer.Option(
        DEFAULT_BAUD,
        "--baud",
        "-b",
        help="Baud rate; el sample rate efectivo es BAUD/10",
    ),
):
    """Stream de audio al Arduino via UART -> MCP4725 -> PAM8403."""
    resolved_port = port or detect_port()
    sample_rate = baud // 10

    console.print(_info_panel(resolved_port, audio_file, sample_rate))

    with console.status(f"[cyan]Convirtiendo {audio_file.name} con ffmpeg..."):
        pcm = convert_to_pcm(audio_file, sample_rate)
    console.print(
        f"[green]✓[/] Convertido: {len(pcm)} bytes (~{len(pcm) / sample_rate:.2f} s)"
    )

    stream(resolved_port, baud, pcm)


if __name__ == "__main__":
    app()
