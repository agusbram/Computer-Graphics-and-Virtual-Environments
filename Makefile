# ------------------------------------------------------------------------------
# Makefile - proyecto OpenGL inicial (ventana GLFW + GLAD).
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Librerias de terceros
GLAD_DIR = ./third_party/glad
include $(GLAD_DIR)/glad.mk

# ------------------------------------------------------------------------------
# Configurar includes paths
INC_DIRS = \
    $(GLAD_INC_DIRS) \
    ./src

# ------------------------------------------------------------------------------
# Configurar dirs de librerias externas y dirs locales al proyecto
# (fuentes para compilar).
# Recolección automática de fuentes. Mantener DISJUNTO de SRC_DIR
# (si no, main.o se linkearía dos veces).
# Se puede usar LIB_DIRS para estructurar el proyecto con subdirectorios.
# Ej.
#LIB_DIRS = \
#    ./src/input
LIB_DIRS = \
    $(GLAD_LIB_DIRS) \
    ./src/core

# ------------------------------------------------------------------------------
# Dir del main de la aplicación y nombre del archivo de código fuente
SRC_DIR  = \
    ./src                     # main.cpp

MAIN_CXX     = main

# ------------------------------------------------------------------------------
# Configurar salida el proyecto
PROJECT_NAME = ogl-app

# ------------------------------------------------------------------------------
# Configurar linkeo de librerías
ifeq ($(OS), Windows_NT)
        PROJECT_LDLIBS = -L/mingw64/lib -lglfw3 -lopengl32 -lgdi32 -lpthread
else
	PROJECT_LDLIBS = -lglfw -lGL -lX11 -lXrandr -ldl -lpthread
endif

# ------------------------------------------------------------------------------
# Configurar opciones de compilación
# Para compilar con símbolos para debug usar: -g -Wall
USERCPPFLAGS = -O2 -Wall -Wextra

# ------------------------------------------------------------------------------
# Makefile.master
ifeq ($(wildcard ./Makefile.master),)
    $(error Makefile.master no encontrado: se espera symlink a \
    makefile-unified/Makefile.master o una copia local junto a este Makefile)
endif
include ./Makefile.master
