# CGyAV — Proyecto OpenGL (template + Prácticos 01 y 02)

Proyecto base de **Computación gráfica y ambientes virtuales (0494)** — CRUC-IUA.
Template de aplicación OpenGL moderna (core profile 4.6) con GLFW + GLAD sobre el
que se desarrollan los prácticos del proyecto integrador
(*Simulador de Acrobacias Aéreas*):

- **Práctico 01** — triángulo con shaders en archivos + módulo `ResourceManager`.
- **Práctico 02** — *Del triángulo a la malla*: cubo indexado (24 vértices / 36
  índices, un color por cara) dibujado con los módulos `Mesh`, `Shader` y
  `primitives`; el `main.cpp` queda sin recursos propios de OpenGL.

---

## Requisitos

- Compilador C++ con soporte **C++17** (`g++`)
- `make`
- Librerías de desarrollo: **GLFW**, GL, X11 (`libglfw-dev`, etc. en Linux)
- GLAD está incluido en `third_party/glad` (no hace falta instalarlo)

## Compilación y ejecución

```bash
make            # compila en build/ y genera bin/ogl-app
./bin/ogl-app   # abre la ventana con el cubo indexado (3 caras de 3 colores)
```

- `ESC` cierra la aplicación.
- La consola muestra versión de driver/vendor/GLSL al arrancar.
- Los errores de shaders se informan por consola (logs GLSL con tamaño dinámico).

> Los shaders se leen desde `assets/` en tiempo de ejecución: se pueden editar y
> volver a correr el programa **sin recompilar**.

## Estructura del proyecto

```
├── assets/
│   └── shaders/            # fuentes GLSL externos (solid.vs / solid.fs, triangle.*)
├── src/
│   ├── main.cpp            # app: ventana GLFW + cubo (sin recursos propios de GL)
│   └── core/
│       ├── MeshData.h          # tupla Vertex + malla plana (sin OpenGL)
│       ├── Mesh.h/.cpp         # dueño del VAO/VBO/EBO (DSA 4.5, load/clear)
│       ├── Shader.h/.cpp       # dueño del programa linkeado (compile_from_source)
│       ├── Primitives.h/.cpp   # generadores de mallas: cube() (24/36, color por cara)
│       ├── ResourceManager.h   # cache de recursos (C++ puro, sin OpenGL)
│       └── ResourceManager.cpp
├── tests/
│   └── main_test_rm.cpp    # test standalone del ResourceManager
├── third_party/glad/       # cargador de funciones OpenGL
├── Makefile                # configuración del proyecto (usa Makefile.master)
├── Clinica-Practico-01.md          # apuntes: clínica de debugging (3 roturas)
├── ResourceManager-Practico-01.md  # apuntes: diseño y funcionamiento del módulo
├── Practico02-Modulos.md          # apuntes: decisiones y cómo se sabe que está bien
├── Practico02-Consultas.md        # FAQ: offsetof, primitives, depth test, clínica, etc.
└── Practico02-Portabilidad.md     # por qué el diseño permite agregar figuras nuevas
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

## Módulos de la malla (Práctico 02)

El triángulo del Práctico 01 se reemplaza por un cubo indexado, repartido en
módulos que **poseen** sus recursos de OpenGL:

| Módulo | Posee | Contrato |
|---|---|---|
| `Mesh` | VAO, VBO, EBO | `load(MeshData)` arma el VAO (2 atributos entrelazados, EBO por ranura dedicada); `count()` = cantidad de índices |
| `Shader` | programa linkeado | `compile_from_source(vs, fs)` compila+linkea; `bool` + log del driver, objeto vacío si falla |
| `primitives` | (no posee nada) | `cube(sx, sy, sz)` genera 24 vértices/36 índices con color plano por cara, en antihorario desde afuera |
| `ResourceManager` | strings en caché | lee los fuentes del disco; no compila |

Cada interfaz documenta sus decisiones en el header (copia borrada, mover deja el
origen en cero, `clear()` repetible, `offsetof` para los atributos, cubo centrado
con colores fijos, etc.). Ver `Practico02-Modulos.md`.

## Documentación de los prácticos

- [`Practico02-Modulos.md`](Practico02-Modulos.md) — decisiones de los módulos,
  cómo se sabe que el cubo está bien (consola 24/36, 3 caras visibles, sin
  `glDelete*` en main), y la clínica del práctico (romper el dibujo).
- [`Practico02-Consultas.md`](Practico02-Consultas.md) — FAQ con las explicaciones
  completas: `offsetof`, `primitives`, depth test, la clínica de `glDrawArrays`,
  los parámetros de `cube()` y la suma de `base` en los índices.
- [`Practico02-Portabilidad.md`](Practico02-Portabilidad.md) — por qué el diseño
  por módulos permite dibujar cualquier figura nueva cambiando una sola línea
  (con el ejemplo verificado de `triangle()`).
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
