# ------------------------------------------------------------------------------
# dlfdm.mk — entradas de build del componente dlfdm
#
# Declara las fuentes e includes del módulo en un solo lugar.
#
# DLFDM_DIR debe apuntar a la raíz del módulo dlfdm
#   - "."                  si se compila desde el repo del módulo
#   - "libs/dlfdm"         si se integra como submódulo en otro proyecto
#
# El -I a la raíz habilita los includes con prefijo de módulo:
#   #include <dlfdm/fdmsolver.h>
#
# Dependencias externas (deben estar en el include path del proyecto):
#   -
# ------------------------------------------------------------------------------
DLFDM_DIR ?= .

DLFDM_INC_DIRS = \
    $(DLFDM_DIR)/include

# Listado manual de subcarpetas con fuentes (wildcard no recursivo).
# Agregar acá cada nueva subcarpeta de src/ con .cpp.
DLFDM_LIB_DIRS = \
    $(DLFDM_DIR)/src/dlfdm \
    $(DLFDM_DIR)/src/dlfdm/models/aircraft \
    $(DLFDM_DIR)/src/dlfdm/models/propulsion
