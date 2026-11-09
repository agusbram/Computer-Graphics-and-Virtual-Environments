# Arquitectura del proyecto (constancia para más adelante)

Apunte de referencia con la arquitectura en capas propuesta en la clase
(11 de septiembre, filmina de arquitectura) para el proyecto integrador
(*Simulador de Acrobacias Aéreas*). Queda como **constancia en memoria**:
todavía NO se aplica al código (se adaptará cuando lleguen los prácticos de
cámara/FDM/etc.), pero sirve para saber hacia dónde se va y no terminar con
un `main.cpp` gigante.

---

## Visión de capas (de abajo hacia arriba)

```
  ┌─────────────────────────────────────────────────────────────┐
  │  Capa de aplicación                                         │
  │  (punto de entrada, game loop, orquestación)                │
  │  └── Application · main.cpp                                 │
  └─────────────────────────────────────────────────────────────┘
                            ▲
  ┌─────────────────────────────────────────────────────────────┐
  │  Capa de sistemas                                           │
  │  (subsistemas independientes, SIN dependencias cruzadas)    │
  │  ├── FDM            física de vuelo                         │
  │  ├── InputHandler   teclado / joystick                      │
  │  ├── CameraSystem   3 vistas, matrices                      │
  │  ├── Scene          terreno, skybox, modelo                 │
  │  ├── HUD            instrumentos, overlay 2D                │
  │  ├── GameLogic      checkpoints, maniobras                  │
  │  └── Renderer       pipeline OpenGL, draw calls, estado GPU │
  └─────────────────────────────────────────────────────────────┘
                            ▲
  ┌─────────────────────────────────────────────────────────────┐
  │  ResourceManager (transversal)                              │
  │  shaders · texturas · modelos                               │
  └─────────────────────────────────────────────────────────────┘
                            ▲
  ┌─────────────────────────────────────────────────────────────┐
  │  Estructuras de datos compartidas (SOLO datos, sin lógica)  │
  │  ├── FlightData     pitch, roll, hdg…                       │
  │  ├── CircuitState   waypoints…                              │
  │  └── CameraData                                             │
  └─────────────────────────────────────────────────────────────┘
```

### Lectura

1. **Abajo de todo** está la *estructura de datos compartidas*: son datos
   planos, sin lógica, que viajan entre capas.
2. **Arriba de los datos** está el *ResourceManager* (transversal): toca disco
   y provee shaders, texturas y modelos por nombre a quien los pida.
3. **Arriba** viene la *capa de sistemas*: subsistemas independientes entre sí,
   sin dependencias cruzadas.
4. **Arriba de todo**, la *capa de aplicación* (`Application · main.cpp`):
   inicializa, corre el game loop y orquesta.

---

## Detalle de cada módulo

| Módulo | Capa | Responsabilidad | NO hace |
|---|---|---|---|
| `Application · main.cpp` | aplicación | inicializa todo, corre el game loop, orquesta | lógica de negocio |
| `ResourceManager` | transversal | carga y cachea shaders, texturas y modelos por nombre | nada gráfico directo |
| `Shader` | infraestructura | compila, linkea y setea uniforms | decidir qué se dibuja |
| `Mesh` | infraestructura | posee VAO/VBO/EBO, carga bytes | saber qué representa |
| `Texture` (P08) | infraestructura | posee una textura de la GPU (2D / cubemap / 1D): 4 etapas DSA, mipmaps, muestreo | leer archivos (eso es de `ResourceManager`) |
| `primitives` | infraestructura | genera geometría a partir de medidas | saber dónde va |
| `Aircraft / Scene` | sistemas | qué modelos hay y dónde está cada uno | emitir draw calls |
| `Renderer` | sistemas | emite draw calls, gestiona estado de GPU | física, lógica |
| `FDM` | sistemas | física de vuelo | — |
| `InputHandler` | sistemas | teclado / joystick | — |
| `CameraSystem` | sistemas | 3 vistas, matrices | — |
| `HUD` | sistemas | instrumentos, overlay 2D | — |
| `GameLogic` | sistemas | checkpoints, maniobras | — |

### Estructuras de datos compartidas (solo datos)

| Estructura | Contenido |
|---|---|
| `FlightData` | pitch, roll, hdg (rumbo), etc. |
| `CircuitState` | waypoints (circuito / checkpoints) |
| `CameraData` | datos de la cámara |

### Criterio para módulos nuevos (por funcionalidad)

- **Input / física** → todo lo que toca hardware de entrada o el FDM.
- **Lógica / CG** → todo lo que implementa un requerimiento del programa o de
  computación gráfica.
- **Transversal** → orquestación y pipeline de GPU.
- **Infraestructura / datos** → recursos y estructuras compartidas, sin lógica.

---

## Cómo encaja con lo que ya existe (Prácticos 01–04)

La arquitectura es el **destino**; el código actual ya da pasos hacia ella:

- `Aircraft` (Práctico 04) ≈ la parte "modelo" de **Scene**: qué modelos hay y
  dónde está cada uno. Encapsula la composición para que el día del `.obj` solo
  cambie `init()`.
- `RenderItem` + `Aircraft::collect()` ≈ el **contrato que consumirá el
  `Renderer`**: una lista de piezas ya transformadas, listas para dibujar.
- `Shader` / `Mesh` / `ResourceManager` / `primitives` ≈ infraestructura ya
  separada (la base de la tabla de arriba).
- El loop de dibujo vive todavía en `main` (así lo pide el Práctico 04).

### Cuándo se aplica

- **Cámara (Unidad VII)** ✔ — ya implementado en el Práctico 05: `CameraSystem`
  (cámara orbital, `CameraData`/`CameraCommand`) e `InputHandler` (mouse). El
  `Renderer` real y las "3 vistas" del sistema de cámaras quedan para más
  adelante.
- **FDM / game loop** ✔ — ya implementado en el Práctico 06: la librería
  `dlfdm` (FDM, caja negra), `InputHandler` con teclado, `FlightData` como
  estructura compartida y el game loop de paso fijo en `main`.
- **Terreno (Unidad VIII)** ✔ — ya implementado en el Práctico 07: el módulo
  `Terrain` (malla generada por heightmap de ruido, normales, recorte de cara
  trasera, ventana infinita) y `Alturas` (fuente de alturas desacoplada). Es la
  parte "terreno" de `Scene`.
- **LOD del terreno** ✔ — parte del Práctico 07: `Terrain` pasa a ser un
  **geometry clipmap** (varios niveles anidados, un `RenderItem` por nivel,
  geomorphing en `solid.vs`). Como mejora leve se agrega **niebla** lineal en
  `solid.fs` (tema del Práctico 09, donde se mejorará). La interfaz
  `init/update/collect` no cambió: el `main` sigue sin conocer los niveles.
  Ver `docs-clipmaps/Geometry-Clipmaps.md`. (Desde el Práctico 08 ese código
  vive en `terreno.vs` / `terreno.fs`.)
- **Texturas y cielo** ✔ — Práctico 08:
  - `ResourceManager` carga **imágenes** (`ImageData`, stb_image): es su rol de
    "transversal: shaders · texturas · modelos".
  - `Texture` es infraestructura, igual que `Mesh`: el dueño del recurso de GPU.
  - `Skybox` es la parte **"skybox" de `Scene`**: cubemap o degradé, más el suelo
    continuado. Es dueño de su cubo, sus texturas y su shader.
  - `RenderItem` suma un puntero **no dueño** a la textura: el contrato con el
    futuro `Renderer` sigue siendo "una lista de piezas listas para dibujar".
  - Hay **un programa por tipo de objeto** (terreno, aeronave, fondo). `main`
    elige con cuál dibujar cada lista, que es justamente lo que un `Renderer`
    haría.

  Ver `Practico08-Decisiones.md`.
- **Iluminación y niebla** (Práctico 09): código GLSL común, Phong, materiales y
  gamma. Ver `Practico09-Pendientes.md`.
- **HUD**: el módulo `HUD`.
- **Circuito / maniobras**: `CircuitState` + `GameLogic`.

No conviene crear los módulos vacíos antes de tiempo: cada capa llega con su
práctico.
