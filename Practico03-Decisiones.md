# Práctico 03 — Primitivas paramétricas y matriz de modelo: decisiones

Apunte de diseño del **Práctico 03**: qué cambió respecto de los prácticos
anteriores, qué decisiones se tomaron y por qué, y cómo se verifica que está
bien. Sigue el formato de documentación de `Practico02-Modulos.md` y
`ResourceManager-Practico-01.md`.

---

## Objetivo

Generar las mallas de un **cilindro** y un **cono** a partir de pocos
parámetros (en vez de escribir vértices a mano) y armar una **escena con varias
piezas**, cada una en distinto lugar, tamaño y color, usando una **matriz de
modelo** y un **color** que viajan como *uniform*. Se agrega la biblioteca
**glm** (solo encabezados; en este sistema está en `/usr/include/glm`).

---

## Parte 1 — La tupla del vértice cambia

| | Práctico 02 | Práctico 03 |
|---|---|---|
| Tupla | `px,py,pz, r,g,b` | `position, normal, tex_coords` |
| Tipos | 6 `float` sueltos | `glm::vec3`, `glm::vec3`, `glm::vec2` |
| Color | atributo por vértice | **sale** de la tupla |
| Normal | — | **entra** (perpendicular a la superficie, unitaria) |
| UV | — | **entran** (coordenadas de textura, 0..1) |

**Por qué.** El color no es un dato del *vértice* sino del *objeto* (una pieza
entera es de un color): repetirlo en cientos de vértices es desperdicio. La
normal y las UV sí son datos por vértice (varían de vértice en vértice) y se
calculan/guardan aunque **todavía no se usen** — la normal la consumen la
iluminación (Unidad IX) y el descarte de caras; las UV las consumen las
texturas.

**Qué arrastra el cambio (la "migración" que responde la pregunta del Práctico
02).** En el Práctico 02 los desplazamientos de los atributos se calculaban con
`offsetof(Vertex, ...)` justamente para este momento. Al cambiar la tupla solo
hubo que tocar:

1. `MeshData.h` — la nueva tupla (con `#include <glm/glm.hpp>`).
2. `Mesh.cpp` — `load()` pasa de 2 a **3 atributos** (ubicaciones 0, 1, 2), los
   tres sobre el mismo buffer entrelazado; los `offsetof` se recalculan solos.
3. `solid.vs` — declara los tres `layout (location = ...)`.

Eso coincide con lo previsto: **no se reescribió el módulo `Mesh`**, se cambió
el contenido de la tupla y lo que se deriva de ella.

**Nota sobre un atributo no usado (cuestión a pensar).** Si un atributo/uniform
se declara en el shader y no se usa, el compilador de GLSL puede eliminarlo, y
entonces `glGetUniformLocation`/`glGetAttribLocation` devuelven `-1`. No es un
error: `-1` se ignora en silencio. Por eso el módulo `Shader` avisa cuando lee
una ubicación `-1` (ver Parte 3). En nuestro caso `aTexCoords` se declara pero
aún no se usa; está pinchado por `layout (location = 2)` explícito en el shader
y en el VAO, así que la conexión es estable de todos modos.

---

## Parte 2 — primitives: generación paramétrica

`cylinder()` y `cone()` no escriben coordenadas a mano: todo se calcula en
bucles. Cambiar gajos = cambiar un parámetro, y la malla se regenera sola.

### Cilindro
- Firma: `cylinder(radio, largo, gajos, anillos = 1)`.
- **Eje de revolución = Y** (decisión 3 del header): un anillo es
  `(r·cosθ, y, r·senθ)`, la normal lateral es radial `(cosθ, 0, senθ)` (la
  posición con la componente axial anulada, normalizada).
- **N+1 vértices por anillo** (no N): en la costura posición y normal
  coinciden, pero la UV vale 0 y 1 a la vez → un atributo difiere → se duplica.
- **`anillos` = subdivisiones axiales** (default 1 → dos anillos de vértices,
  uno en cada extremo). El lateral de N gajos y 2 anillos da
  **2(N+1) = 2·9 = 18** vértices para N=8 (verificado en el test).

### Cono
- Firma: `cone(radio, conicidad, gajos, anillos = 1)`.
- **`conicidad` = ángulo de apertura COMPLETO α, en RADIANES** (decisión 4).
  El semiángulo es α/2 y fija la altura: `largo = radio / tan(α/2)`.
- **Normal lateral inclinada** por el semiángulo (no radial):
  `(cos(α/2)·cosθ, sen(α/2), cos(α/2)·senθ)`.
- **Ápice = 1 solo vértice** con normal axial `(0,1,0)` (criterio de las
  filminas: no comparte con el lateral porque la normal difiere). Queda
  documentado: estrictamente la normal no está definida en el ápice; se elige
  la axial porque da mejor sombreado después.
- Los anillos intermedios van achicando el radio hacia el ápice; la última
  banda es un abanico de triángulos con el vértice del ápice.

### Orientación de los índices (cuestión a pensar)
El orden "natural" al recorrer el cuadrilátero `{b0, b1, t1}` deja **todas** las
normales al revés (como advierte la guía). Se usó:

```cpp
{b0, t1, b1}   y   {b0, t0, t1}        // cilindro y bandas del cono
{b0, apex, b1}                         // abanico del ápice del cono
```

Esto se verificó **por cálculo** (ver abajo): el producto vectorial
`(B−A) × (C−A)` apunta al mismo lado que la normal de los tres vértices en todos
los triángulos.

### Sin tapas (decisión tomada)
Se modela **solo la superficie lateral** (decisión 7). Las tapas llevarían
normal axial y nunca comparten vértices con el lateral; quedan para más
adelante.

### Esfera (opcional de la guía, agregada)
`sphere(radio, gajos, anillos = 8)` sigue el mismo método con un bucle más:
anillos de **latitud** entre los polos. Los polos son un único vértice con
normal axial (criterio del ápice del cono). La banda sur lleva el orden
`{S, v_j, v_j+1}`, la norte `{v_j, N, v_j+1}` (distinto al sur, por la
dirección de la normal) y las bandas del medio usan el patrón del cilindro.
Verificado con el test de winding (`esfera 8 gajos/4 anillos` = 29 vértices,
`12/8` = 93).

---

## Parte 3 — Shader: los uniform

Un *uniform* es una variable del programa que se fija antes de ejecutar el
pipeline y mantiene el mismo valor para todos los vértices/fragmentos de esa
llamada. Por él viajan ahora el **color** y la **matriz de modelo**.

Decisiones (documentadas en `Shader.h`):

1. **Un método por tipo** (`mat4`, `vec3`, `float`) — no es lista cerrada; cada
   tipo nuevo agrega una sobrecarga.
2. **DSA: `glProgramUniform*`**, que reciben el programa como argumento y **no
   exigen haberlo activado** con `use()` (a diferencia de `glUniform*`, que
   actúa sobre el programa activo: llamarlo sin activarlo cambia el uniform de
   otro programa y es difícil de encontrar).
3. **Dos sabores**: por *nombre* (legible, busca el string cada vez) y por
   *ubicación cacheada* (`loc()` una vez + `set_uniform(int, ...)` en el loop),
   que evita buscar el string miles de veces por segundo.
4. **Ubicación −1 avisada**: si el uniform no existe o lo eliminó el
   compilador, `loc()` avisa por consola; pasar −1 no produce error (se ignora).

En el `main` se usa la **ubicación cacheada**: se consulta `loc("uModel")`,
`loc("uColor")` y `loc("uAjuste")` una sola vez y se setea por entero dentro
del loop.

---

## Parte 4 — La escena

- **4 piezas**: cubo (rojo, chico, a la izquierda), cilindro (verde),
  esfera (amarilla; es el *opcional* de la guía sumado a la escena) y cono
  (azul, a la derecha). Mallas generadas **una sola vez**, fuera del loop;
  dentro del loop solo cambian los uniform y se dibuja.
- **Layout calibrado con un chequeador de bounding-box.** La primera
  composición dejaba el **cubo cortado**: su esquina rotada (el Scale del
  cubo se aplica ANTES de la inclinación, y la diagonal crece con las
  rotaciones) sobresalía del borde izquierdo del NDC (`x < -1`). Se escribió
  un pequeño programa que aplica a los vértices REALES de cada primitiva la
  misma cadena del shader (`uAjuste * T * inclinacion * S`) y reporta los
  extremos por pieza. Con eso se fijaron posiciones `x = -0.92 / -0.32 /
  0.26 / 0.82` y tamaños que dejan sobrantes claros al borde y un hueco
  real de ~0.06 entre piezas (las "cajas" proyectadas pueden solaparse
  aunque los sólidos no se toquen: se verificó por distancia entre
  centros).
- **Matriz de modelo** por pieza: `glm::translate` + una inclinación común
  (`rotate` en Y de 35° y en X de −25°, la rotación que en el Práctico 02
  venía dentro de la caja negra) + `glm::scale` sobre la identidad. Lleva la
  malla de su sistema local (centrado en el origen) al lugar de la escena.
  Sin esa rotación, la cámara mira de frente y cada pieza se ve "chata" (el
  cubo como un cuadrado, el cilindro como un rectángulo, el cono como un
  triángulo): la malla es 3D, pero ni el color plano ni el `uAjuste` (sin
  perspectiva todavía) transmiten profundidad.
- **`uAjuste` = caja negra** (Unidad VII): corrige el aspect de la ventana y
  niega Z:

  ```cpp
  glm::scale(glm::mat4(1.0f), glm::vec3(alto/ancho, 1.0f, -1.0f));
  ```

- **Depth test** mantenido como en el Práctico 02 (`glEnable` una vez +
  `glClear(COLOR | DEPTH)` por cuadro).
- En el vertex shader: `gl_Position = uAjuste * uModel * vec4(aPos, 1.0);` — se
  aplica **primero** la matriz más a la derecha (`uModel`), luego `uAjuste`.

**Por qué la multiplicación va en el shader y no en el CPU:** la GPU transforma
los vértices **en paralelo** (un núcleo por vértice); en el CPU serían
secuenciales. El CPU *arma* la matriz de cada objeto; la GPU *aplica* el
producto matriz×vector por vértice.

---

## Cómo se sabe que está bien

1. **Conteos en consola** (el programa los imprime):
   - cubo: 24 vértices, 36 índices.
   - cilindro (N=20, 2 anillos): **42 = 2(N+1)** vértices, 120 índices.
   - esfera (N=20, 12 anillos): **233** vértices (11 bandas interiores × 21 +
     2 polos), 1320 índices.
   - cono (N=20, 1 anillo): 22 vértices (21 del anillo + ápice), 60 índices.
2. **Verificación por cálculo del winding** (`tests/main_test_primitives.cpp`):
   para cada triángulo, `(B−A)×(C−A)` apunta al mismo lado que las normales de
   sus vértices. Sin esto, un orden invertido no se nota (el culling está
   apagado). Resultado: **todas las primitivas pasan**, incluidos cilindro con
   1 y 2 anillos y cono con 1 y 3 anillos.

   ```bash
   g++ -std=c++17 -Wall -Wextra -I./src src/core/Primitives.cpp \
       tests/main_test_primitives.cpp -o build/test_primitives && ./build/test_primitives
   ```

3. **Cambiar gajos y volver a correr**: la malla se regenera sola, sin tocar
   una coordenada (es el punto de la generación paramétrica).
4. **Modo de depuración de normales** (en `solid.fs`, comentado): reemplazar
   `FragColor = vec4(uColor, 1.0)` por
   `FragColor = vec4(vNormal * 0.5 + 0.5, 1.0)`. El lateral del cilindro debe
   mostrar un **degradé suave** alrededor del eje (normal que gira) y el cubo
   **seis caras de color plano** (normal constante por cara).
   *Probado con captura de pantalla y análisis de píxeles*: la línea que cruza
   una cara del cubo da **6 colores únicos** (cara plana) contra **136** del
   lateral del cilindro (degradé: un color casi por píxel). Detalle observado:
   al activar el modo debug, la consola avisa
   `uColor ... ubicacion -1` porque el compilador GLSL elimina el uniform que
   no se usa — exactamente la falla prevista en el módulo `Shader`.
   *Verificación visual (con la escena de 4 piezas del final)*: el cubo
   muestra tres caras de color plano (normales `+z/-x/-y` constantes); el
   cilindro un degradé suave en el lateral con el canal verde fijo en 128
   (las normales radiares no tienen componente Y) y un color distintivo en el
   canto inferior del agujero (base sin tapa); la esfera un degradé en toda la
   superficie (la normal apunta a todos lados); el cono un degradé en el
   lateral —con componente Y en la normal (≠128), como corresponde a su
   inclinación— y un color único en su base.
5. Compila **sin warnings** (`-Wall -Wextra`), y no hay `glDelete*` en
   `main.cpp` (todo en los destructores de `Mesh`/`Shader`).

---

## Respuestas a las "cuestiones a pensar" del enunciado

| Pregunta | Decisión |
|---|---|
| ¿Se escribieron a mano los offsets o se calculan? | Se calculan con `offsetof` desde el Práctico 02; la migración a 3 atributos tocó solo `MeshData.h` + `Mesh.cpp` + shader |
| ¿Cuántos lugares se tocaron al agregar un atributo? | Tres (tupla, VAO, shader); coincide con lo previsto |
| ¿Qué devuelve una consulta por un atributo eliminado? | `-1`, sin error; `Shader::loc()` lo avisa |
| ¿Hace falta auxiliar conicidad ↔ altura? | No: la altura sale de `radio / tan(α/2)` internamente |
| ¿Grados o radianes? | **Radianes**, documentado en el header |
| ¿El ápice es un vértice o N? | **Uno solo**, con normal axial |
| ¿Cuántos vértices sin las UV en el lateral? | El lateral de N gajos bajaría de N+1 a N por anillo (no habría costura que duplicar) |
| ¿Por qué el objeto movido queda en cero? | (del Práctico 02, sigue vigente) dos dueños liberarían dos veces |
| ¿Cuándo consultar la ubicación del uniform? | Una vez, antes del loop; el valor guardado deja de valer si se recompila el programa |
