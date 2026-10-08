# ─────────────────────────────────────────────────────────────────────────────
# Makefile — repo del FDM (dlfdm). Build standalone del ejecutable de prueba.
# ─────────────────────────────────────────────────────────────────────────────

# Reutiliza la lista de fuentes/includes del componente (una sola fuente de verdad)
DLFDM_DIR = .
include ./dlfdm.mk

INC_DIRS = $(DLFDM_INC_DIRS)
LIB_DIRS = $(DLFDM_LIB_DIRS)        # incluye src/dlfdm
SRC_DIR  = ./test                   # acá vive main.cpp (entry point)

PROJECT_NAME = dlfdm-test
MAIN_CXX     = main
# HUD Test
#PROJECT_NAME = dlfdm-hud-test
#MAIN_CXX     = hud-data-example

PROJECT_LDLIBS = -lpthread          # el FDM no necesita nada de GUI

USERCPPFLAGS = -O2 -Wall -Wextra

# Build con símbolos de debug:
#USERCPPFLAGS = -g -Wall -Wextra

include ./Makefile.master
