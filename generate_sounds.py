#!/usr/bin/env python3
"""Generate WAV sound files for the Snake game using only stdlib."""
import wave, struct, math, os

SAMPLE_RATE = 44100
OUT_DIR = os.path.join(os.path.dirname(__file__), "sounds")
os.makedirs(OUT_DIR, exist_ok=True)

def write_wav(filename, frames):
    path = os.path.join(OUT_DIR, filename)
    with wave.open(path, 'w') as f:
        f.setnchannels(1)
        f.setsampwidth(2)   # 16-bit
        f.setframerate(SAMPLE_RATE)
        f.writeframes(b''.join(struct.pack('<h', max(-32768, min(32767, int(s)))) for s in frames))
    print(f"  wrote {path}")

def sine(freq, duration, volume=0.5, phase=0.0):
    n = int(SAMPLE_RATE * duration)
    return [volume * 32767 * math.sin(2 * math.pi * freq * t / SAMPLE_RATE + phase)
            for t in range(n)]

def envelope(samples, attack=0.01, release=0.05):
    """Apply a simple attack/release envelope."""
    n = len(samples)
    atk = int(attack * SAMPLE_RATE)
    rel = int(release * SAMPLE_RATE)
    out = list(samples)
    for i in range(min(atk, n)):
        out[i] *= i / atk
    for i in range(min(rel, n)):
        out[n - 1 - i] *= i / rel
    return out

def note(freq, dur, vol=0.4):
    return envelope(sine(freq, dur, vol), attack=0.005, release=0.03)

# ---- Note frequencies ----
C4, D4, E4, F4, G4, A4, B4 = 261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88
C5, D5, E5, G5              = 523.25, 587.33, 659.25, 783.99

# ---- Background music: upbeat looping melody ----
# Two-bar motif that loops cleanly
BEAT = 0.13  # seconds per 16th note

melody_notes = [
    (E5, 2), (D5, 2), (C5, 2), (D5, 2),
    (E5, 2), (E5, 2), (E5, 4),
    (D5, 2), (D5, 2), (D5, 4),
    (E5, 2), (G5, 2), (G5, 4),
    (E5, 2), (D5, 2), (C5, 2), (D5, 2),
    (E5, 2), (E5, 2), (E5, 2), (E5, 2),
    (D5, 2), (D5, 2), (E5, 2), (D5, 2),
    (C5, 8),
]

bass_notes = [
    (C4, 4), (G4, 4), (A4, 4), (G4, 4),
    (F4, 4), (C4, 4), (G4, 4), (G4, 4),
    (C4, 4), (G4, 4), (A4, 4), (G4, 4),
    (F4, 4), (C4, 4), (G4, 4), (G4, 4),
]

def build_track(note_list):
    frames = []
    for freq, beats in note_list:
        frames += note(freq, beats * BEAT)
    return frames

melody_frames = build_track(melody_notes)
bass_frames   = build_track(bass_notes)

# Pad shorter track
max_len = max(len(melody_frames), len(bass_frames))
melody_frames += [0.0] * (max_len - len(melody_frames))
bass_frames   += [0.0] * (max_len - len(bass_frames))

music_frames = [m * 0.6 + b * 0.3 for m, b in zip(melody_frames, bass_frames)]

print("Generating sounds/music.wav ...")
write_wav("music.wav", music_frames)

# ---- Die sound: descending frequency sweep + minor chord stab ----
def sweep(f_start, f_end, duration, vol=0.5):
    n = int(SAMPLE_RATE * duration)
    frames = []
    for t in range(n):
        frac = t / n
        freq = f_start + (f_end - f_start) * frac
        s = vol * 32767 * math.sin(2 * math.pi * freq * t / SAMPLE_RATE)
        env = 1.0 - frac          # linear fade out
        frames.append(s * env)
    return frames

die_frames  = sweep(880, 110, 0.7, vol=0.5)
# Add a short dissonant thud at the start
thud = note(A4, 0.12, vol=0.6)
die_frames[:len(thud)] = [d + t for d, t in zip(die_frames[:len(thud)], thud)]

print("Generating sounds/die.wav ...")
write_wav("die.wav", die_frames)

# ---- Eat sound: quick ascending blip ----
eat_frames = note(G5, 0.04, vol=0.5) + note(C5*2, 0.06, vol=0.4)
print("Generating sounds/eat.wav ...")
write_wav("eat.wav", eat_frames)

print("Done.")
