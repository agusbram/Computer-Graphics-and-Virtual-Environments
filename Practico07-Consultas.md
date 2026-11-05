# Práctico 07 — Consultas (preguntas y respuestas)

> **Nota (Práctico 07, LOD):** estas consultas describen la ventana única del
> Práctico 07. Ahora `Terrain` es un geometry clipmap de varios niveles (cada
> nivel tiene su propio `paso`, `centro`, `esquina` y `Mesh`); las preguntas
> sobre los niveles están en la sección 9 y siguientes de
> [`docs-clipmaps/Geometry-Clipmaps.md`](docs-clipmaps/Geometry-Clipmaps.md).

FAQ con las dudas que fueron surgiendo al leer `Terrain` y el `main`. Mismo
formato que `Practico02-Consultas.md`.

---

## Chuleta (resumen para refrescar)

- `mitad_ = (N·paso)/2` = **media extensión**: centra la ventana → `esquina = centro − mitad`.
- `esquina()` = **esquina (mundo)** de la grilla: ubica cada nodo (leer el heightmap) y arma el modelo.
- `template <typename FuenteAltura>` = **genérico**: sirve cualquier cosa que devuelva una altura (clase `Alturas`, lambda, imagen…). Desacopla generación de fuente.
- `~Terrain()` = destructor. `= delete` = **sin copia** porque `Terrain` **posee** un `Mesh` (dueño único).
- Miembros: **`datos_` = CPU** (recalcular), **`malla_` = GPU** (dibujar); `altura_terreno_` = **fija** (por eso se puede bajar); `centro_` = centro *snapped*; `primera_` = primer cuadro.
- `init`: **generar** (CPU) → `malla_.load` (**subir** a GPU) — dos pasos.
- `update`: centro *snapped* a múltiplos de `paso_`; **recalcula solo al cruzar celda**; arma `modelo_` (**horizontal sigue** al avión, **vertical fija**).
- `collect`: empaqueta un **RenderItem** (patrón `Aircraft`); **no dibuja**.
- `main`: `flight.position = 0` = **origen flotante** (el avatar se dibuja en el origen, el mundo relativo); `terreno.update` va **en el game loop** (1×/cuadro); `camara.update(0,…)` = objetivo en el origen; `static double ult` = **temporizador 1/s** del readout.

---

## A. La clase `Terrain`

### A1. ¿Por qué en el constructor se multiplica por `0.5` (`mitad_`)?

```cpp
, mitad_(0.5f * static_cast<float>(N) * paso)
```

El terreno mide **N celdas × `paso`** por lado → su extensión total es `N·paso`.
La **mitad** de esa extensión es `0.5·N·paso`. Se guarda como `mitad_` porque la
ventana se **centra** en el avión: si el centro está en `centro_`, la **esquina**
(la esquina de menor x,y) queda en `centro_ − mitad_`. Así el avión cae en el
medio de la grilla (puede dar la vuelta en todas las direcciones).

### A2. ¿Para qué sirve `esquina()`?

```cpp
glm::vec2 Terrain::esquina() const { return centro_ - glm::vec2(mitad_); }
```

Devuelve la **esquina del mundo donde arranca la grilla** (la posición de menor
x,y), en unidades de escena. Se usa para dos cosas:

1. **Generar los vértices / leer el heightmap**: el vértice (i,j) tiene como
   posición del mundo `esquina + (j·paso, i·paso)`. Sin la esquina no se sabría
   **dónde** del mundo está cada nodo (y el ruido daría un terreno distinto).
2. **La matriz de modelo**: para llevar la esquina al lugar relativo al avión.

### A3. ¿Qué es `template <typename FuenteAltura>`?

Es un **template** (plantilla): hace que `generar_malla_terreno` funcione con
**cualquier** tipo que "sepa devolver una altura", sin nombrar un tipo concreto:

```cpp
template <typename FuenteAltura>
MeshData generar_malla_terreno(unsigned N, float paso, const glm::vec2& esquina_,
                               const FuenteAltura& altura) { ... altura(x,z) ... }
```

Sirve para **desacoplar**: la generación de la malla solo necesita que le pasen
un "objeto invocable" que, dado `(x,z)`, devuelva la altura. Puede ser nuestra
clase `Alturas`, una lambda, o cualquier otra fuente (una imagen, un archivo…).
La función se **instancia** para el tipo que le pases. Es el equivalente a decir
"no me importa de dónde salen las alturas, solo que seas una función de altura".

### A4. ¿Qué hacen `~Terrain();` y los `= delete`?

```cpp
~Terrain();                                 // destructor
Terrain(const Terrain&) = delete;           // copiar: PROHIBIDO
Terrain& operator=(const Terrain&) = delete;
```

- **`~Terrain()`** es el **destructor**: libera lo que el objeto posee al morir.
  Acá no hay código propio porque el único dueño de recursos es `malla_` (`Mesh`),
  que se libera solo en su propio destructor.
- **`= delete`** **deshabilita** la copia (construir un `Terrain` a partir de
  otro, o asignarlo). ¿Por qué? Porque `Terrain` **posee** un `Mesh`, y `Mesh` es
  de **dueño único sin copia** (dos dueños liberarían el mismo VAO dos veces).
  Si se pudiera copiar el `Terrain`, se intentaría copiar su `Mesh` → error. Se
  borra la copia para que el compilador lo rechace de una forma clara en vez de
  fallar adentro de `Mesh`.

### A5. ¿Qué es cada miembro?

```cpp
unsigned N_;                 // celdas por lado
float    paso_;              // separación entre vértices (unidades de escena)
float    mitad_;             // media extensión de la ventana = (N/2)·paso
float    altura_terreno_;    // altitud FIJA del terreno en el mundo (no sigue al avión)
Alturas  alturas_;           // fuente de alturas (ruido), desacoplada
Mesh     malla_;             // la malla en la GPU (VAO/VBO/EBO)
MeshData datos_;             // los datos en CPU (vértices+índices): para recalcular y re-subir
glm::vec3 color_ {0.33f, 0.45f, 0.28f};  // color plano del terreno
glm::mat4 modelo_ {1.0f};    // matriz de modelo (la arma update, la usa collect)
glm::vec2 centro_ {0.0f};    // centro de la ventana (mundo), a saltos de celda
bool      primera_ {true};   // "¿es la primera actualización?"
```

- **`datos_` vs `malla_`**: `datos_` es la copia en **CPU** (de la que se leen/
  recalculan las alturas); `malla_` es la copia en **GPU** (la que se dibuja).
  Hay que actualizar las dos al mover la ventana (recalcular en CPU y re-subir).
- **`altura_terreno_`**: la altitud **fija** del terreno. Por eso el avión puede
  acercarse/bajar (ver A8).
- **`primera_`**: bandera de "primer cuadro" (ver A7).

### A6. ¿Por qué en `init()` se genera la malla y después se hace `malla_.load(datos_)`?

```cpp
datos_ = generar_malla_terreno(N_, paso_, esquina(), alturas_);  // 1) datos en CPU
malla_.load(datos_);                                             // 2) subir a la GPU
```

Son **dos pasos distintos**:

1. `generar_malla_terreno` **calcula los datos** (vértices + índices) en la CPU;
   es **código puro**, sin OpenGL.
2. `malla_.load(datos_)` **sube esos datos a la GPU** (crea el VAO/VBO/EBO).

Es la separación de siempre: **generar** (datos) vs **subir** (GPU). Si mañana se
cambia el método de altura, cambia el paso 1 y el paso 2 queda igual.

### A7. En `update()`: ¿cómo se calcula `nuevo`? ¿Qué es `primera_`?

```cpp
const glm::vec2 nuevo(std::round(pos_avion.x / paso_) * paso_,
                      std::round(pos_avion.y / paso_) * paso_);
if (primera_ || nuevo != centro_) { centro_ = nuevo; primera_ = false; recalcular_vertices(); }
```

- **`nuevo`** es la posición del avión **redondeada a múltiplos de `paso_`** (a
  "saltos de celda") en la horizontal. Al redondear, el centro cae siempre sobre
  un **nodo** del heightmap → cada vértice vuelve a caer sobre un nodo y el
  relieve no "repta". (Si siguiera al avión de forma continua, los vértices
  caerían **entre** nodos y habría que interpolar.)
- **`primera_`** es `true` hasta la primera llamada a `update()`. En `init()` se
  armó la malla con el centro en `(0,0)`, pero esa es provisoria: **la primera**
  `update()` tiene que recalcular sí o sí con el centro real. Después `primera_`
  queda `false` y **solo** se recalcula cuando el centro redondeado **cambia**
  (al cruzar una celda).
- **`recalcular_vertices()`** vuelve a calcular altura + normal de cada vértice y
  re-sube el VBO (ver la consulta sobre el VBO).

### A8. ¿Por qué se hace ese `translate` (la matriz de modelo)?

```cpp
modelo_ = glm::translate(glm::mat4(1.0f),
                         glm::vec3(esquina().x - pos_avion.x,   // horizontal (x)
                                   esquina().y - pos_avion.y,   // horizontal (y)
                                   altura_terreno_ - pos_avion.z)); // vertical (altura)
```

Es el **origen flotante** (Práctico 06): todo se dibuja **relativo al avión**. Los
vértices del terreno están en coordenadas locales de la ventana (0..N·paso); la
matriz de modelo los lleva a **su lugar relativo al avión**:

- **x, y** (horizontal): la esquina de la ventana **restada** la posición del
  avión. Como el centro sigue al avión, `esquina − pos_avion ≈ −mitad` → la
  ventana queda **centrada** en el avión.
- **z** (vertical): `altura_terreno_ − pos_avion.z`. Como `altura_terreno_` es
  **fija**, si el avión baja (`pos_avion.z` disminuye) esta resta se hace **menos
  negativa** → el terreno queda **menos abajo** → el avión se acerca y lo puede
  tocar. Si el terreno siguiera al avión en vertical, este término sería
  constante y nunca se podría bajar.

### A9. ¿Por qué `collect()`?

```cpp
void Terrain::collect(std::vector<RenderItem>& items) const {
    RenderItem item;
    item.mesh  = &malla_;
    item.model = modelo_;
    item.color = color_;
    items.push_back(item);
}
```

Es el **mismo patrón que `Aircraft::collect()`**: el terreno se empaqueta en un
`RenderItem` (malla + matriz de modelo + color) y lo agrega a la lista. Así el
`main` **no sabe** que existe un "terreno": solo recorre una **lista de cosas para
dibujar**. Es la separación "el que arma entrega una lista; el que dibuja solo la
recorre". `collect()` **no dibuja** (no llama a OpenGL), solo describe.

---

## B. El `main` (origen flotante y game loop)

### B1. `flight.position = 0` → ¿el avión no se mueve?

```cpp
const glm::vec3 pos_avion = flight.position;   // posición REAL (unidades)
terreno.update(pos_avion);                     // el terreno la necesita (ventana)
flight.position = glm::vec3(0.0f);             // el AVATAR se dibuja en el origen
avion.update(flight);
camara.update(glm::vec3(0.0f), ...);
```

- `flight.position` es la posición **real** del avión en la escena (sale del FDM).
- Antes de dibujar se guarda en `pos_avion` (el terreno la usa para mover la
  ventana) y **después se pone en 0**: el avión se **dibuja en el origen**.
- ¿Por qué? Es el **origen flotante** (Práctico 06): la cámara sigue al avión, así
  que en pantalla el avión **está siempre centrado**. En vez de dibujarlo en su
  posición real (~5000 m, que degradaría la precisión de float), se dibuja en el
  origen y **el mundo se dibuja relativo a él** (el terreno con su `translate`).
  El resultado visual es idéntico, pero con coordenadas chicas (sin z-fighting).
- O sea: **el avión no se "mueve" en pantalla** (porque la cámara lo sigue), pero
  **el mundo sí se mueve relativo a él**. Su posición real no se pierde: está en
  `pos_avion` (y en el FDM) para lo que la necesite (el terreno, y más adelante el
  resto de la escena).

### B2. Si el terreno es infinito, ¿no debería llamarse `terreno.update()` en un bucle siempre?

¡Sí, y se hace! `terreno.update(pos_avion)` está **dentro** del game loop
(`while (!glfwWindowShouldClose(window))`), así que se llama **una vez por
cuadro**. Lo que pasa es que **recalcula los vértices solo cuando la ventana se
corre una celda** (cuando el centro redondeado cambia); el resto de los cuadros
solo recalcula la matriz de modelo. Por eso el terreno es "infinito" sin costar
cada cuadro: la topología es fija y las alturas cambian solo al cruzar una celda.

### B3. ¿Por qué `camara.update(glm::vec3(0.0f), ...)`?

El **primer parámetro de `CameraSystem::update` es el OBJETIVO** (el punto al que
mira la cámara). Como el avión (el avatar) se dibuja en el **origen** (B1), el
objetivo es `(0,0,0)`. La cámara orbita **alrededor del origen**, donde quedó el
avión. Si el avión se dibujara en su posición real, acá iría `pos_avion`.

### B4. ¿Qué es el bloque del `static double ult …`?

```cpp
static double ult = -1.0;
if (ahora - ult >= 1.0) {
    ult = ahora;
    ... std::cout << "[vuelo] TAS=... " ...
}
```

Es un **temporizador de "una vez por segundo"**: `ult` guarda el instante del
**último** mensaje y `ahora` el actual. Cuando pasó al menos 1 s, imprime el estado
de vuelo y actualiza `ult`. Sin eso, imprimiría una vez por cuadro (~60/s) e
inundaría la consola. Es solo **feedback de depuración** (para ver que W/S cambian
la velocidad, que la altitud se mantiene, etc.).

---

## Recordatorio de build

El `Makefile` **no recompila al cambiar un `.h`** (no hay dependencias de headers).
Tras editar un encabezado: **`make clean && make`**.
