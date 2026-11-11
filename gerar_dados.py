#!/usr/bin/env python3
"""Gera dados.txt: 1000 linhas 'estatura(cm) peso(kg)' (seed fixa)."""
import random
random.seed(42)
with open("dados.txt", "w") as f:
    for _ in range(1000):
        h = random.gauss(170, 9)
        w = max(40.0, random.gauss(0.9 * (h - 100) + 5, 9))
        f.write(f"{h:.1f} {w:.1f}\n")
