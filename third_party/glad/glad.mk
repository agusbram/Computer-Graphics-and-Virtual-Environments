# ------------------------------------------------------------------------------
# glad.mk — entradas para build del componente Glad
#
# Declara las fuentes e includes del módulo en un solo lugar.
#
# GLAD_DIR debe apuntar a la raíz del componente:
#   - "."                 si se compila standalone (desde su repo)
#   - "third_party/glad"  si se integra a otro proyecto
# ------------------------------------------------------------------------------

GLAD_DIR ?= .

GLAD_INC_DIRS = \
    $(GLAD_DIR)/include

# Listado manual de subcarpetas con fuentes (wildcard no recursivo).
# Agregar acá cada nueva subcarpeta de src/ con .cpp.
GLAD_LIB_DIRS = \
    $(GLAD_DIR)/src
