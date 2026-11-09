# CGyAV — Proyecto OpenGL (template + Prácticos 01 a 08)

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
- **Práctico 06** — *FDM, game loop e input*: se integra la librería de
  dinámica de vuelo `dlfdm` (`libs/dlfdm`), se sincronizan los dos relojes con
  un **game loop de paso fijo + acumulador**, se amplía `InputHandler` con
  **teclado** (cuatro mandos con rampas + comandos discretos por callback) y se
  lleva la salida del FDM a la escena con `to_world()` (NED → escena).
- **Práctico 07** — *Terreno procedural*: malla de terreno indexada (N×N celdas)
  generada a partir de un *heightmap* de **ruido procedural** (FastNoiseLite),
  con **normales** por diferencias centradas y **eliminación de superficies
  ocultas** (z-buffer + recorte de cara trasera); **terreno infinito** (la
  ventana sigue al avión) y modo **malla de alambre** (tecla F).
  - *Niveles de detalle (LOD)* (parte del Práctico 07): **geometry clipmaps**
    (Losasso & Hoppe 2004). La ventana única se reemplaza por 5 grillas
    anidadas de 128×128 celdas con paso que se duplica por nivel (51 km de
    lado). Etapa 1: anillos con hueco que siguen al avión. Etapa 2:
    **geomorphing** en el vertex shader + triángulos de área cero (sin grietas
    ni *popping*).
  - *Niebla* (mejora leve, adelanto del Práctico 09): niebla lineal en el
    fragment shader que oculta el borde del terreno; se mejora en el P09.
- **Práctico 08** — *Texturas y cielo*: imágenes decodificadas con
  **stb_image** en `ResourceManager` y subidas a la GPU por el módulo
  **`Texture`** (DSA, 4 etapas, mipmaps); **pasto** en el terreno con
  coordenadas de textura en coordenadas de mundo (continuas entre los niveles
  del LOD); verificación de las `(u, v)` de las primitivas con una **imagen de
  prueba asimétrica**; **chapa** CC0 en el avión; **cielo** con un *cubemap*
  (módulo `Skybox`, vista sin traslación, método `z = w`) o un **degradé 1D**;
  y **suelo continuado** en el fondo para tapar la banda del "problema de la
  altitud".

---

## Requisitos

- Compilador C++ con soporte **C++17** (`g++`)
- `make`
- Librerías de desarrollo: **GLFW**, GL, X11 (`libglfw-dev`, etc. en Linux)
- **glm** (`libglm-dev`; solo encabezados) — tipos y operaciones de álgebra lineal
- GLAD está incluido en `third_party/glad` (no hace falta instalarlo)
- **stb_image** (v2.30) está incluido en `third_party/stb` (un solo encabezado;
  no hace falta instalarlo)

## Compilación y ejecución

```bash
make            # compila en build/ y genera bin/ogl-app (incluye dlfdm y FastNoiseLite)
./bin/ogl-app   # aeronave volando por el FDM sobre el terreno procedural
```

- `ESC` cierra.
- **Teclado**: flechas = cabeceo/alerones; `Q`/`E` = timón; `W`/`S` = potencia;
  `ESPACIO` = pausa; `F` = wireframe; `L` = colores por nivel del clipmap;
  `M` = geomorphing on/off; `N` = niebla on/off. (La tecla representa el bastón: ↑ baja la
  nariz, como en un avión real.)
- **Teclado (Práctico 08)**: `U` = imagen de prueba UV; `T` = apariencia del avión
  (camuflaje / metal gris / colores de las piezas);
  `I` = filtrado del pasto (con mipmaps / sin mipmaps / "trampa"); `K` = cielo
  cubemap ↔ degradé; `G` = suelo continuado; `1` = solo el nivel fino del LOD;
  `RePág`/`AvPág` = altura sobre el terreno (para medir la banda).
- **Mouse**: botón izquierdo arrastrado = orbitar (yaw/pitch); botón derecho
  arrastrado (vertical) = acercar/alejar.
- La consola muestra versión de driver/vendor/GLSL al arrancar.
- Los errores de shaders se informan por consola (logs GLSL con tamaño dinámico).

> Los shaders se leen desde `assets/` en tiempo de ejecución: se pueden editar y
> volver a correr el programa **sin recompilar**.

## Estructura del proyecto

```
├── assets/
│   ├── shaders/            # fuentes GLSL externos:
│   │                       #   terreno.*  (morph del LOD + pasto + niebla)       — P08
│   │                       #   aeronave.* (color, chapa o imagen de prueba)       — P08
│   │                       #   fondo.*    (cielo cubemap/degradé + suelo continuado) — P08
│   │                       #   solid.*    (OBSOLETOS: la versión del P07, ya no se cargan)
│   └── textures/           # P08: suelo/ (pasto), cielo/ (cubemap), avion/ (chapa CC0),
│                           #      prueba/ (imagen UV) + LICENCIAS.md (avion/: chapa CC0 + camuflaje generado)
├── libs/
│   └── dlfdm/              # librería de dinámica de vuelo (FDM, Práctico 06)
├── src/
│   ├── main.cpp            # app: game loop (paso fijo) + FDM + terreno + cielo + cámara
│   └── core/
│       ├── MeshData.h          # tupla Vertex {position, normal, tex_coords, altura_gruesa} (glm)
│       ├── Mesh.h/.cpp         # dueño del VAO/VBO/EBO (DSA 4.5, 4 atributos; re-subida de vértices e índices)
│       ├── Shader.h/.cpp       # dueño del programa linkeado + uniforms (DSA)
│       ├── Primitives.h/.cpp   # cube(), cylinder(), cone() y sphere() (opcional)
│       ├── Aircraft.h/.cpp     # modelo de la aeronave: piezas + locales + pose
│       ├── RenderItem.h        # item de dibujo {mesh, model, color, morph, textura, escala_uv} (P04; LOD P07; textura P08)
│       ├── Texture.h/.cpp      # dueño único de una textura (2D / cubemap / 1D, DSA, mipmaps) (P08)
│       ├── Skybox.h/.cpp       # fondo: cubo + cubemap + degradé + suelo continuado (P08)
│       ├── CameraData.h        # par de matrices {view, projection} (Práctico 05)
│       ├── CameraCommand.h     # deltas de órbita {yaw, pitch, distancia}
│       ├── CameraSystem.h/.cpp # cámara orbital: lookAt + perspective
│       ├── FlightData.h        # datos de vuelo compartidos + to_world() (P06)
│       ├── InputHandler.h/.cpp # mouse + teclado -> cámara y comandos del FDM
│       ├── Terrain.h/.cpp      # terreno procedural + LOD por geometry clipmaps (P07)
│       ├── ResourceManager.h   # cache de recursos (C++ puro, sin OpenGL): shaders e imágenes (P08)
│       └── ResourceManager.cpp
├── tests/
│   ├── main_test_rm.cpp        # test standalone del ResourceManager
│   └── main_test_primitives.cpp# verificación de winding por producto vectorial
├── third_party/
│   ├── glad/               # cargador de funciones OpenGL
│   ├── FastNoiseLite/      # ruido procedural para el heightmap (Práctico 07)
│   └── stb/                # stb_image.h: decodificar PNG/JPG (Práctico 08)
├── tools/
│   ├── generar_imagen_prueba_uv.py # genera la imagen de prueba asimétrica (P08)
│   └── generar_camuflaje.py        # genera el camuflaje del avión (tileable, P08)
├── Makefile                # configuración del proyecto (usa Makefile.master)
├── Clinica-Practico-01.md          # apuntes: clínica de debugging (3 roturas)
├── ResourceManager-Practico-01.md  # apuntes: diseño y funcionamiento del módulo
├── Practico02-Modulos.md          # apuntes: decisiones y cómo se sabe que está bien
├── Practico02-Consultas.md        # FAQ: offsetof, primitives, depth test, clínica, etc.
├── Practico02-Portabilidad.md     # por qué el diseño permite agregar figuras nuevas
├── Practico03-Decisiones.md       # apuntes: tupla, paramétricas, uniforms, matriz de modelo
├── Practico04-Decisiones.md       # apuntes: despiece de la aeronave, módulo Aircraft, pose
├── Practico05-Decisiones.md       # apuntes: cámara orbital, perspectiva, input por mouse
├── Practico06-Decisiones.md       # apuntes: FDM, game loop de paso fijo, teclado, to_world
├── Practico07-Decisiones.md       # apuntes: terreno procedural (heightmap, culling, ventana infinita)
├── Practico07-Consultas.md        # FAQ del terreno: mitad, esquina, template, dueño único, etc.
├── docs-clipmaps/                 # Práctico 07: Geometry-Clipmaps.md (LOD, etapas 1 y 2, niebla leve) + img/
├── Practico08-Decisiones.md       # apuntes: texturas, Texture, cielo (cubemap/degradé), suelo continuado
├── Practico08-Consultas.md        # FAQ del P08: vista sin traslación, xyww, culling, tesela, uniforms...
├── docs-p08/img/                  # capturas del P08 (cielo, prueba UV, mipmaps, banda)
├── Practico09-Pendientes.md       # qué queda para el P09 y por qué no se hizo en el P08
└── Arquitectura-Proyecto.md       # arquitectura en capas (destino del proyecto, para más adelante)
```

## Test del ResourceManager

El enunciado pide un programa de prueba que demuestre el cacheo, compilable en
forma aislada (sin OpenGL). Desde el Práctico 08 necesita también el encabezado
de stb_image:

```bash
g++ -std=c++17 -Wall -Wextra -I./src -I./third_party/stb src/core/ResourceManager.cpp \
    tests/main_test_rm.cpp -o build/test_rm && ./build/test_rm
```

Demuestra: carga desde disco, caché (segundo pedido no relee), aviso ante clave
repetida con otros archivos, excepciones ante archivo faltante o clave inexistente,
y comportamiento de `clear()`. **Práctico 08:** carga de imágenes en RGBA, caché
de imágenes, **volteo vertical** (la fila 0 con flip es la última sin flip),
aviso ante la misma clave con otro flip, `unload_image` y error ante una imagen
inexistente.

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

## FDM, game loop e input (Práctico 06)

La escena ahora la mueve un **modelo de dinámica de vuelo** (`dlfdm`), con un
**game loop de paso fijo** y control por **teclado**.

| Pieza | Cambio |
|---|---|
| `libs/dlfdm` (nuevo) | librería del FDM (se integra con su `dlfdm.mk`, como glad) |
| `FlightData.h` (nuevo) | datos de vuelo compartidos (posición de escena + φ/θ/ψ) y `to_world()` (NED → escena) |
| `Aircraft` | `update(FlightData)` compone la pose con los ejes/sentidos adaptados al FDM |
| `InputHandler` | se amplía con **teclado**: los 4 mandos por polling con rampas + `controls()`/`set_controls()`; el mouse sigue dando el `CameraCommand` |
| `main.cpp` | game loop con **acumulador** (paso fijo `dt=1/120`, tope 0.25 s), FDM, `to_world()`, callbacks de pausa/wireframe |

- **Game loop**: `frame_dt` se acumula (con tope) y se consume a pasos enteros
  de `dt`; el resto queda para el cuadro siguiente (`simulado + acc = reloj`).
- **FDM**: `fdm.update()` adentro del while de paso fijo; `getState()` una vez
  por cuadro. Arranca desde el **trim** (estado + comandos) a 5000 m / 150 m/s.
- **`to_world()`**: NED → escena (`x=−norte, y=este, z=−abajo`); la actitud se
  compone con `T·Rz(−ψ)·Ry(θ)·R_{eje nariz}(φ)` (verificado: θ>0 sube la nariz,
  φ>0 baja el ala derecha, ψ>0 gira a la derecha).
- **Controles**: flechas = cabeceo/alerones, Q/E = timón, W/S = potencia,
  ESPACIO = pausa, F = wireframe. La tecla representa el bastón (convención
  registrada). Ver `Practico06-Decisiones.md`.

## Terreno procedural (Práctico 07)

Se agrega el **terreno** por el cual moverse: una malla poligonal con relieve
generada a partir de un *heightmap* de ruido procedural.

| Pieza | Cambio |
|---|---|
| `third_party/FastNoiseLite` (nuevo) | header del ruido procedural (Perlin + octavas) para el heightmap |
| `Terrain.h/.cpp` (nuevo) | módulo del terreno: `generar_malla_terreno()` (función libre), `Alturas` (fuente desacoplada) y la clase `Terrain` con la ventana infinita |
| `main.cpp` | crea el terreno, activa **`GL_CULL_FACE`**, lo dibuja relativo al avión y le pasa la posición del FDM |

- **Malla indexada** de N×N celdas (N+1 vértices por lado): N=200 → 40.401
  vértices y 80.000 triángulos (4 mallas y 7 piezas del avión vs 1 malla del
  terreno).
- **Normales** por diferencias centradas (para la iluminación, Unidad IX).
- **Eliminación de superficies ocultas**: `GL_DEPTH_TEST` + `GL_CULL_FACE`
  (recorte de cara trasera por *winding*). La escena es **Z-arriba**, así que el
  orden de índices se invirtió respecto de la guía (que usa Y-arriba) para que
  las caras miren hacia +Z.
- **Escala** (decisión): 1 unidad de escena = 10 m; terreno de 5 km. El terreno
  está a una **altitud fija** (no sigue al avión en vertical), a ~15 u (~150 m)
  por debajo de la altitud de trim: el avión vuela **bajo** y, con el elevador,
  puede descender hasta tocarlo.
- **Terreno infinito** (opcional): la malla es una ventana que sigue al avión a
  saltos de celda (solo se recalculan las alturas). **Modo alambre** con `F`.
  Ver `Practico07-Decisiones.md`.

## Niveles de detalle: geometry clipmaps (Práctico 07) + niebla (mejora leve)

El terreno pasa de **una** ventana uniforme (5 km, paso 25 m) a un **geometry
clipmap**: varias grillas regulares **anidadas** y centradas en el avión, con
detalle fino cerca y grueso lejos (material: diapositiva de clase, paper de
Losasso & Hoppe 2004 y GPU Gems 2 cap. 2).

| Pieza | Cambio |
|---|---|
| `Terrain.h/.cpp` | la clase guarda un `std::vector<Nivel>` (paso, centro, hueco, `Mesh`, `MeshData`, modelo); `collect` entrega **un `RenderItem` por nivel**; calcula `z_c` y los triángulos de área cero |
| `Terrain(...)` | nuevo argumento `niveles`: `Terrain(128, 2.5, 5, 20, altura, 1337, 0.005)`; `N` = celdas por nivel (múltiplo de 4), `paso` = el del nivel fino |
| `Mesh.h/.cpp` | `actualizar_indices()` (re-sube el EBO al correrse el hueco) y atributo **location 3** |
| `MeshData.h` | `Vertex::altura_gruesa` (la `z_c` del paper) |
| `RenderItem.h` | campo `morph` → uniform `uMorph = (paso, N/2, w)`; 0 = sin morph |
| `solid.vs` | geomorphing `z' = mix(z, z_c, α)` y distancia a la cámara (`vDistancia`) |
| `solid.fs` | niebla lineal `mix(uColor, cielo, f)` |
| `main.cpp` | uniforms `uMorph` y de niebla, color de cielo único (`kCielo`), teclas L/M/N |

- **Etapa 1**: 5 niveles de 128×128 celdas, paso 2.5·2^l u. Cada nivel sigue al
  avión a saltos de 2·paso_l (alineación: su borde cae sobre vértices del nivel
  siguiente) y el nivel grueso no dibuja el hueco que cubre el fino → anillos.
  ~131.000 triángulos para 51 km de lado (antes 80.000 para 5 km). Solo se
  recalcula el nivel que se movió.
- **Etapa 2**: **geomorphing** (cerca del borde exterior el nivel fino toma la
  forma del grueso, α en el vertex shader; ancho w = N/10) + **triángulos de
  área cero** en el borde → sin grietas (T-junctions) ni *popping*.
- **Niebla** (mejora leve; el tema es del **Práctico 09**, donde se va a
  mejorar): lineal de 300 u (3 km) a 1800 u (18 km), del mismo color que el
  fondo; oculta el borde del terreno y el plano `far`.
- Verificación: `L` (anillos de colores), `F` (malla), `M` (con/sin morph:
  aparecen/desaparecen las grietas), `N` (con/sin niebla: aparece el borde).
  Ver [`docs-clipmaps/Geometry-Clipmaps.md`](docs-clipmaps/Geometry-Clipmaps.md).

> **Build**: el `Makefile` no recompila al cambiar un encabezado → tras editar un
> `.h` hay que hacer **`make clean && make`**.

## Texturas y cielo (Práctico 08)

| Pieza | Cambio |
|---|---|
| `third_party/stb` (nuevo) | `stb_image.h` v2.30: decodifica PNG/JPG; `STB_IMAGE_IMPLEMENTATION` solo en `ResourceManager.cpp` |
| `ResourceManager` | `ImageData` (siempre RGBA) + `load_image(key, file, flip)` / `get_image` / `unload_image`; flip fijado en **cada** carga (estado global de stb) |
| `Texture.h/.cpp` (nuevo) | dueño único (mismas reglas que `Mesh`); 4 etapas en DSA; `load_2d` (mipmaps `⌊log₂⌋+1`), `load_cubemap`, `load_1d`; muestreo **cambiable** (`set_muestreo`) |
| `RenderItem` | `const Texture* textura` (no dueño; `nullptr` = color plano) + `escala_uv` |
| `Terrain` | `(u, v)` = posición en el **mundo** módulo 5120 u (continua entre niveles del LOD y con el fondo) |
| `Aircraft` | `AparienciaAvion` (tecla `T`): **camuflaje** (textura generada, tileable, con detalle de chapa), **metal gris** (chapa CC0 que tiñe un gris) o colores del P04; `U` = imagen de prueba |
| `Skybox.h/.cpp` (nuevo) | cubo + cubemap + degradé 1D + suelo continuado; vista sin traslación; método (b) `z = w` + `GL_LEQUAL`; sin recorte de caras |
| Shaders | `terreno.*`, `aeronave.*` y `fondo.*` (un programa por tipo de objeto); `solid.*` quedan obsoletos |
| `main.cpp` | carga de imágenes/texturas (y su liberación de la CPU), dos listas de dibujo, fondo al último, teclas U/T/I/K/G/1/RePág/AvPág |

- **Tesela del pasto:** 50 m (1024 teselas sobre el clipmap de 51,2 km).
- **Escena Z-arriba:** el cubemap se lee con la dirección rotada `(x, z, −y)`. El suelo continuado usa el plano `z = altura del terreno`.
- **Datos calculados del cubemap:** el color del horizonte (es el color de la niebla) y la dirección del sol pintado (≈ 30° de elevación, para el P09).
- **Banda de la altitud:** con el LOD empieza a notarse a ~700 m sobre el terreno; con solo el nivel fino, desde el vuelo normal. El suelo continuado la tapa a cualquier altura.

Ver [`Practico08-Decisiones.md`](Practico08-Decisiones.md) y [`Practico08-Consultas.md`](Practico08-Consultas.md). Lo que queda para el Práctico 09 está en [`Practico09-Pendientes.md`](Practico09-Pendientes.md).

## Documentación de los prácticos

- [`Arquitectura-Proyecto.md`](Arquitectura-Proyecto.md) — arquitectura en capas
  del proyecto (aplicación / sistemas / datos), destino del simulador.
- [`Practico08-Decisiones.md`](Practico08-Decisiones.md) — texturas y cielo:
  `ImageData`/`load_image` (flip, RGBA, `unload_image`), módulo `Texture` (4 etapas
  DSA, mipmaps, la trampa), `(u, v)` del terreno en coordenadas de mundo, imagen de
  prueba asimétrica, chapa del avión, `Skybox` (cubemap, vista sin traslación,
  método `z = w`, Z-arriba), degradé 1D, problema de la altitud (medido) y suelo
  continuado, con capturas.
- [`Practico08-Consultas.md`](Practico08-Consultas.md) — FAQ del P08: por qué se
  quita la traslación, `xyww` + `GL_LEQUAL`, culling del cubo, LOD vs suelo
  continuado, tesela, `uniform`/`binding`, `GL_RGBA8`, día/noche, arena, imagen de
  prueba, y las "cuestiones a pensar" de la guía.
- [`Practico09-Pendientes.md`](Practico09-Pendientes.md) — qué quedó preparado para
  el Práctico 09 y qué se dejó para él (código GLSL común, Phong, niebla
  exponencial, gamma, día/noche) y por qué.
- [`docs-clipmaps/Geometry-Clipmaps.md`](docs-clipmaps/Geometry-Clipmaps.md) —
  Práctico 07 (LOD): geometry clipmaps explicados con las figuras del paper (regiones,
  anillos, transición), implementación de las etapas 1 y 2, niebla (mejora leve), FAQ del
  código (alineación, `uMorph`, `mix`, `RenderItem`, etc.) y cómo probar cada cosa.
- [`Practico07-Decisiones.md`](Practico07-Decisiones.md) — terreno procedural:
  malla indexada, heightmap por ruido, normales, recorte de cara trasera (winding),
  ventana infinita (terreno infinito) y escala.
- [`Practico07-Consultas.md`](Practico07-Consultas.md) — FAQ del terreno: `mitad_`,
  `esquina()`, el template de la fuente de alturas, dueño único, cada miembro,
  `init()`/`update()`/`collect()`, origen flotante y el game loop.
- [`Practico06-Decisiones.md`](Practico06-Decisiones.md) — FDM, game loop de
  paso fijo con acumulador, teclado en `InputHandler` y `to_world()`.
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
cámara ✔ → FDM/game loop/input ✔ → terreno + LOD (clipmaps) ✔ → niebla leve ✔ (mejorar en P09) → texturas + cielo ✔ (P08) → iluminación + niebla + gamma (P09; mejora propia: curvatura de la Tierra) → HUD → circuito/maniobras/puntuación.

Requerimientos mínimos: escenario 3D, aeronave con actitud correcta, HUD (3
instrumentos), control vía teclado/joystick con FDM provisto, 3 cámaras, circuito
(5 checkpoints), detección de 2 maniobras y puntuación — más 2 mejoras extras.

Herramientas de IA: permitidas, declaradas y defendibles — hay que poder explicar,
romper y arreglar cualquier fragmento del código entregado.
