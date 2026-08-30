# Práctico 02 — Consultas del código (FAQ)

Apunte de consultas surgidas mientras se repasaba el código del Práctico 02
(*Del triángulo a la malla*): qué significa cada pieza, por qué está así y qué
hace realmente la clínica. Las respuestas son las explicaciones completas de la
revisión del código, en orden de consulta.

---

## Índice

1. [¿Qué hace `offsetof`?](#1-qué-hace-offsetof)
2. [El módulo `primitives` explicado](#2-el-módulo-primitives-explicado)
3. [¿Qué hace el depth test y qué debería mostrar?](#3-qué-hace-el-depth-test-y-qué-debería-mostrar)
4. [La clínica del práctico: ¿por qué sucede eso?](#4-la-clínica-del-práctico-por-qué-sucede-esto)
5. [¿Por qué "lee el buffer" en la clínica?](#5-por-qué-lee-el-buffer-en-la-clínica)
6. [Los parámetros de `cube(scale_x, scale_y, scale_z)`](#6-los-parámetros-de-cubescale_x-scale_y-scale_z)
7. [¿Por qué se suman `base + 0 … base + 3` en los índices?](#7-por-qué-se-suman-base--0--base--3-en-los-índices)

---

## 1. ¿Qué hace `offsetof`?

Es una **macro de C++** (de la cabecera `<cstddef>`) que devuelve, **en tiempo de
compilación**, el desplazamiento en bytes de un campo dentro de una estructura,
medido desde el inicio de la estructura.

```cpp
struct Vertex { float px, py, pz; float r, g, b; };  // 6 floats = 24 bytes
offsetof(Vertex, r) == 12
```

El `Vertex` queda así en memoria:

| campo | byte de inicio |
|---|---|
| `px` | 0 |
| `py` | 4 |
| `pz` | 8 |
| `r`  | 12 |
| `g`  | 16 |
| `b`  | 20 |

En `Mesh::load()` se usa como el *relativeoffset* de cada atributo del VAO: le dice
a OpenGL dónde arranca cada atributo dentro del mismo buffer entrelazado.

```cpp
glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, px)); // posicion -> byte 0
glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, r));  // color   -> byte 12
```

La alternativa sería escribirlos a mano (`0` y `12`), como en el triángulo del
Práctico 01 (`3 * sizeof(float)` a mano). El motivo de usar `offsetof`:

- **No se recalculan a mano.** Si mañana `Vertex` gana un campo en el medio (por
  ejemplo una normal entre la posición y el color), los desplazamientos cambian y,
  con valores fijos a mano, habría que acordarse de tocar este código. Con
  `offsetof` se recalculan solos.
- **Costo cero en runtime**: es una constante que se resuelve en compilación, no
  una función que se ejecuta.

Resumen: `offsetof(Tipo, campo)` = el "byte de arranque" del campo dentro del
`struct`, calculado por el compilador; se lo pasamos a OpenGL para que sepa dónde
está cada atributo en el buffer.

---

## 2. El módulo `primitives` explicado

`primitives::cube()` **no dibuja nada ni toca OpenGL**: es un **generador de
datos**. Devuelve un `MeshData`, que son dos listas: la de vértices y la de
índices. La consigna del enunciado es justamente que los 24 vértices no se
escriben a mano en el `main`; se obtienen llamando a esta función.

### Cómo se modela cada cara

Cada cara se describe con un pequeño struct interno (`FaceSpec`):

```cpp
struct FaceSpec {
    int axis;    // 0 = X, 1 = Y, 2 = Z
    int sign;    // +1 cara positiva, -1 cara negativa
    int u_axis;  // primer eje tangente
    int v_axis;  // segundo eje tangente
};
```

- `axis` + `sign` identifican la **normal exterior** de la cara. Ej.: `axis = 2`,
  `sign = +1` → la cara que mira hacia `+z`.
- `u_axis` y `v_axis` son los dos ejes que quedan *tangentes* a esa cara (los que
  quedan "planos" sobre su superficie), elegidos en un orden tal que el producto
  vectorial `unitU × unitV` dé exactamente la normal exterior:

```
  frente    (+z):  X × Y  = +Z      atrás    (-z):  Y × X  = -Z
  derecha   (+x):  Y × Z  = +X      izquierda (-x): Z × Y  = -X
  arriba    (+y):  Z × X  = +Y      abajo    (-y):  X × Z  = -Y
```

Con ese orden garantizado, recorrer las esquinas de la cara en el orden
`(u−, v−), (u+, v−), (u+, v+), (u−, v+)` produce caras **en sentido antihorario
mirando la cara desde afuera**. Ese criterio se fija ahora y no se cambia: de él
depende el futuro *back-face culling* del teórico (qué cara se dibuja y cuál no).

### El loop que arma los datos

Por cada una de las 6 caras hace dos cosas:

1. **Empuja 4 vértices** (uno por esquina). Las coordenadas se arman así:
   - en el **eje del cubo** (`axis`): `sign · 0.5 · scale[eje]` → fija la posición
     de la cara (por eso plata un "0.5" en el eje normal);
   - en los **ejes tangentes** (`u` y `v`): recorre las 4 esquinas con `±0.5 ·
     scale`, y así se cubren los cuatro vértices del cuadrado.
   - Al vértice se le asigna el **color de su cara**.

2. **Empuja 6 índices** (ver punto 7 del índice): dos triángulos por cara,
   `(0,1,2)` y `(0,2,3)`, corriéndole la `base` según la cara.

### ¿Por qué 24 vértices y no 8?

Porque en OpenGL **el color es parte del vértice**: no existe "color por cara"
nativo. La regla de clase dice *un vértice se comparte si y solo si coinciden
todos sus atributos*. Como cada cara tiene su color distinto, las esquinas **no se
comparten entre caras**: una misma esquina física del cubo tiene 3 caras
distintas, o sea 3 tuplas (posición, color) distintas. Resultado:

```
6 caras × 4 esquinas = 24 vértices únicos
6 caras × 6 índices  = 36 índices        (dos triángulos por cara)
```

Los índices "mezclan" esos 24 vértices: cada triángulo toma 3 de la misma cara y
como los 3 tienen el mismo color, la cara queda **plana** (un solo color). Esa es
la gracia de la malla indexada: se escriben 24 vértices en el VBO en vez de 36.

Resumen: `primitives` es un generador de datos puros (sin GPU); describe cada cara
por su normal exterior y recorre sus esquinas siempre en el mismo sentido para
dejar todo listo para el culling futuro.

---

## 3. ¿Qué hace el depth test y qué debería mostrar?

### El problema que resuelve

Los triángulos se dibujan **en el orden en que los manda el programa**, uno encima
de otro. En un cubo rotado, algunos triángulos de las caras *traseras* se dibujan
*demasiado tarde* (después que las delanteras) y, por ser los últimos en pasarse,
se escribirían encima: las caras traseras le "ganarían" a las delanteras aunque
estén más lejos. Ese efecto de *paint order* es el que vemos cuando todo se ve
transparente o desordenado.

### Cómo funciona

OpenGL tiene un buffer aparte (el **depth buffer**, uno por píxel) que guarda la
profundidad `z` de lo que se dibujó en cada píxel. Con el test activo, por cada
fragmento nuevo se compara su `z` contra la que está guardada:

- si el nuevo es **más cercano** a la cámara, se escribe (y se actualiza la `z`);
- si el nuevo es **más lejano**, se descarta.

Así el orden en que se dibujan los triángulos **deja de importar**: el que gana es
siempre el más cercano.

### Las dos líneas que se necesitan

```cpp
glEnable(GL_DEPTH_TEST);                                  // UNA vez, antes del loop
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // CADA cuadro: dos bits
```

- `glEnable(GL_DEPTH_TEST)` activa el mecanismo una sola vez.
- `glClear(COLOR | DEPTH)` borra, en cada cuadro, el color **y la profundidad**.
  Si solo se borrara el color, las profundidades del cuadro anterior seguirían ahí
  y las caras viejas le ganarían al cubo nuevo.

### Qué deberías ver

Con el test bien puesto: **el cubo con 3 caras de 3 colores distintos, que no
parpadean ni aparecen/desaparecen**. Sin el test (o con las profundidades sin
borrar), verías porciones de caras traseras montadas sobre las delanteras.

Resumen: el depth test decide qué fragmento gana cuando varios compiten por el
mismo píxel — el más cercano a la cámara — usando un buffer de profundidad que hay
que habilitar una vez y limpiar en cada cuadro.

---

## 4. La clínica del práctico: ¿por qué sucede esto?

La rotura de la clínica es cambiar:

```cpp
glDrawElements(GL_TRIANGLES, cube.count(), GL_UNSIGNED_INT, nullptr);
```

por:

```cpp
glDrawArrays(GL_TRIANGLES, 0, 36);    // ROTURA
```

y no tocar nada más. El resultado es un cubo "roto" y **sin ningún error de
OpenGL**. Veamos por qué.

### La diferencia entre los dos dibujos

- **`glDrawElements`**: la GPU **recorre el EBO** (la lista de 36 índices) y por
  cada índice busca `vertices[indices[k]]` en el VBO. La lectura está *mediada*
  por la tabla de índices. Los índices se escribieron **agrupados por cara** (cada
  triángulo toma 3 índices de la misma cara), por eso los triángulos salen planos
  y bien formados.

- **`glDrawArrays`**: **ignora el EBO por completo** y lee el VBO **en orden**
  directo, de a 3: triángulo 1 = vértices `0,1,2`; triángulo 2 = `3,4,5`; …
  triángulo 12 = `33,34,35`.

### Qué pasa con los tríos (problema nº 1)

Los 24 vértices del buffer están agrupados por cara, 4 seguidos:

```
cara 0: 0  1  2  3    cara 1: 4  5  6  7    cara 2: 8  9  10  11
cara 3: 12 13 14 15   cara 4: 16 17 18 19   cara 5: 20 21 22 23
```

Los 12 tríos que arma `glDrawArrays` son:

| triángulo | vértices | resultado |
|---|---|---|
| 1 | 0, 1, 2 | 3 de los 4 de la cara 0 → **medio cuadrado (con agujero)** |
| 2 | 3, 4, 5 | mix face 0 + face 1 → cruza de un color a otro |
| 3 | 6, 7, 8 | cruza cara 1 + cara 2 |
| 4 | 9, 10, 11 | cruza |
| 5 | 12, 13, 14 | cruza |
| 6 | 15, 16, 17 | cruza |
| 7 | 18, 19, 20 | cruza |
| 8 | 21, 22, 23 | 3 de los 4 de la cara 5 → medio cuadrado |
| 9–12 | 24..35 | **fuera del buffer** (problema nº 2) |

Cuando un triángulo toma vértices de dos caras de distinto color, el color se
**interpola** dentro del triángulo (gradiente) y la forma queda deformada.

### Qué pasa con las lecturas fuera del buffer (problema nº 2)

`glDrawArrays(..., 36)` pide **36 atributos**, pero el buffer solo tiene **24
vértices**. La GPU lee igual los vértices `24..35`, que no existen: comportamiento
**indefinido** según el spec. En la práctica devuelve ceros o basura, así que los
últimos 4 triángulos **colapsan hacia el origen** o aparecen descolocados.

### Y nada informa del error

Ninguna de estas dos cosas produce un error de OpenGL: pedir 36 vértices es legal
aunque el buffer tenga menos; el driver **no valida el contenido**. Por eso la
guía aclara que hay que *visualizarlo o predecirlo*: es una falla silenciosa, igual
que las de la clínica del triángulo (atributo apagado, punto y coma, vértice fuera
de NDC). El único "feedback" es lo que se ve (o no se ve) en la pantalla.

---

## 5. ¿Por qué "lee el buffer" en la clínica?

Aclaración de un malentendido natural: **todo dibujo siempre lee el buffer de
vértices**. La diferencia entre los dos llamados no es "leer" o "no leer", sino
*qué* vértices lee y *en qué orden*.

```cpp
// Los atributos de posición y color viven en el VBO; sin leerlos no hay triángulos.
// Lo que cambia es el CAMINO de lectura:
glDrawElements(...)  ->  lee el EBO (tabla) y con cada índice busca en el VBO.
                         La lectura del VBO está MEDIADA por el índice:
                         0 -> 1 -> 2 -> 0 -> 2 -> 3 -> 4 -> ... (saltos elegidos).
glDrawArrays(...)    ->  NO hay tabla. Lee el VBO DIRECTO y en orden:
                         vertices[0], vertices[1], ..., vertices[35] (secuencial).
```

Entonces lo de "lee el buffer" en la clínica es porque el dibujo arranca del mismo
VBO (no puede no leerlo); lo que rompe es que:

1. **sin los índices**, el orden directo corta las caras (tríos que mezclan
   colores, ver punto 4), y
2. el contador dice 36 y el buffer tiene 24 → las lecturas `24..35` **se salen**
   del buffer (ceros o basura).

En la práctica: el mismo VBO, dos "rutas de lectura" — una **indexada** (la elige
el EBO) y otra **secuencial** (del 0 al 35). Por eso cambiar una sola línea desarma
el cubo sin tocar datos ni shaders.

Resumen: leer el buffer es inherente al dibujo; lo que cambia es el *camino*
(indexado vs. secuencial), y ese cambio es el que genera la falla.

---

## 6. Los parámetros de `cube(scale_x, scale_y, scale_z)`

La firma del generador:

```cpp
MeshData cube(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);
```

Devuelve un `MeshData` (solo datos, sin OpenGL). Los tres parámetros son **la
escala de cada dirección**:

| parámetro | eje | dimensión |
|---|---|---|
| `scale_x` | X | ancho |
| `scale_y` | Y | alto |
| `scale_z` | Z | profundidad |

Los tres tienen **valor por defecto 1.0**, así que:

```cpp
cube()                     // cubo de lado 1 (de -0.5 a +0.5 en cada eje)
cube(2.0f, 2.0f, 2.0f)     // cubo de lado 2 (de -1 a +1)
cube(4.0f, 4.0f, 0.2f)     // placa delgada: 4 de ancho, 4 de alto, 0.2 de pro.
```

Cómo se usan adentro: forman el arreglo `kScale = {scale_x, scale_y, scale_z}`, y
cada cara calcula sus coordenadas como `sign · 0.5 · kScale[eje]`. Es decir, el
parámetro actúa sobre el **medio lado**: el cubo se estira del centro hacia
afuera, por eso quedó **centrado en el origen** y no apoyado en `y = 0` (decisión
documentada en el header: es más cómodo transformar después un objeto centrado).

El enunciado remarca que con estos parámetros se obtienen "otras figuras de caras
rectas paralelas" (placas, paralelepípedos) sin escribir nada nuevo — y que cuando
aparezcan `cylinder(radio, altura, gajos)` y `cone(...)` se agregan al módulo con
la misma forma (devolver `MeshData` con parámetros de tamaño), sin reescribirlo.

Resumen: `scale_x/y/z` estiran el cubo en cada dirección a partir del centro; con
los defaults obtenés el cubo de lado 1 pedido por el práctico.

---

## 7. ¿Por qué se suman `base + 0 … base + 3` en los índices?

Es la forma de **convertir las 4 esquinas locales de una cara en índices
globales** dentro de la lista de 24 vértices.

```cpp
mesh.indices.push_back(base + 0);
mesh.indices.push_back(base + 1);
mesh.indices.push_back(base + 2);
mesh.indices.push_back(base + 0);
mesh.indices.push_back(base + 2);
mesh.indices.push_back(base + 3);
```

Los vértices se guardan **agrupados por cara**: la cara 0 ocupa las posiciones
`0..3`, la cara 1 `4..7`, …, la cara 5 `20..23`. Para la cara `f`, `base = f * 4`
es **dónde arrancan sus 4 vértices**:

| cara | base | sus 4 vértices en la lista |
|---|---|---|
| 0 | 0 | 0, 1, 2, 3 |
| 1 | 4 | 4, 5, 6, 7 |
| 5 | 20 | 20, 21, 22, 23 |

Entonces `base+0, base+1, base+2, base+3` son **las 4 esquinas de esa cara**, en el
orden antihorario en que se generaron.

Un cuadrado se pinta con **dos triángulos que comparten la diagonal** (la que va de
la esquina 0 a la 2), para no desperdiciar un vértice en el medio:

```
1 ———— 2        triángulo 1: (0, 1, 2)  = base+0, base+1, base+2
|  ╱  |        triángulo 2: (0, 2, 3)  = base+0, base+2, base+3
|╱    |
0 ———— 3
```

Por eso `base+0` (la esquina 0 de la cara) aparece **dos veces**: es la esquina que
ambas mitades comparten. Es normal en mallas indexadas repetir ese índice; el
ahorro real está en que cada vértice **único** se escribe una sola vez en el VBO
aunque varios triángulos lo usen.

El mismo patrón se repite para las 6 caras con distinta `base`:

```
cara 0: 0, 1, 2, 0, 2, 3
cara 1: 4, 5, 6, 4, 6, 7
...
cara 5: 20, 21, 22, 20, 22, 23
```

6 caras × 6 índices = **36 índices** totales, que es exactamente lo que pide la
consola del programa ("se esperan 24 y 36").

Resumen: `base + 0..3` suma el desplazamiento de la cara para referirse a las 4
esquinas correctas en la lista global; los dos triángulos comparten la diagonal
0→2, y por eso `base+0` se repite.