#!/usr/bin/env python3
"""Gera include/icones_usados.h só com os ícones que o código usa.

A biblioteca de ícones tem ~9400 ficheiros (~100 MB); incluí-los todos (icons.h,
icons_NxN.h) obriga o compilador a mais de 3 GB de memória, o que o Pi não tem
(24/10/2026). Este script procura no src/ e no include/:
  - nomes com tamanho, ex. wi_sunrise_48x48;
  - getBitmap(nome, BitmapSize) em templates, com os tamanhos de
    getConditionsBitmap<N>, e getBitmap(nome, N).
Corre sozinho antes de cada compilação (extra_scripts no platformio.ini) e só
reescreve o ficheiro se mudar.
"""
import os
import re

try:
    Import("env")  # noqa: F821 (corre dentro do PlatformIO/SCons)
    BASE = env.subst("$PROJECT_DIR")  # noqa: F821
except NameError:
    BASE = os.path.dirname(os.path.abspath(__file__))

ICONES = os.path.join(BASE, "lib", "esp32-weather-epd-assets", "icons")
SAIDA = os.path.join(BASE, "include", "icones_usados.h")
TAMANHOS = (16, 24, 32, 48, 64, 96, 128, 160, 196)

re_direto = re.compile(r"\b([a-z0-9_]+?)_(\d+)x\2\b")
re_getbitmap = re.compile(r"getBitmap\(\s*([a-z0-9_]+)\s*,\s*(\w+)\s*\)")
re_template = re.compile(r"getConditionsBitmap<(\d+)>")


def ler_codigo():
    textos = []
    for pasta in ("src", "include"):
        for raiz, _, ficheiros in os.walk(os.path.join(BASE, pasta)):
            for f in ficheiros:
                caminho = os.path.join(raiz, f)
                if f.endswith((".cpp", ".h")) and caminho != SAIDA:
                    with open(caminho, encoding="utf-8", errors="replace") as fh:
                        textos.append(fh.read())
    return "\n".join(textos)


def existe(nome, tam):
    return os.path.isfile(os.path.join(ICONES, f"{tam}x{tam}", f"{nome}_{tam}x{tam}.h"))


def main():
    codigo = ler_codigo()
    usados = set()  # (nome, tamanho)
    for nome, tam in re_direto.findall(codigo):
        if int(tam) in TAMANHOS and existe(nome, int(tam)):
            usados.add((nome, int(tam)))

    tam_template = {int(t) for t in re_template.findall(codigo)}
    nomes_getbitmap = set()
    for nome, tam in re_getbitmap.findall(codigo):
        nomes_getbitmap.add(nome)
        tams = [int(tam)] if tam.isdigit() else sorted(tam_template)
        for t in tams:
            if existe(nome, t):
                usados.add((nome, t))
            else:
                print(f"gerar_icones: aviso, não existe {nome}_{t}x{t}")

    linhas = [
        "// GERADO por gerar_icones.py antes de cada compilação: não editar à mão.",
        "// Só os ícones usados no código (ver o cabeçalho do script).",
        "#ifndef __ICONES_USADOS_H__",
        "#define __ICONES_USADOS_H__",
        "",
        "#include <cstddef>",
        "",
    ]
    for nome, tam in sorted(usados, key=lambda x: (x[1], x[0])):
        linhas.append(f'#include "icons/{tam}x{tam}/{nome}_{tam}x{tam}.h"')

    linhas += ["", "typedef enum icon_name {"]
    linhas += [f"  {n}," for n in sorted(nomes_getbitmap)]
    linhas += ["} icon_name_t;", "",
               "constexpr const unsigned char* getBitmap(icon_name_t icon, size_t size)",
               "{", "  switch (icon) {"]
    for n in sorted(nomes_getbitmap):
        linhas += [f"  case {n}:", "    switch (size) {"]
        for t in TAMANHOS:
            if (n, t) in usados:
                linhas.append(f"    case {t}: return {n}_{t}x{t};")
        linhas += ["    default: return nullptr;", "    }"]
    linhas += ["  default:", "    return nullptr;", "  }", "}", "", "#endif", ""]

    novo = "\n".join(linhas)
    antigo = None
    if os.path.isfile(SAIDA):
        with open(SAIDA, encoding="utf-8") as fh:
            antigo = fh.read()
    if novo != antigo:
        with open(SAIDA, "w", encoding="utf-8") as fh:
            fh.write(novo)
        print(f"gerar_icones: {len(usados)} ícones -> include/icones_usados.h")


main()
