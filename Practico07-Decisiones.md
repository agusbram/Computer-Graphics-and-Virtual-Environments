# Práctico 07 — Terreno procedural: decisiones

Apunte de diseño del **Práctico 07**: cómo se genera una malla de terreno a
partir de un *heightmap* (alturas por ruido procedural), cómo se calculan sus
normales, cómo se activa la eliminación de superficies ocultas y cómo se
integran el terreno y la aeronave en la misma escena. Sigue el formato de los
apuntes anteriores.

---

## Objetivo

Agregar a la escena el **terreno** por el cual moverse: una malla poligonal con
relieve, **generada por programa** a partir de una fuente de alturas, dibujada
con **eliminación de superficies ocultas** (z-buffer + recorte de cara trasera)
y con **modo malla de alambre** para inspeccionarla. Es la primera malla del
proyecto que no se escribe a mano ni sale de una primitiva: sale de **datos**.

---

## Integración de la fuente de ruido (FastNoiseLite)

- Se copió **`FastNoiseLite.h`** (un solo encabezado, MIT) a
  **`third_party/FastNoiseLite/`** y se agregó esa carpeta a los `INC_DIRS` del
  `Makefile`. No hay nada que compilar/enlazar aparte.
- Ventaja: el relieve **no se repite**, no hay que cargar ningún archivo y la
  fuente de alturas se reduce a una llamada a función. Con la **semilla** se
  reproduce el mismo terreno entre sesiones.

---

## Parte 1 — La malla poligonal

Malla REGULAR de **N × N celdas** → **N+1 vértices por lado**. Para N=200:
**40.401 vértices** y **80.000 triángulos**.

```cpp
for (unsigned i = 0; i <= N; ++i)          // <= N: hay N+1 VÉRTICES por lado
    for (unsigned j = 0; j <= N; ++j)
        vertices.push_back({ j*paso, i*paso, altura(i,j) });

for (unsigned i = 0; i < N; ++i)           // < N: se recorren CELDAS
    for (unsigned j = 0; j < N; ++j) {
        const unsigned a = idx(i,j),   b = idx(i,j+1);
        const unsigned d = idx(i+1,j), c = idx(i+1,j+1);
        // 2 triángulos por celda, compartiendo la diagonal
    }
```

La generación es una **función libre** (`generar_malla_terreno<N>`) que recibe
N, el paso y la **fuente de alturas**: no usa OpenGL y solo devuelve datos
(desacopla la geometría del relieve).

### ¿Conviene indexar? Sí

Sin índices, cada triángulo listaría 3 vértices → 240.000 (cada vértice interior
se repite **6 veces**). Indexando, el vértice interior se guarda una sola vez
(40.401). La regla para compartir es **de sombreado** (que coincidan TODOS los
atributos), no geométrica: en la grilla el vértice interior tiene una sola
normal (superficie suave) → tupla única; en el cubo no (cada esquina necesita
una normal por cara).

### ⚠️ Winding: la guía usa Y-arriba, la escena es Z-arriba

La guía genera los vértices `{ j·paso, altura, i·paso }` (**Y = altura**) y
ordena los índices `a→d→c`. Como **nuestra escena es Z-arriba** (la aeronave se
modeló con Z arriba, Práctico 04), los vértices se generan `{ j·paso, i·paso,
altura }` (Z = altura) y, con el MISMO orden de la guía, las normales apuntaban
hacia **−Z** → el recorte de cara trasera descartaba TODO el terreno (no se veía
nada). Por eso se **invirtió el sentido** a `a→c→d` y `a→b→c`, que deja las
caras mirando hacia **+Z** (arriba). Verificado: con el recorte activado el
terreno se ve completo desde arriba y **no** se ve desde abajo.

---

## Parte 2 — Alturas, normales y escala

### Alturas (heightmap por ruido)

Clase `Alturas`, **desacoplada** de la generación de la malla (cambiarla por una
imagen o un archivo de texto se reduce a cambiar esa clase):

```cpp
float Alturas::operator()(float x, float z) const {
    return amplitud_ * ruido_.GetNoise(x, z);   // Perlin + FBm (5 octavas)
}
```

- **Semilla**: reproduce el mismo terreno. **Frecuencia**: tamaño de los rasgos
  grandes (0.005 → rasgos de ~200 unidades). **Octavas**: detalle fino encima.
- **Ojo**: la amplitud pedida **no** es la que se obtiene; sumar octavas
  normaliza la salida y en la práctica usa ~1/3 del rango. Se imprimió el
  mínimo/máximo una vez para fijar la escala, en vez de tanteos.

### Normales por diferencias centradas

Para `z = f(x,y)` (el terreno se apoya en X-Y), la normal hacia arriba es
`n = normalize(-∂f/∂x, -∂f/∂y, 1)`. Como la superficie es discreta, las derivadas
se estiman con los **nodos vecinos** (diferencias centradas; de un solo lado en
el borde). No se puede dejar la normal en cero (normalizar 0 es indefinido).
Se calculan ya en este práctico aunque la iluminación llega después (Unidad IX).

### Escala (decisión registrada)

- **Escala global: 1 unidad de escena = 10 m.** El modelo del avión (1 unidad)
  representa ~10 m (el real mide ~9 m), así todo queda coherente.
- **Separación entre vértices**: 2.5 u (25 m) → fija el detalle del relieve.
- **Tamaño de la grilla**: N = 200 → **5 km** de lado.
- **Amplitud vertical**: 60 u nominales (el real es ~1/3).
- **Despeje**: el avión vuela **40 u (~400 m) sobre el terreno**.

---

## Parte 3 — Eliminación de superficies ocultas

Dos mecanismos que en OpenGL son **dos líneas** (vienen apagadas por defecto):

```cpp
glEnable(GL_DEPTH_TEST);   // z-buffer: resuelve por FRAGMENTO quién está adelante
glEnable(GL_CULL_FACE);    // recorte de cara trasera: descarta por POLÍGONO
glCullFace(GL_BACK);       // qué se descarta (default)
glFrontFace(GL_CCW);       // antihorario = cara frontal (default)
```

**Clave**: OpenGL **no usa el atributo normal** para decidir qué cara descarta;
usa el **signo del área del triángulo ya proyectado** (el *winding*). Por eso hay
que respetar el orden de los vértices: **antihorario mirando la cara desde
afuera**. (Por eso el punto anterior del winding invertido en Z-arriba.)

### Cómo diagnosticar (de la guía, aplicado)

| Cambiar una línea | Qué significa |
|---|---|
| `glPolygonMode(..., GL_LINE)` | si el objeto no aparece en alambre, el problema es geometría/cámara/matriz, no el recorte |
| `glDisable(GL_CULL_FACE)` | si vuelven las caras, es *winding*, no geometría |
| `glFrontFace(GL_CW)` | si se invierte el síntoma, es inversión global (todo el bucle al revés) |
| (mirar los agujeros) | si son regulares (tablero), es inconsistencia dentro del bucle |

La corrección se hace **en el generador de la malla**, no con `glFrontFace`.

---

## Terreno "infinito" (el opcional) — ventana que sigue al avión

Estrategia: separar el **dato** (heightmap, estático) de la **geometría** (malla,
una "ventana" que sigue al avión).

- La **topología** (índices) se arma **una vez**.
- Al cruzar una celda, la ventana se corre **un paso** (múltiplo de la
  separación) y se recalculan **solo las alturas**.
- La ventana se mueve **a saltos**, no de forma continua: si siguiera al avión
  continuamente los vértices caerían **entre** nodos del heightmap y el relieve
  "reptaría" (la superficie parecería deslizarse). Con el centro redondeado a
  múltiplos del paso, cada vértice vuelve a caer sobre un nodo y el terreno queda
  fijo.
- **Optimización**: las alturas se calculan una vez por nodo en una grilla
  temporal y las normales se leen de ahí (si no, cada normal repetiría ~4
  lecturas del ruido y el costo se multiplicaría). Al moverse se re-sube solo el
  VBO (`Mesh::actualizar_vertices`, sin recrear el VAO/EBO).

---

## Integración en la escena y altitud del terreno

El avión (FDM) está a ~5000 m y la cámara orbita a pocas unidades. Si se dejara
la altitud real, el terreno (a nivel del mar) quedaría ~500 unidades abajo y no
se vería. Decisión (elegida): **el terreno se ubica a una ALTITUD FIJA en el
mundo**, igual a la altitud de *trim* menos un **despeje** chico (~15 u = 150 m).

- **La parte horizontal sigue al avión** (ventana infinita); **la vertical es
  fija**: como el terreno **no** sigue al avión en altura, si el avión desciende
  (con el elevador) el terreno queda cada vez más cerca → **se puede volar bajo y
  hasta tocar el terreno**. (Si el terreno siguiera al avión también en vertical,
  el despeje sería constante y nunca se podría bajar.)
- La **posición horizontal** la da la posición real del FDM; el terreno se dibuja
  **relativo al avión** (mismo origen flotante, para no arrastrar coordenadas
  grandes ni sufrir z-fighting).
- `near/far` de la cámara se agrandó (`far` 3000) porque el terreno mide ~500
  unidades.

## Modo malla de alambre

`glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` cambia cómo OpenGL **rasteriza** los
polígonos (no rellena, dibuja los lados). Se llama **una vez** (es estado del
driver) y aplica a los `glDraw*` siguientes. Está en el **callback de la tecla F**
(un comando discreto → callback, ver Práctico 06) y alterna relleno/alambre.
Sirve para verificar la grilla del terreno (`N+1` líneas por lado).

---

## Cómo se sabe que está bien (verificado)

1. **Con el recorte activado, el terreno se ve completo desde arriba**: ni
   desaparece ni le faltan parches. Verificado con captura (relleno + alambre).
2. **En modo alambre se ven N+1 líneas por lado y el terreno se dibuja hasta el
   borde**: verificado con captura (grilla completa).
3. **El relieve coincide con el heightmap** (no espejado ni rotado): el ruido se
   evalúa en la posición del mundo, y el orden del bucle (i, j) mapea a (y, x).
4. **Mirando desde abajo, con el recorte activado, el terreno NO se ve** (las
   caras miran hacia arriba): verificado poniendo la cámara por debajo (solo se
   ve el avión).
5. **El avión llega al borde y no pasa nada raro**: con el terreno **infinito**
   (ventana), el borde no se alcanza nunca (la ventana sigue al avión).

> **Lección de build (importante)**: el `Makefile` del proyecto **no recompila al
> cambiar un encabezado** (no hay dependencias de headers). Un cambio en un `.h`
> no se ve hasta hacer **`make clean && make`**. Nos pasó: el terreno seguía sin
> verse porque corría el `.o` viejo. Si algo no cambia tras editar un header,
> build limpio.

## Controles (agregado)

- `F` alterna relleno ↔ **malla de alambre** (terreno incluido).
- El resto igual que el Práctico 06 (flechas/Q-E/W-S, ESPACIO, mouse, ESC).

---

## Arquitectura

Se agrega el módulo **`Terrain`** (capa **Scene/sistemas**): genera la malla y
la mantiene; el resto del programa solo pide su `RenderItem` (como `Aircraft`).
`Alturas` queda como **infraestructura/datos** (fuente de alturas desacoplada).
La `Scene` real (terreno + modelos) sigue integrándose en `main` por ahora.
