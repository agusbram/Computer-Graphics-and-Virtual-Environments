# CGyAV — Proyecto OpenGL (template + Prácticos 01, 02, 03, 04 y 05)

Proyecto base de **Computación gráfica y ambientes virtuales (0494)** — CRUC-IUA.
Template de aplicación OpenGL moderna (core profile 4.6) con GLFW + GLAD sobre el
que se desarrollan los prácticos del proyecto integrador
(*Simulador de Acrobacias Aéreas*):

- **Práctico 01** — triángulo con shaders en archivos + módulo `ResourceManager`.
- **Práctico 02** — *Del triángulo a la malla*: cubo indexado (24 vértices / 36
  índices, un color por cara) dibujado con los módulos `Mesh`, `Shader` y
  `primitives`; el `main.cpp` queda sin recursos propios de OpenGL.
- **Práctico 03** — *Primitivas paramétricas y matriz de modelo*: tupla con
  normal y UV (sale el color), `cylinder()`/`cone()` generados por parámetros,
  uniform en `Shader` (color y matriz de modelo) y una escena con cuatro
  piezas.
- **Práctico 04** — *Armado de la aeronave*: un solo modelo compuesto por
  varias primitivas (nariz, fuselaje, cola, alas y empenajes) encapsulado en
  el módulo `Aircraft`, con matrices locales fijas y una pose
  (posición + orientación) recalculada por cuadro; verificado con un cabeceo
  oscilante alrededor del centro de gravedad.
- **Práctico 05** — *Cámara en la escena*: la "caja negra" `uAjuste` se
  reemplaza por matrices de vista y proyección en perspectiva (`glm::lookAt` +
  `glm::perspective`), con una cámara orbital (`CameraSystem`) controlable con
  el mouse (`InputHandler`) y proyección recalculada al redimensionar.

---

## Requisitos

- Compilador C++ con soporte **C++17** (`g++`)
- `make`
- Librerías de desarrollo: **GLFW**, GL, X11 (`libglfw-dev`, etc. en Linux)
- **glm** (`libglm-dev`; solo encabezados) — tipos y operaciones de álgebra lineal
- GLAD está incluido en `third_party/glad` (no hace falta instalarlo)

## Compilación y ejecución

```bash
make            # compila en build/ y genera bin/ogl-app
./bin/ogl-app   # escena con la aeronave y cámara orbital en perspectiva
```

- `ESC` cierra la aplicación.
- **Mouse**: botón izquierdo arrastrado = orbitar (yaw/pitch); botón derecho
  arrastrado (vertical) = acercar/alejar.
- La consola muestra versión de driver/vendor/GLSL al arrancar.
- Los errores de shaders se informan por consola (logs GLSL con tamaño dinámico).

> Los shaders se leen desde `assets/` en tiempo de ejecución: se pueden editar y
> volver a correr el programa **sin recompilar**.

## Estructura del proyecto

```
├── assets/
│   └── shaders/            # fuentes GLSL externos (solid.vs / solid.fs, triangle.*)
├── src/
│   ├── main.cpp            # app: aeronave + cámara orbital (sin recursos propios de GL)
│   └── core/
│       ├── MeshData.h          # tupla Vertex {position, normal, tex_coords} (glm)
│       ├── Mesh.h/.cpp         # dueño del VAO/VBO/EBO (DSA 4.5, 3 atributos)
│       ├── Shader.h/.cpp       # dueño del programa linkeado + uniforms (DSA)
│       ├── Primitives.h/.cpp   # cube(), cylinder(), cone() y sphere() (opcional)
│       ├── Aircraft.h/.cpp     # modelo de la aeronave: piezas + locales + pose
│       ├── RenderItem.h        # item de dibujo {mesh, model, color} (Práctico 04)
│       ├── CameraData.h        # par de matrices {view, projection} (Práctico 05)
│       ├── CameraCommand.h     # deltas de órbita {yaw, pitch, distancia}
│       ├── CameraSystem.h/.cpp # cámara orbital: lookAt + perspective
│       ├── InputHandler.h/.cpp # mouse por polling -> CameraCommand
│       ├── ResourceManager.h   # cache de recursos (C++ puro, sin OpenGL)
│       └── ResourceManager.cpp
├── tests/
│   ├── main_test_rm.cpp        # test standalone del ResourceManager
│   └── main_test_primitives.cpp# verificación de winding por producto vectorial
├── third_party/glad/       # cargador de funciones OpenGL
├── Makefile                # configuración del proyecto (usa Makefile.master)
├── Clinica-Practico-01.md          # apuntes: clínica de debugging (3 roturas)
├── ResourceManager-Practico-01.md  # apuntes: diseño y funcionamiento del módulo
├── Practico02-Modulos.md          # apuntes: decisiones y cómo se sabe que está bien
├── Practico02-Consultas.md        # FAQ: offsetof, primitives, depth test, clínica, etc.
├── Practico02-Portabilidad.md     # por qué el diseño permite agregar figuras nuevas
├── Practico03-Decisiones.md       # apuntes: tupla, paramétricas, uniforms, matriz de modelo
├── Practico04-Decisiones.md       # apuntes: despiece de la aeronave, módulo Aircraft, pose
├── Practico05-Decisiones.md       # apuntes: cámara orbital, perspectiva, input por mouse
└── Arquitectura-Proyecto.md       # arquitectura en capas (destino del proyecto, para más adelante)
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

## Primitivas paramétricas y matriz de modelo (Práctico 03)

Cambios sobre los módulos del Práctico 02:

| Pieza | Cambio |
|---|---|
| `MeshData.h` | `Vertex` pasa a `{position, normal, tex_coords}` (glm); **sale el color** |
| `Mesh` | `load()` arma **3 atributos** (0=posición, 1=normal, 2=UV) con `offsetof` |
| `primitives` | se agregan `cylinder(radio, largo, gajos, anillos)`, `cone(radio, conicidad, gajos, anillos)` y `sphere(radio, gajos, anillos)` (opcional de la guía) — generación paramétrica, eje Y, ángulos en radianes, N+1 vértices por anillo, solo lateral |
| `Shader` | se agregan `set_uniform` (`mat4`/`vec3`/`float`) con DSA y `loc()` para cachear la ubicación |
| shaders | 3 atributos de entrada + uniforms `uModel`, `uAjuste`, `uColor` |
| `main.cpp` | escena con 4 piezas, cada una con su matriz de modelo (glm) y su color (uniform) |

El color deja de ser un dato por vértice y viaja como **uniform** (un valor por
objeto); la normal y las UV se calculan y guardan aunque todavía no se usen
(las consumen la iluminación y las texturas, más adelante). Ver
`Practico03-Decisiones.md`.

### Test de las primitivas

Verificación por cálculo del orden de los índices (sin dibujar nada): el
producto vectorial `(B−A)×(C−A)` de cada triángulo debe apuntar al mismo lado
que la normal de sus vértices.

```bash
g++ -std=c++17 -Wall -Wextra -I./src src/core/Primitives.cpp \
    tests/main_test_primitives.cpp -o build/test_primitives && ./build/test_primitives
```

## Armado de la aeronave (Práctico 04)

La escena del Práctico 03 se reemplaza por **un solo modelo**: la aeronave,
compuesta por varias primitivas y encapsulada en el módulo `Aircraft`.

| Pieza | Cambio |
|---|---|
| `RenderItem.h` (nuevo) | `{const Mesh* mesh, glm::mat4 model, glm::vec3 color}` — lo único que se necesita para dibujar una pieza ya transformada |
| `Aircraft.h/.cpp` (nuevo) | dueño de las mallas y las matrices: `init()` arma piezas y matrices locales UNA vez; `update(pos, angulos)` recalcula solo la pose; `collect(items)` entrega la lista lista para dibujar |
| `main.cpp` | por cuadro: `update()` (cabeceo oscilante de verificación) → `collect()` → dibujar cada item con `uModel = vista · item.model` |

- Sistema del modelo: origen en la **nariz**, X+ hacia la cola, Y+ ala derecha,
  Z+ arriba; medidas relativas al largo del fuselaje (L = 1).
- Despiece: nariz (cono), fuselaje (cilindro), cola (cono), ala izquierda y
  derecha (placas), estabilizador horizontal y deriva (placas): **7 piezas con
  4 mallas distintas** (las cuatro placas comparten la malla del cubo; el ala
  derecha es la misma malla trasladada, sin escalado negativo).
- Pose: `Mpose = T(pos) · R(ángulos) · T(−ref)`, con el punto de referencia a
  **0.438·Lf** desde la nariz (posición de la imagen de la guía); cada pieza
  se dibuja con `Mpose · Mlocal`.
- Verificación: cabeceo oscilante (`20° · sin(t·0.75)`) — todas las piezas
  giran juntas alrededor del centro de gravedad. Ver `Practico04-Decisiones.md`.

## Cámara en la escena (Práctico 05)

La "caja negra" `uAjuste` se reemplaza por **vista** y **proyección** en
perspectiva, y se agrega una cámara orbital controlable con el mouse.

| Pieza | Cambio |
|---|---|
| `solid.vs` | `uAjuste` se parte en `uView` + `uProjection`: `gl_Position = uProjection · uView · uModel · v` |
| `CameraData.h` (nuevo) | `{glm::mat4 view, projection}` — lo único que sale del módulo de cámara |
| `CameraCommand.h` (nuevo) | deltas de órbita `{yaw_delta, pitch_delta, dist_delta}` |
| `CameraSystem.h/.cpp` (nuevo) | cámara orbital (yaw/pitch/distancia), `glm::lookAt` + `glm::perspective`; `set_viewport()` recalcula la proyección |
| `InputHandler.h/.cpp` (nuevo) | mouse por polling → traduce píxeles a radianes → `CameraCommand` |
| `main.cpp` | por cuadro: `input.update()` → `camara.update()` → dibujar; callback de resize → `set_viewport` |

- Proyección en **perspectiva**: `glm::perspective(45°, aspect, 0.1, 100)`;
  la cuarta fila queda en `(0,0,−1,0)` (`w = −z`), así los objetos lejanos se
  achican (antes, con `uAjuste`, no había división por w).
- Cámara orbital: el eje polar de la órbita es **Z** (la vertical de la
  escena); `pitch` acotado a ±81°, distancia a [2, 40]; el `up` es `(0,0,1)`.
- Mouse: botón izquierdo = orbitar, botón derecho (vertical) = zoom.
- La proyección se recalcula al redimensionar (callback con `glfwSetWindowUserPointer`).
  Ver `Practico05-Decisiones.md`.

## Documentación de los prácticos

- [`Arquitectura-Proyecto.md`](Arquitectura-Proyecto.md) — arquitectura en capas
  del proyecto (aplicación / sistemas / datos), destino del simulador; se aplica
  cuando lleguen la cámara y el FDM.
- [`Practico05-Decisiones.md`](Practico05-Decisiones.md) — cámara orbital,
  proyección en perspectiva, manejo del mouse y callback de redimensionado.
- [`Practico04-Decisiones.md`](Practico04-Decisiones.md) — despiece de la
  aeronave, sistema de referencia, módulo `Aircraft`, la pose con su punto de
  referencia y cómo se verifica la rotación conjunta.
- [`Practico03-Decisiones.md`](Practico03-Decisiones.md) — tupla nueva, primitivas
  paramétricas (cilindro/cono/esfera), uniforms en `Shader`, matriz de modelo, decisiones
  y cómo se verifica el orden de los índices.
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
shaders ✔ → primitivas 3D/Mesh/Shader ✔ → matrices ✔ (matriz de modelo) → aeronave ✔ →
cámara ✔ → FDM/game loop → terreno/HUD/texturas → circuito/maniobras/puntuación.

Requerimientos mínimos: escenario 3D, aeronave con actitud correcta, HUD (3
instrumentos), control vía teclado/joystick con FDM provisto, 3 cámaras, circuito
(5 checkpoints), detección de 2 maniobras y puntuación — más 2 mejoras extras.

Herramientas de IA: permitidas, declaradas y defendibles — hay que poder explicar,
romper y arreglar cualquier fragmento del código entregado.
