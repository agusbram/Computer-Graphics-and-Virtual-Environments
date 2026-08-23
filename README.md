# CGyAV — Proyecto OpenGL (template + Práctico 01)

Proyecto base de **Computación gráfica y ambientes virtuales (0494)** — CRUC-IUA.
Template de aplicación OpenGL moderna (core profile 4.6) con GLFW + GLAD sobre el
que se desarrolla el **Práctico 01** (triángulo con shaders en archivos y módulo
`ResourceManager`), primera pieza del proyecto integrador
(*Simulador de Acrobacias Aéreas*).

---

## Requisitos

- Compilador C++ con soporte **C++17** (`g++`)
- `make`
- Librerías de desarrollo: **GLFW**, GL, X11 (`libglfw-dev`, etc. en Linux)
- GLAD está incluido en `third_party/glad` (no hace falta instalarlo)

## Compilación y ejecución

```bash
make            # compila en build/ y genera bin/ogl-app
./bin/ogl-app   # abre la ventana con el triángulo naranja
```

- `ESC` cierra la aplicación.
- La consola muestra versión de driver/vendor/GLSL al arrancar.
- Los errores de shaders se informan por consola (logs GLSL con tamaño dinámico).

> Los shaders se leen desde `assets/` en tiempo de ejecución: se pueden editar y
> volver a correr el programa **sin recompilar**.

## Estructura del proyecto

```
├── assets/
│   └── shaders/            # fuentes GLSL externos (triangle.vs / triangle.fs)
├── src/
│   ├── main.cpp            # app: ventana GLFW + triángulo (VAO/VBO + shaders)
│   └── core/
│       ├── ResourceManager.h    # cache de recursos (C++ puro, sin OpenGL)
│       └── ResourceManager.cpp
├── tests/
│   └── main_test_rm.cpp    # test standalone del ResourceManager
├── third_party/glad/       # cargador de funciones OpenGL
├── Makefile                # configuración del proyecto (usa Makefile.master)
├── Clinica-Practico-01.md          # apuntes: clínica de debugging (3 roturas)
└── ResourceManager-Practico-01.md  # apuntes: diseño y funcionamiento del módulo
```

## Test del ResourceManager

El enunciado pide un programa de prueba que demuestre el cacheo, compilable en
forma aislada con solo la biblioteca estándar:

```bash
g++ -std=c++17 -Wall -Wextra -I./src src/core/ResourceManager.cpp \
    tests/main_test_rm.cpp -o build/test_rm && ./build/test_rm
```

Demuestra: carga desde disco, caché (segundo pedido no relee), aviso ante clave
repetida con otros archivos, excepciones ante archivo faltante o clave inexistente,
y comportamiento de `clear()`.

## Módulo ResourceManager (Práctico 01 — Parte 2)

Cache de recursos en **C++ puro** (sin llamadas a OpenGL). Hoy administra fuentes
de shaders; su forma (*struct por tipo de recurso + load_\* + get_\* + mapa*) fue
elegida para ampliarlo más adelante con texturas y modelos.

Decisiones tomadas (documentadas también en el header):

| Situación | Comportamiento |
|---|---|
| Archivo/carpeta faltante | Excepción `std::runtime_error` con mensaje claro |
| Clave ya cargada, mismos archivos | Devuelve el caché sin leer disco |
| Clave ya cargada, archivos distintos | Warning por stderr; gana el caché |
| Retorno | Referencia `const` (inválida tras `clear()`) |

Ver `ResourceManager-Practico-01.md` para el detalle completo.

## Documentación del práctico

- [`Clinica-Practico-01.md`](Clinica-Practico-01.md) — las tres roturas del
  triángulo (atributo VAO, punto y coma del shader, vértice fuera de NDC), qué
  mostró cada una, qué no, y los conceptos detrás.
- [`ResourceManager-Practico-01.md`](ResourceManager-Practico-01.md) — diseño,
  decisiones, funcionamiento interno del caché y recetas de uso.

## Contexto: roadmap del proyecto integrador

Simulador de acrobacias aéreas (entrega final con defensa oral). Módulos por clase:
shaders ✔ → primitivas 3D/Mesh/Shader → matrices → aeronave → cámara → FDM/game loop →
terreno/HUD/texturas → circuito/maniobras/puntuación.

Requerimientos mínimos: escenario 3D, aeronave con actitud correcta, HUD (3
instrumentos), control vía teclado/joystick con FDM provisto, 3 cámaras, circuito
(5 checkpoints), detección de 2 maniobras y puntuación — más 2 mejoras extras.

Herramientas de IA: permitidas, declaradas y defendibles — hay que poder explicar,
romper y arreglar cualquier fragmento del código entregado.
