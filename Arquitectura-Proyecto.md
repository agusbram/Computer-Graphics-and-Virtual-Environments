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
| `Shader` | infraestructura | compila (P09: varios archivos por etapa + `#version`), linkea y setea uniforms | decidir qué se dibuja |
| `Mesh` | infraestructura | posee VAO/VBO/EBO, carga bytes | saber qué representa |
| `Texture` (P08) | infraestructura | posee una textura de la GPU (2D / cubemap / 1D): 4 etapas DSA, mipmaps, muestreo | leer archivos (eso es de `ResourceManager`) |
| `primitives` | infraestructura | genera geometría a partir de medidas | saber dónde va |
| `Aircraft / Scene` | sistemas | qué modelos hay y dónde está cada uno | emitir draw calls |
| `Renderer` | sistemas | emite draw calls, gestiona estado de GPU | física, lógica |
| `FDM` | sistemas | física de vuelo | — |
| `InputHandler` | sistemas | teclado / joystick | — |
| `CameraSystem` | sistemas | 3 vistas, matrices | — |
| `DiaNoche` (P09) | sistemas | reloj del día: la hora, la dirección y el color del sol | dibujar, OpenGL |
| `HUD` | sistemas | instrumentos, overlay 2D | — |
| `GameLogic` | sistemas | checkpoints, maniobras | — |

### Estructuras de datos compartidas (solo datos)

| Estructura | Contenido |
|---|---|
| `FlightData` | pitch, roll, hdg (rumbo), etc. |
| `CircuitState` | waypoints (circuito / checkpoints) |
| `CameraData` | datos de la cámara |
| `Iluminacion` (P09) | sol (dirección, color), luz ambiente, niebla (color y visibilidad), modo de verificación del suelo (tecla B), Phong/Blinn-Phong (tecla R), corrección gamma (tecla C) y modo del halo del sol (tecla Y): un solo lugar, lo leen los tres programas |
| `Material` (P09) | coeficientes de Phong de una pieza `{kd, ks, ns}`: viaja en el `RenderItem` en lugar del color |
| `ShaderSource` (P09) | fuente de un programa: lista de archivos por etapa (lo llena `ResourceManager`, lo compila `Shader`) |

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
- **Iluminación y niebla** (Práctico 09): código GLSL común ✔,
  terreno iluminado (Phong) ✔, avión iluminado (material en el `RenderItem`,
  matriz normal calculada al dibujar) ✔, niebla exponencial (terreno, avión y
  cielo) ✔, corrección gamma en el código común ✔, opcional halo del sol ✔,
  opcional día/noche: paso 1 ✔ (el sistema `DiaNoche`, sin dependencias: solo
  glm); siguen los pasos 2 a 4 y la curvatura.
  Ver `Practico09-Decisiones.md`.
- **HUD**: el módulo `HUD`.
- **Circuito / maniobras**: `CircuitState` + `GameLogic`.
- **`Renderer`** (pendiente, para **después** del Práctico 09): sacar el dibujo
  de `main`. Ver la sección siguiente, "Estado actual frente a la propuesta".

No conviene crear los módulos vacíos antes de tiempo: cada capa llega con su
práctico.

---

## Estado actual frente a la propuesta (revisión del 25-nov, Práctico 09)

Revisión de **todo el proyecto** hasta la Parte 4 del Práctico 09, contra la
filmina de arquitectura del 11 de septiembre (`Clase 11 septiembre.md`). Se
revisó **qué incluye cada archivo** (`#include`) y **qué módulos llaman a
OpenGL**.

**Resumen.** La base sigue la propuesta: los datos compartidos, la
infraestructura y los sistemas están separados y **ningún sistema depende de
otro**. Lo que falta es la capa del **`Renderer`**: hoy **dibuja `main`**, y la
filmina dice que *"main orquesta todo, gestiona los módulos y ejecuta el
programa. **No dibuja ni crea modelos**"*.

> **Decisión (25-nov):** se deja anotado para hacerlo **más adelante**. Primero
> se termina el Práctico 09 (Parte 5, corrección gamma, y los opcionales: halo
> del sol → día/noche → curvatura de la Tierra).

### Lo que cumple

| Regla de la filmina | Cómo está en el código | |
|---|---|---|
| Sistemas **sin dependencias cruzadas** | `Aircraft`, `Terrain`, `CameraSystem`, `InputHandler` y `Skybox` no se incluyen entre sí. Se comunican por **datos**: `InputHandler` → `CameraCommand` → `CameraSystem`; el FDM → `FlightData` → `Aircraft`. | ✔ |
| `ResourceManager`: carga y cachea, **nada gráfico directo** | 0 llamadas a OpenGL. | ✔ |
| `Shader`: compila, linkea y setea uniforms; **no decide qué se dibuja** | P09: varios archivos por etapa y `#version`. No sabe qué objeto dibuja. | ✔ |
| `Mesh`: VAO/VBO/EBO; **no sabe qué representa** | Recibe un `MeshData` y lo sube. | ✔ |
| `primitives`: genera geometría; **no sabe dónde va** | Solo depende de `MeshData`; 0 llamadas a OpenGL. | ✔ |
| `Aircraft` / `Scene`: qué modelos hay y dónde; **no emite draw calls** | `Aircraft` y `Terrain` solo llenan listas de `RenderItem` con `collect()`; 0 llamadas a OpenGL. | ✔ (salvo `Skybox`, ver abajo) |
| El modelo **encapsulado en una clase** (para el día del `.obj`) | `Aircraft`: cambiar las primitivas por un `.obj` solo toca `init()`. | ✔ |
| Estructuras compartidas **solo datos** | `CameraData`, `CameraCommand`, `Material`, `MeshData`, `ShaderSource`, `RenderItem`. | ✔ (salvo `Iluminacion` y `FlightData`, ver abajo) |
| HUD, GameLogic, CircuitState | Todavía no existen, a propósito: "cada capa llega con su práctico". | — |

### Mapa de dependencias (quién incluye a quién, 25-nov)

```
Datos (no incluyen nada del proyecto):
  CameraData · CameraCommand · Material · MeshData · ShaderSource · FlightData(*)
  Iluminacion(*) → Shader          ← no es "solo datos" (punto 3)

Infraestructura:
  Mesh → MeshData        Primitives → MeshData
  Shader → ShaderSource  ResourceManager → ShaderSource
  Texture.cpp → ResourceManager    ← solo por ImageData (punto 4)

Sistemas (ninguno incluye a otro sistema):
  Aircraft     → FlightData, Mesh, RenderItem, Texture
  Terrain      → Mesh, MeshData, RenderItem, Texture
  CameraSystem → CameraData, CameraCommand
  InputHandler → CameraCommand
  Skybox       → Iluminacion, Mesh, ResourceManager, Shader, Texture
  DiaNoche     → (nada del proyecto: solo glm)     ← agregado el 26-nov (día/noche)

Aplicación:
  main.cpp → todos                  ← y además DIBUJA (punto 1)
```

### Dónde no la sigue

**1. No hay `Renderer`: dibuja `main`.** Es lo más importante.

- `main.cpp` tiene **1098 líneas**; la función `main()` sola, unas **630**. Es
  el "`main.cpp` gigante" que este apunte quería evitar desde el principio.
- En `main` están las ubicaciones de los uniforms de cada pieza
  (`UniformsPieza`, `UniformsEscena`), `dibujar_items()` con el
  `glDrawElements`, el `glClear` y el estado de la GPU (prueba de profundidad,
  recorte de caras traseras, MSAA).
- Ya estaba anotado arriba ("el loop de dibujo vive todavía en `main`, así lo
  pide el Práctico 04"), pero desde entonces `main` creció con cada práctico.

**2. El `Skybox` se dibuja solo.** `Skybox::draw()` hace su propio
`glDrawElements` y cambia el estado de la GPU (`glDepthFunc(GL_LEQUAL)`,
`glDisable(GL_CULL_FACE)`). Es parte de **Scene**, que según la tabla "NO
emite draw calls". Fue una decisión del P08 ("es dueño de su cubo, sus
texturas y su shader"). Según la filmina, debería **entregar** lo que hay que
dibujar y que lo dibuje el `Renderer`.

**3. `Iluminacion` no es "solo datos".** Está en la tabla de estructuras
compartidas, pero `Iluminacion.h` incluye `Shader.h` y declara
`buscar_uniforms_iluminacion()` y `enviar_iluminacion()`: buscar y enviar
uniforms es trabajo del `Renderer`. También tiene `sol_con_elevacion()`, una
cuenta.

**4. Detalles menores.**

- `FlightData` incluye `dlfdm/defines.h` y trae la conversión `to_world()`
  (NED → escena): un dato que conoce al FDM.
- `ImageData` vive en `ResourceManager.h`, así que `Texture.cpp` incluye al
  `ResourceManager` solo por ese struct. `ShaderSource` y `MeshData` ya tienen
  su propio encabezado.
- Las teclas de depuración y de opciones (B, X, R, J, N, K, L, RePág/AvPág…)
  están en el `key_callback` de `main` (unas 130 líneas), con su estado en
  `AppContext`. El módulo de teclado de la filmina es el `InputHandler`, que
  hoy solo maneja los mandos de vuelo y la cámara.

### Plan para hacerlo (a futuro)

De lo más barato y seguro a lo más caro:

| Paso | Qué se hace | Riesgo |
|---|---|---|
| 1 | **`Iluminacion` solo datos**: el struct queda en `Iluminacion.h`, sin `Shader.h`. `buscar_uniforms_iluminacion` / `enviar_iluminacion` pasan al `Renderer` (o a un archivo propio mientras no exista). | Bajo |
| 2 | **`ImageData.h` propio**, como `MeshData.h` y `ShaderSource.h`: `Texture` deja de incluir al `ResourceManager`. | Bajo |
| 3 | **`FlightData` sin `dlfdm`**: `to_world()` pasa al lado del FDM (un adaptador donde se usa el solver). | Bajo |
| 4 | **Módulo `Renderer`**: recibe las listas de `RenderItem` (malla + matriz de modelo + material + textura), la **cámara** y la **luz**, y dibuja. Se lleva `UniformsPieza`, `UniformsEscena`, `dibujar_items()`, el envío de la iluminación, el `glClear` y el estado de la GPU. `main` queda en: inicializar, correr el game loop, pedir los `collect()` y pasarle las listas al `Renderer`. Es exactamente lo que dice la filmina: el `Renderer` "**no sabe** cómo se construye el avión ni el cielo: sólo recibe la malla a dibujar + la matriz modelo + la cámara". | Medio: toca **todo** el dibujo |
| 5 | **El fondo lo dibuja el `Renderer`**: el `Skybox` entrega su cubo, sus texturas y sus parámetros (`ParametrosFondo`), y el `Renderer` tiene un camino aparte para el fondo, porque necesita su propio estado de profundidad y de caras. | Medio |
| 6 | **Teclas al `InputHandler`**: las de opciones pasan a ser comandos (como `CameraCommand`), y `main` solo los aplica. | Bajo, pero largo |

**Cómo verificar después de cada paso:** la misma escena tiene que verse
igual. Probar los tres programas (terreno, avión, fondo) y todas las teclas de
verificación del P09: B, X, R, J, N, K y L. Los tests (`test_rm`,
`test_primitives`, `test_normales`) tienen que seguir pasando.
