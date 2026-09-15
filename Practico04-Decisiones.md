# Práctico 04 — Armado de la aeronave: decisiones

Apunte de diseño del **Práctico 04**: cómo se compone la aeronave con las
primitivas del Práctico 03, cómo se organiza el módulo `Aircraft` y cómo se
verifica la pose con una rotación conjunta. Sigue el formato de los apuntes
anteriores (`Practico03-Decisiones.md`, etc.).

---

## Objetivo

Completar el armado de la aeronave planteado en el taller de la clase:
**componer un único modelo** usando varias primitivas del Práctico 03 y
**verificar la composición** sometiéndola a una rotación conjunta (la pose).
La diferencia con el Práctico 03: allá las piezas eran objetos independientes;
acá son parte de UN objeto y sus transformaciones ya no son independientes.

---

## Parte 1 — El módulo Aircraft

### Qué hay y por qué

| Pieza | Contenido | Notas |
|---|---|---|
| `RenderItem.h` | `{const Mesh* mesh; glm::mat4 model; glm::vec3 color;}` | Lo único que el resto del programa necesita para dibujar una pieza ya transformada. |
| `Aircraft.h/.cpp` | `init()` / `update(pos, angulos)` / `collect(items)` | Dueño de las piezas (mallas), de las matrices locales y de la pose. |

**Decisión — `vec3` y no el `vec4` del molde de la guía:** el fragment shader
no usa el canal alfa (`uColor` es `vec3`); no tiene sentido arrastrar un cuarto
componente que nadie consume.

**La interfaz es la del molde** (`init/update/collect`): el resto del programa
no sabe cuántas piezas tiene el avión ni cómo están armadas. El día que el
modelo salga de un archivo `.obj`, **solo cambia `init()`** (que deja de llamar
a los generadores y carga la malla externa); `update()`, `collect()` y el
`main` quedan igual.

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| ¿Con qué pose se dibuja si `collect()` se llama antes de `update()`? | `pose_` se construye con `{1.0f}`: identidad → el avión aparece centrado en el origen del mundo, sin rotar (el punto de referencia queda en `(0,0,0)`). No es un error, pero conviene que el `main` llame `update()` antes de dibujar. |
| ¿Y antes de `init()`? | `piezas_` está vacío → `collect()` agrega cero items → no se dibuja nada. |
| ¿Por qué separar `init()` (una vez) de `update()` (cada cuadro)? | `init()` genera geometría (bucles) y sube mallas a la GPU (`glBufferData`): costoso y repetirlo no cambia nada. `update()` solo multiplica matrices: barato y necesario por cuadro (la pose cambia). |
| ¿Qué cambia el día del `.obj`? | Solo `init()`. Es la razón de existir del módulo: encapsular la definición del modelo. |

---

## Parte 2 — Piezas y matrices locales (`init()`)

### Sistema de referencia del modelo (decisión 1)

- **Origen en la nariz** (recomendación de la clase: las traslaciones quedan
  positivas hacia atrás).
- **X+ hacia la cola** (atrás), **Y+ hacia el ala derecha**, **Z+ hacia
  arriba** — mano derecha, convención aeronáutica.
- Medidas relativas al largo del fuselaje **L = 1** (la guía aúlica pide no
  usar datos reales).

### Despiece (S-211, 7 piezas, 4 mallas)

| Pieza | Primitiva | Medidas | Matriz local (escrita; se aplica primero la de más a la derecha) |
|---|---|---|---|
| nariz | `cone` | r=0.09, h=0.20 | `T(0.10,0,0) · Rz(+90°)` |
| fuselaje | `cylinder` | r=0.09, l=0.50 | `T(0.45,0,0) · Rz(−90°)` |
| cola | `cone` | r=0.09, h=0.30 | `T(0.85,0,0) · Rz(−90°)` |
| ala izquierda | `cube` (placa) | S(0.16, 0.39, 0.03) | `T(0.42,+0.285,−0.005) · S(...)` |
| ala derecha | `cube` (misma malla) | S(0.16, 0.39, 0.03) | `T(0.42,−0.285,−0.005) · S(...)` |
| estab. horizontal | `cube` (misma malla) | S(0.09, 0.30, 0.02) | `T(0.90,0,0) · S(...)` |
| deriva | `cube` (misma malla) | S(0.10, 0.02, 0.12) | `T(0.89,0,0.07) · S(...)` |

Detalles del armado:

- Cilindro y conos nacen con el eje en **Y**: `Rz(−90°)` lo acuesta hacia +X
  (cola); `Rz(+90°)` lo apunta hacia −X (nariz). La conicidad sale de la
  relación radio/altura (`conicidad = 2·atan(r/h)`), coherente con el
  Práctico 03 (la altura del cono es `r/tan(α/2)`).
- Las placas nacen como cubos de lado 1 y se deforman con **escalado no
  uniforme** — exactamente la técnica que sugiere la guía.

### Contar (actividad aúlica, verificado por consola)

- **Mallas distintas: 4** (cono nariz, cilindro, cono cola, cubo).
- **Matrices de modelo por cuadro: 7** (una por pieza).
- **¿Por qué no coinciden?** Porque las cuatro placas (alas y empenajes)
  comparten la malla del cubo y difieren solo en su matriz local. La guía
  señala justamente que los dos números no tienen por qué coincidir.

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| ¿La normal sigue siendo perpendicular después del escalado no uniforme? | Para el **cubo** sí: sus caras son planos axiales y un escalado por eje no los inclina (la normal `(1,0,0)` de la cara `x = cte` sigue perpendicular a `x = cte` tras escalar). Para cilindro/cono/esfera **no**: la sección pasaría a ser elíptica. Por eso el fuselaje se arma con el radio (escalado uniforme) y las placas son cubos — la pieza para la que "no importa" es justamente la placa. |
| ¿Por qué no escalado negativo para el ala derecha? | `scale(-1,1,1)` invierte el orden de los vértices (winding): las caras quedarían mirando para adentro. La pieza simétrica se arma con la MISMA malla trasladada al lado opuesto (criterio "cómo se sabe que está bien": ninguna pieza usa escalado negativo). |

---

## Parte 3 — Pose y verificación (`update()`, `collect()`)

### La pose (decisión 2: dos niveles, sin árbol)

```
Mlocal(pieza)  : fija, se arma UNA vez en init()
Mpose          : T(pos) · Rx(rolido) · Ry(cabeceo) · Rz(guiñada) · T(−ref)
Mmundo(pieza)  = Mpose · Mlocal          ->  collect() compone el producto
```

- `T(−ref)` va a la **derecha** (se aplica primero): lleva el punto de
  referencia (centro de gravedad) al origen para que la rotación ocurra
  alrededor de él. Si se lo pusiera del otro lado, el avión "giraría en arco"
  alrededor del origen del modelo (la nariz) — la falla que describe la clase.
- El punto de referencia se fijó en **`(0.438, 0, 0)`** = **0.438·Lf** desde la
  nariz, sobre el eje (Lf = largo total del avión). Es la posición que indica
  la imagen de la guía para aplicar las transformaciones de pose.
- Los ángulos de `update()` vienen en **radianes** (criterio del proyecto
  desde el Práctico 03): `x` = cabeceo, `y` = guiñada, `z` = rolido. El orden
  de aplicación es rolido → cabeceo → guiñada (lo escrito más a la derecha se
  aplica primero).

### Verificación (cómo se sabe que está bien)

1. **Cabeceo oscilante desde el `main`**: `cabeceo = 20° · sin(t · 0.75)`, con
   posición y demás ángulos en cero. Todas las piezas deben moverse JUNTAS,
   girando alrededor del centro de gravedad.
2. **Por cálculo** (programa auxiliar `bbox_aircraft.cpp`, en `/tmp/opencode`,
   no forma parte del proyecto): se aplicó a los vértices reales de cada
   pieza la misma cadena del shader (`ajuste · vista · pose · local`) y se
   comprobó, para cabeceo −20°/0°/+20°:
   - `pose · ref = (0, 0, 0)` exacto → la rotación es alrededor del punto de
     referencia y no de otro punto;
   - todas las piezas quedan dentro del NDC con margen;
   - la nariz sube/baja mientras la cola hace lo opuesto (misma rotación,
     fases opuestas por estar a distinto lado del punto de referencia).
3. **Captura de pantalla real** (dos instantes del ciclo, separados ~3.4 s):
   se midieron los centros de cada pieza. Con la nariz arriba la cola y los
   empenajes están abajo; con la nariz abajo, la cola y los empenajes suben
   ~50 px. Todas las piezas conservan sus posiciones relativas: el conjunto
   rota como un solo cuerpo.

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| ¿De qué lado va la `T(−ref)`? | A la derecha de la composición (se aplica primero). Del otro lado, la traslación quedaría "rotada" y el avión orbitaría alrededor del origen del mundo. |
| ¿Un alerón móvil alcanza con la local fija? | No: haría falta un tercer nivel (`Mpose · Mala · Malerón`) y recalcular ese producto por cuadro. Ahí la jerarquía deja de ser "lista de dos niveles". |
| ¿Hace falta un árbol de transformaciones? | Para ESTE modelo no: ninguna pieza se mueve respecto de otra, alcanza una lista de piezas + dos niveles (`Mpose · Mlocal`). Un árbol (o `Mpose · Mlocal · Mpieza-hija`) se justifica cuando aparezcan superficies móviles. |

---

## Arquitectura: qué cambió en el `main`

- La escena del Práctico 03 (piezas sueltas) se reemplaza por **un solo
  modelo**: `Aircraft`. El loop por cuadro hace `update()` (con la actitud),
  `collect()` y recorre la lista dibujando — exactamente el esqueleto de la
  guía.
- La rotación de vista de los prácticos anteriores ahora vive en el `main`
  como una matriz `vista` separada: `uModel = vista · item.model`. Las
  matrices de `Aircraft` describen solo el MODELO; la vista describe cómo se
  lo mira. La matriz de vista real llega en la Unidad VII (hoy sigue siendo
  parte de la caja negra junto con `uAjuste`).
- `vista = Ry(25°) · Rx(−65°)`: elegida para que el eje del fuselaje quede
  horizontal en pantalla, el "arriba" casi vertical y las alas diagonales
  hacia adentro (vista 3/4). Con la cámara frontal del template el modelo
  quedaría chato, como en el Práctico 03.

> **Lección del orden en glm (un bug real corregido).** `glm::translate`,
> `glm::rotate` y `glm::scale` **post-multiplican** (`m·T`, `m·R`, `m·S`), así
> que la llamada que se *escribe más afuera* es la que se *aplica más tarde*.
> Escribir `glm::translate(glm::rotate(I, a, eje), t)` da `R·T` y la pieza se
> **traslada antes de rotar** (quedaba desplazada, "piezas separadas"). Para
> rotar/escalar primero y trasladar después hay que anidar al revés:
> `glm::rotate(glm::translate(I, t), a, eje)` = `T·R`. Las matrices locales de
> nariz/fuselaje/cola usan esta última forma; las placas usan
> `glm::scale(glm::translate(I, t), s)` = `T·S` (correcta desde el principio).
- La consola responde el "contar" de la actividad aúlica: **7 piezas con 4
  mallas distintas** (y 7 matrices de modelo por cuadro).

## Build y estado

- Compila sin warnings (`-Wall -Wextra`).
- `make` recolecta `Aircraft.cpp` solo (está en `src/core`); no hubo que tocar
  el `Makefile`.
- Los tests de los prácticos anteriores siguen pasando (`test_primitives`,
  `test_rm`).
