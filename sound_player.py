#!/usr/bin/env python3
"""
Snake sound player — listens on serial and plays audio via pygame.
Usage: python3 sound_player.py <port> [baud]
  Mac example:  python3 sound_player.py /dev/cu.usbmodem14101
  Win example:  python3 sound_player.py COM3
"""

import sys, os
import serial
import pygame

SOUNDS_DIR = os.path.join(os.path.dirname(__file__), "sounds")

def load_sounds():
    sounds = {}
    files = {
        "MUSIC:START": "music.wav",
        "SOUND:DIE":   "die.wav",
        "SOUND:EAT":   "eat.wav",
    }
    for cmd, fname in files.items():
        path = os.path.join(SOUNDS_DIR, fname)
        if os.path.exists(path):
            sounds[cmd] = pygame.mixer.Sound(path)
            print(f"  loaded {fname}")
        else:
            print(f"  WARNING: {path} not found — run generate_sounds.py first")
    return sounds

def play_sound(cmd, sounds, music_channel, sfx_channel):
    cmd = cmd.strip()

    if cmd == "MUSIC:START":
        if "MUSIC:START" in sounds:
            music_channel.play(sounds["MUSIC:START"], loops=-1)

    elif cmd == "MUSIC:STOP":
        music_channel.stop()

    elif cmd == "SOUND:DIE":
        music_channel.stop()
        if "SOUND:DIE" in sounds:
            sfx_channel.play(sounds["SOUND:DIE"])

    elif cmd == "SOUND:EAT":
        if "SOUND:EAT" in sounds:
            sfx_channel.play(sounds["SOUND:EAT"])

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    pygame.mixer.init(frequency=44100, size=-16, channels=1, buffer=512)
    pygame.mixer.set_num_channels(4)
    music_channel = pygame.mixer.Channel(0)
    sfx_channel   = pygame.mixer.Channel(1)

    print(f"Loading sounds from {SOUNDS_DIR} ...")
    sounds = load_sounds()

    print(f"Connecting to {port} at {baud} baud ...")
    try:
        ser = serial.Serial(port, baud, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        sys.exit(1)

    print("Connected. Listening for sound commands (Ctrl+C to quit).")
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(f"  << {line}")
                    play_sound(line, sounds, music_channel, sfx_channel)
    except KeyboardInterrupt:
        print("\nStopped.")
        music_channel.stop()
        ser.close()

if __name__ == "__main__":
    main()
