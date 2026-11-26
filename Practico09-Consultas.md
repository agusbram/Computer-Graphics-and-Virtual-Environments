# Práctico 09 — Consultas (preguntas y respuestas)

Preguntas frecuentes del **Práctico 09 (iluminación y niebla)**, en el mismo formato que `Practico08-Consultas.md`. Las respuestas están adaptadas a **este** proyecto: escena Z-arriba, origen flotante (el avión siempre en el origen) y terreno con LOD (geometry clipmaps de 51 km).

> Este archivo va a crecer durante la implementación, parte por parte.

---

## Chuleta (resumen para refrescar)

| Concepto | En una línea |
|---|---|
| Código GLSL común | `iluminacion.glsl` se **pega delante** del código propio de cada fragment shader: `#version` + común + propio. |
| `glShaderSource` con varias cadenas | Recibe un arreglo de textos y los compila como uno solo, en orden. Es el "`#include`" de GLSL. |
| `#version` | Tiene que ser la **primera** línea del texto completo: por eso la agrega `Shader` y ningún archivo la tiene. |
| `#line 1 N` | "La línea que sigue es la 1 del archivo N": el error sale como `N:línea` del archivo correcto. |
| Uniform eliminado | El compilador borra los uniforms que no se usan: `glGetUniformLocation` da −1 y `glProgramUniform*` con −1 no hace nada. |
| `struct Iluminacion` | El **único lugar** de la CPU con el sol y la niebla; se envía a los tres programas en cada cuadro. |
| "Mundo" con origen flotante | Posiciones relativas al avión: el ojo que se envía es `ojo_rel`, en ese mismo sistema. |
| Luz direccional | Rayos paralelos: una sola dirección `L` (`uSolDir`) para toda la escena; apunta **hacia** el sol. |
| Material del suelo | `kd` = textura, `ks` = 0. Escrito **una vez** en `iluminar_suelo()` para la malla y el suelo continuado. |
| Normal del terreno en mundo | El modelo del terreno es solo una traslación: la normal local ya es la de mundo (sin matriz normal). |
| `normalize` en el fragment shader | La normal interpolada es más corta que 1; sin normalizar, el centro de cada triángulo queda más oscuro. |
| Normal del nivel grueso | Se mezcla con el **mismo α** que la altura: en el borde del anillo el fino tiene la forma y la luz del grueso. |
| Margen de la grilla de alturas | 2 nodos más allá del borde: diferencias centrales en todos los vértices y la normal gruesa en el borde. |
| Borde sin margen (`generar_malla_terreno`) | Diferencia de **un solo lado** × 2: abarca un paso y la `z` es de dos. El borde es `j == N`, no `j + 1 == N`. |
| `Material` en el `RenderItem` | El color del P04 pasa a ser `kd`; `ks` y `ns` dependen de la apariencia (mate, metal, plástico). La textura **multiplica** a `kd`. |
| Uniform de tipo struct | `uniform Material uMaterial;`. Cada campo es un uniform aparte: se busca con el nombre completo, `"uMaterial.kd"`. |
| Matriz normal | `G = (M⁻¹)ᵀ` de la parte 3×3 de `M`. Se calcula en la CPU, una vez por pieza, y viaja como `mat3`. |
| Cuándo falla usar `M` | Solo con escalado **no uniforme** y una normal con componentes en **dos ejes** que se escalan distinto (el lateral de un cono estirado). |
| Ápice del cono | Un vértice **por gajo**, con la normal lateral: un único vértice con normal axial hace que el cono se ilumine como una ojiva. |
| Blinn-Phong | `H = normalize(L + V)` y `(N·H)^(4·ns)`. Con el × 4 el reflejo queda de tamaño parecido al de Phong. |
| Niebla exponencial | `f = 1 − e^(−d/V)`: 63 % a `V`, 86 % a `2V`, 95 % a `3V`. Un solo parámetro, sin inicio ni corte. |
| Visibilidad elegida | `V = 10 km` desde la Parte 5 (6 km en la Parte 4). El tope es `L/2 ≥ 2,5·V` → `V ≤ ~10 km` con el clipmap de 51,2 km. En el borde: 92,3 %. |
| Niebla en el cielo | `1 − smoothstep(0, 0.10, d.z)`: total en el horizonte, nada desde 5,7°. Borra la línea del horizonte del P08. |
| Por qué no la exponencial en el cielo | El cielo no tiene distancia (`d = ∞`): la exponencial lo dejaría **entero** del color de la niebla. Se usa el ángulo de la vista. |
| `smoothstep(a, b, x)` | `t = clamp((x−a)/(b−a), 0, 1)` y `3t² − 2t³`: de 0 a 1 en forma de S, con pendiente 0 en las puntas (sin franjas). |
| Niebla y altitud | A la altura `h`, ningún punto del terreno tiene menos niebla que `1 − e^(−h/V)`: la distancia es en el espacio. |
| Corrección gamma | Cuentas en **lineal**: `a_lineal()` a cada color que entra, `a_srgb()` al resultado, al final (después de la niebla). `γ = 2,2`. |
| Ida y vuelta | Las imágenes están en sRGB, las cuentas necesitan lineal y el monitor espera sRGB (eleva a 2,2): `a_lineal` al entrar, `a_srgb` al salir. Falta la entrada → lavado; falta la salida → casi negro. |
| Qué se convierte | Lo que es un **color** elegido por cómo se ve: texturas, cubemap, degradé, `kd`, color de la niebla. **No:** sol, ambiente, `ks` (proporciones de luz) ni las normales como color (un dato). |
| El cielo con la corrección | `a_srgb(a_lineal(c)) = c`: se ve **igual**. Si sale lavado, falta `a_lineal` en el cubemap. |
| Niebla con la corrección | Se mezcla en lineal: con la misma `V` se ve más **espesa** (10 % de niebla corre el pasto un 18 % en pantalla). Por eso `V` pasó a 10 km. |
| Ambiente con la corrección | `0,1` y sol `0,9`: la sombra se ve al `0,1^(1/2,2)` = 35 % (con 0,3 sería 58 %). |
| Shader, no hardware | `pow` en el shader: se apaga con `C`. Las texturas siguen en `GL_RGBA8`; con `GL_SRGB8_ALPHA8` además, se convertiría **dos veces**. |

---

# Consultas sobre las decisiones de diseño (antes de implementar)

## 1. No entiendo la pregunta 2: ¿quién pone el `#version` y en qué shaders?

**El problema.** Hasta el Práctico 08 cada archivo de shader empezaba con `#version 460 core`. Esa línea le dice al compilador qué versión de GLSL se usa y **tiene que ser la primera línea del texto que se compila**.

En el Práctico 09 el fragment shader del terreno ya no es un solo archivo. Se pegan dos, uno detrás del otro: `iluminacion.glsl` + `terreno.fs`. Si `terreno.fs` conservara su `#version`, el texto quedaría así:

```
...código de iluminacion.glsl...
#version 460 core        <- en el medio: error de compilación
...código de terreno.fs...
```

Por eso la línea se **saca de los archivos** y la escribe el código C++ (la clase `Shader`) al armar el texto, siempre primera.

**La pregunta era a qué shaders se la agrega el C++:**
- **(a) A todos:** vertex y fragment, y ningún archivo de `assets/shaders/` la tiene. Una sola regla.
- **(b) Solo a los que se arman con varios archivos:** los vertex shaders, que van solos, la conservan adentro. Quedan dos tipos de archivo y hay que acordarse cuál es cuál.

**Se eligió (a).** Esto es lo que compila la GPU para el terreno:

```
Fragment del terreno:
  cadena 0: #version 460 core   <- la agrega Shader
  cadena 1: iluminacion.glsl    (sin #version)
  cadena 2: terreno.fs          (sin #version)

Vertex del terreno:
  cadena 0: #version 460 core   <- la agrega Shader
  cadena 1: terreno.vs          (sin #version)
```

Con (a), si más adelante un vertex shader necesita código común (por ejemplo, la curvatura de la Tierra en `terreno.vs`), alcanza con agregarlo a su lista en `main`: no hay que tocar el archivo. En cada `.vs`/`.fs`, la vieja línea `#version` quedó reemplazada por un comentario que lo explica.

---

# Consultas durante la implementación

## 2. ¿Por qué usamos el origen flotante?

Por **precisión numérica**. Se decidió en el Práctico 07, para corregir el **z-fighting**: zonas donde dos superficies a casi la misma distancia de la cámara "pelean" en la profundidad y parpadean.

**El problema.**
- El FDM trabaja en metros y el avión vuela a ~5000 m de altura. Además se aleja del punto de partida mientras vuela, así que sus coordenadas crecen sin parar.
- Las posiciones se mandan a la GPU como `float`, que tiene unas **7 cifras significativas**. Cuanto más grande es el número, más grande es el escalón mínimo entre dos valores representables (el ULP).
- La cámara está a pocas unidades del avión, pero las dos posiciones son números grandes (~500 u). Para dibujar hay que **restarlas** (eso hace la matriz de vista), y al restar dos números grandes y parecidos se pierde casi toda la precisión.
- Ese error ensuciaba la **profundidad**, y aparecía z-fighting:
  - en las **uniones de las piezas del avión**, que cambiaban de color de un cuadro a otro (commit `6c8f574`);
  - en el terreno.

**La solución.** Dibujar todo **relativo a un punto cercano a la cámara**: el avión.
- En cada cuadro, el avión se pone en el **origen** (`flight.position = 0`).
- El terreno se dibuja trasladado por `esquina − posición del avión`.
- La cámara orbita alrededor del origen.

La imagen es exactamente la misma, porque las distancias entre las cosas no cambian. Lo que cambia es que todas las coordenadas que llegan a la GPU son **chicas**, y con números chicos el `float` alcanza para que la profundidad salga exacta. El avión puede volar 500 km y sus coordenadas en la GPU siguen siendo cercanas a 0.

**Por qué importa en el Práctico 09.** "Mundo" en este proyecto es ese sistema relativo al avión. Por eso a los shaders se les manda `ojo_rel` y no `ojo_mundo`: el ojo tiene que estar en el **mismo sistema** que `vPosMundo`.

Para el sol no cambia nada: es una dirección, y una dirección no depende de dónde esté el origen. Si se mezclaran los dos sistemas, el síntoma sería el que advierte la guía: la luz se movería con la cámara.

---

# Consultas de la Parte 2 (el terreno iluminado)

## 3. ¿Por qué con luz el terreno se ve más oscuro que en el Práctico 08?

Porque ahora el color es `kd` **multiplicado por la luz que recibe**, y un suelo plano no recibe toda la luz del sol:

```
color = (ambiente + sol · max(N·L, 0)) · kd
      = (0,3 + 0,7 · sin 29,8°) · kd          (suelo plano: N·L = seno de la elevación del sol)
      = 0,65 · kd
```

Solo una ladera que mira **de frente** al sol (N·L = 1) llega a `kd`. Se midió en pantalla: con luz / sin luz da 65 / 100.

Las cuentas están bien, pero se hacen sobre colores en **sRGB**, que no es una escala lineal de luz. Multiplicar un valor sRGB por 0,65 oscurece más que recibir el 65 % de la luz. Lo arregla la **Parte 5 (gamma)**: con la cuenta en lineal, el suelo plano queda en `0,65^(1/2,2)` ≈ 0,82 del color de la textura. Por eso no se subieron las intensidades ahora: se ajustan con la gamma activa (decisión 10).

## 4. Si el geomorphing ya mueve la altura, ¿para qué hace falta la normal del nivel grueso?

Porque la luz no depende de la altura sino de la **normal**, y el morph no la movía.

- El morph lleva la **forma** del nivel fino a la del grueso cerca del borde del anillo: en el borde (α = 1) los vértices del fino quedan exactamente sobre la superficie del grueso. Así no hay grietas (Práctico 07).
- Pero cada vértice seguía con **su** normal, calculada con los vecinos del nivel fino (a un paso `p`). El grueso, del otro lado del borde, usa normales calculadas con **sus** vecinos (a `2p`). Son parecidas pero no iguales: en el borde, la luz **salta**.
- Sin luz no se notaba, porque la normal no se usaba. Con luz se ve una línea a lo largo del borde de cada anillo. Se midió pintando la normal como color: sin la mezcla, el salto en el borde es 1,25 veces la variación normal del relieve; con la mezcla, 0,77 (no hay costura).

**El arreglo** es el mismo truco del morph, aplicado a la normal: cada vértice guarda también la normal que tendría en el nivel grueso (`normal_gruesa`) y el vertex shader las mezcla con el **mismo α**: `n' = (1 − α)·n + α·n_c`. En el borde, el fino tiene la forma **y** la normal del grueso.

## 5. ¿Por qué la grilla de alturas tiene un margen? ¿No alcanzaba con el × 2 que se había decidido?

El × 2 arregla **un** problema; el margen arregla los **dos**.

- **El problema del borde:** las normales salen de diferencias centrales, con el vecino de cada lado. En el borde de la grilla falta un vecino y se usaba una diferencia de un solo lado, que abarca un paso en lugar de dos: la pendiente quedaba a la mitad. El × 2 corrige ese largo, pero sigue siendo una diferencia de **un lado** (una aproximación peor que la central).
- **La normal del nivel grueso** (consulta 4) necesita las alturas a **dos pasos** de cada vértice, también en el borde: los nodos −2 y N+2, que están **fuera** de la grilla. Sin esas alturas no se puede calcular la normal gruesa en el borde, que es justo donde α = 1 y más importa.

Como las alturas salen del ruido, que se puede evaluar en cualquier punto del mundo, alcanza con calcular **2 nodos más** de cada lado: 133 × 133 alturas en lugar de 129 × 129 (un 6 % más, y solo cuando un nivel se mueve). Con ese margen, todas las normales son centrales, también en el borde, y el × 2 ya no hace falta en el clipmap.

El × 2 quedó en `generar_malla_terreno`, la función libre de la guía del P07, porque una malla suelta no tiene datos más allá de su borde. Lo prueba `tests/main_test_normales.cpp`: en un plano inclinado, la normal del borde tiene que ser igual a la del interior. Con la fórmula vieja el test falla.

## 6. La guía arma el `Material` del terreno en `terreno.fs`. ¿Por qué acá está en `iluminacion.glsl`?

Porque el suelo se dibuja con **dos programas**: la malla del terreno (`terreno.fs`) y el suelo continuado más allá de la malla (`fondo.fs`). La guía pide que el suelo continuado se ilumine "con la misma función y con el mismo material que la malla", porque si no se vería el borde de la ventana de terreno.

Si cada archivo armara su propio `Material`, habría **dos copias** del material, y nada impediría que algún día queden distintas. Es el mismo problema que tenía la niebla repetida en el P08, y que la Parte 1 resolvió con el código común. Por eso el material del suelo (`kd` = textura, `ks` = 0) se escribe **una vez**, en `iluminar_suelo(P, N, kd)` del código común, y los dos programas la llaman. La tecla `B` también está ahí adentro: cambia el modo de los dos programas a la vez.

Se verificó midiendo el color a los dos lados del borde del clipmap: con luz, (65; 89; 21) en el suelo continuado y (65; 89,1; 20,8) en la malla.

## 7. ¿Qué pasa en el borde con las diferencias centradas? ¿No se sale de la grilla para tomar un vecino de cada lado?

Sí: en el vértice del borde (`j = N`) el vecino `j + 1` **no es un vértice** de la malla. Hay dos funciones que calculan normales y cada una lo resuelve distinto. (La consulta 5 explica **por qué** se eligió el margen; esta explica **cómo** se calcula en cada caso.)

**1. El terreno que se dibuja (clipmap): se calculan nodos de afuera.** Está en `Terrain::recalcular_vertices` (`src/core/Terrain.cpp`). No hay que inventar el vecino que falta: la altura sale de una **función** del ruido, `alturas_(x, y)`, que da un valor en cualquier punto del mundo. Antes de armar los vértices se calcula una grilla de alturas **2 nodos más grande de cada lado**: `i` y `j` van de −2 a N+2.

```
  j:   -2   -1  | 0    1   ...  N-1   N |  N+1  N+2
        ·    ·  | ●    ●   ...   ●    ● |   ·    ·
        margen  |      vértices reales  |  margen
```

- Los nodos `·` **no son vértices**: no se dibujan, solo se leen para las normales.
- En el vértice `N`, la diferencia centrada usa `H(N−1)` y `H(N+1)`. `N+1` existe porque cae en el margen: **todos** los vértices usan la misma fórmula, también los del borde.
- El margen es de **2** (y no de 1) por la normal del nivel grueso, que usa vecinos a dos pasos (`normal_central(i, j, 2)`, porque el paso del grueso es 2p):
  - en el vértice par `N` lee `H(N+2)`;
  - en el impar `N−1`, la regla par/impar promedia las normales gruesas de `N−2` y de `N`, y la de `N` vuelve a leer `N+2`.

  Nunca se lee más allá de 2.

**2. `generar_malla_terreno` (la función de la guía): diferencia de un solo lado, × 2.** Está en `src/core/Terrain.h`. No tiene margen: en el borde usa el nodo de adentro y el propio.

```
interior (j):  dx = h(j−1) − h(j+1)        → abarca 2 pasos
borde j = 0:   dx = 2·(h(0)   − h(1))      → abarca 1 paso, × 2
borde j = N:   dx = 2·(h(N−1) − h(N))      → abarca 1 paso, × 2
```

- La `z` de la normal es `2·paso`, a la escala de dos pasos. Sin el × 2, la diferencia del borde (un paso) quedaba a la mitad y la normal salía **más vertical** de lo que es (decisión 9).
- Esta función también recibe la altura como función, así que podría usar margen. Se mantuvo como en la guía porque es **genérica**: la fuente de alturas podría ser una imagen o un archivo, sin datos fuera de la malla.
- El terreno con LOD no usa estas normales: las reemplaza la versión con margen. Igual se corrigió, porque tiene que dar la normal correcta en cualquier malla que genere. Lo prueba `tests/main_test_normales.cpp`.

**Un detalle que se corrigió en la misma función:** la condición del borde era `j + 1 == N` y pasó a ser `j == N`. Con la vieja:

- el vértice `N − 1` hacía una diferencia de un lado sin necesitarlo;
- el vértice `N` caía en el caso "interior" y leía `h(N + 1)`, una altura de **fuera** de la malla.

**Qué nodos lee cada caso en el vértice del borde `N`.** Una diferencia centrada **no usa el nodo propio**: usa uno de cada lado, a la misma distancia. La de un solo lado usa el propio y uno de adentro.

| Dónde | Normal | Tipo de diferencia | Nodos que lee en `N` | Nodos de afuera |
|---|---|---|---|---|
| Clipmap | Fina (`v.normal`, s = 1) | Centrada | `N−1` y `N+1` | 1 (del margen) |
| Clipmap | Gruesa (`v.normal_gruesa`, s = 2) | Centrada | `N−2` y `N+2` | 2 (del margen) |
| `generar_malla_terreno` | La única que calcula | De un solo lado, × 2 | `N−1` y `N` (el propio) | Ninguno |

```
normal fina en N (clipmap):        N-1   [N]   N+1          → uno adentro, uno afuera
normal gruesa en N (clipmap):    N-2     [N]     N+2        → dos adentro, dos afuera
generar_malla_terreno en N:        N-1   [N]                → el propio y uno adentro (× 2)
```

"Dos hacia afuera y dos hacia adentro" vale solo para la normal **gruesa**. El nodo `N+2` del margen lo lee solo ella: por eso el margen es de 2 y no de 1. El clipmap **nunca** usa la diferencia de un solo lado.

**En resumen:** en el clipmap, que es lo que se ve en pantalla, la diferencia centrada sí usa nodos de afuera, que se calculan a propósito (el margen). En la función genérica, el borde usa una diferencia de un solo lado multiplicada por 2.

---

# Consultas de la Parte 3 (la aeronave iluminada)

## 8. ¿Por qué la normal no se puede transformar con la misma matriz que los puntos?

Porque una normal no es un punto ni un vector "dibujado" sobre la superficie: es lo que queda **perpendicular** a ella. Los vectores que están sobre la superficie (las **tangentes**) sí se transforman con `M`, como los puntos. La normal tiene que seguir siendo perpendicular a esas tangentes **después** de transformarlas.

**La deducción, corta.** Antes de transformar, `n · t = 0` para toda tangente `t`. Después, la tangente es `M·t` y se busca una matriz `G` tal que `(G·n) · (M·t) = 0`. Escrito con matrices: `nᵀ · Gᵀ · M · t = 0`. Esto vale para toda `t` si `Gᵀ · M = I`, o sea `G = (M⁻¹)ᵀ`.

**Un ejemplo con números.** Una rampa a 45° en el plano x-z: la tangente es `t = (1, 0, 1)` y la normal `n = (−1, 0, 1)` (normalizadas, da igual). Se estira **al doble en x**: `M = S(2, 1, 1)`.
- La tangente pasa a ser `M·t = (2, 0, 1)`: la rampa se aplanó.
- Con `M`, la normal sería `M·n = (−2, 0, 1)`, y `(−2, 0, 1) · (2, 0, 1) = −3 ≠ 0`: **no** es perpendicular.
- Con `G = (M⁻¹)ᵀ = S(½, 1, 1)`, la normal es `(−½, 0, 1)`, y `(−½, 0, 1) · (2, 0, 1) = 0`: **sí** es perpendicular.

`M` inclina la normal hacia el eje que **estira**. `G` la inclina hacia el otro lado, que es lo que pasa en la realidad: si la rampa se aplana, su normal se endereza.

**Cuándo da lo mismo:**
- **Rotación pura:** `M⁻¹ = Mᵀ`, así que `G = M`.
- **Escalado uniforme** `S(k, k, k)`: `G = S(1/k, 1/k, 1/k)`, que apunta para el mismo lado que `M·n`. Solo cambia el largo, y `normalize` lo corrige.

Por eso la guía dice que el error no aparece en todas las piezas (ver la consulta 9).

**Por qué en la CPU.** `G` depende solo de `M`, que es la misma para todos los vértices de una pieza. Calcularla en el vertex shader sería repetir la misma inversa en cada vértice. En la CPU son 7 inversas 3×3 por cuadro.

## 9. Si el avión ya tenía placas escaladas, ¿por qué en el P08 no se veía ningún error en la luz?

Es la cuestión del Práctico 04 que la guía pide contestar. El error de usar `M` aparece solo cuando la normal tiene componentes sobre **dos ejes que se escalan distinto**. En el avión del P08 ninguna pieza cumplía eso:

| Pieza | Escalado | Normales | ¿Se nota el error? |
|---|---|---|---|
| Alas, estabilizador, deriva | Cubo con `S` no uniforme | Alineadas con los ejes del escalado (±x, ±y, ±z) | **No**: el escalado solo cambia su largo, y `normalize` lo corrige |
| Fuselaje | Ninguno (se generó con su radio y su largo) | Radiales | **No** |
| Nariz y cola (hasta el P08) | Ninguno (se generaban con su radio y su alto) | Inclinadas | **No**: sin escalado, `G = M` |
| Pose del avión | Rotación + traslación | — | **No**: para una rotación, `G = M` |

O sea, con el avión del P08 la tecla X no habría cambiado **un solo píxel**. Por eso, en la Parte 3 la nariz y la cola salen de un **cono unitario estirado** con `S(r, h, r)` (decisión 17). La normal del lateral del cono tiene una parte radial y otra axial, que se escalan distinto (0,09 y 0,20 en la nariz). Ahí `M` se equivoca por **41,6°**, y al apretar X cambian exactamente esos dos conos y nada más (se midió: ver la Parte 3 en `Practico09-Decisiones.md`).

## 10. ¿Por qué hay que repetir el vértice del ápice del cono? ¿No es el mismo punto?

Es el mismo **punto**, pero no el mismo **vértice**. Un vértice es la tupla completa (posición, normal, coordenadas de textura), y en la punta del cono cada gajo llega con **su** normal.

Con un solo vértice de normal axial (0, 1, 0), la GPU interpola en cada triángulo del abanico entre la normal lateral de la base y la axial de la punta. Con 1 anillo, el abanico es **todo** el lateral. En la nariz (semiángulo 24,2°), la normal lateral forma 65,8° con el eje y la axial, 0°. A media altura la interpolada queda a ~33° del eje: **desviada ~33°**. Con luz, el cono se ve redondeado, como una ojiva, y el brillo sale como una mancha redonda en lugar de una franja recta.

Repitiendo el ápice una vez por gajo, cada triángulo tiene tres normales laterales y la luz es la de un cono hasta la punta. Es el mismo motivo por el que el cubo tiene 24 vértices y no 8, y por el que el anillo tiene N + 1 vértices (la costura): **un atributo distinto en el mismo punto obliga a duplicar el vértice**. En la esfera no hace falta: en el polo la normal **está** definida y es la axial.

Cuesta 19 vértices más por cono (de 22 a 41). La textura se aplica igual que antes, porque los ápices conservan la coordenada (0,5; 1).

## 11. ¿Qué cambia con Blinn-Phong y por qué el exponente × 4?

Solo cambia la **especular**: cómo se mide qué tan cerca está el ojo del reflejo.
- **Phong:** compara el rayo reflejado `R` con la dirección al ojo `V`: `(R·V)^ns`.
- **Blinn-Phong:** compara la normal `N` con el vector medio `H = normalize(L + V)`, que está a mitad de camino entre la luz y el ojo: `(N·H)^ns`. Si el ojo está justo en el reflejo, `H` coincide con `N` y el brillo es máximo, igual que en Phong.

**El × 4.** Si el ojo se aleja del reflejo un ángulo θ, `H` se aleja de `N` más o menos **θ/2**. Con el mismo exponente, el brillo de Blinn-Phong cae más despacio y sale más **ancho**. Para que los dos reflejos tengan un tamaño parecido se usa un exponente unas 4 veces mayor. Es una regla práctica: con ángulos chicos, `cos(θ/2)^(4n) ≈ cos(θ)^n`. Se midió en el fuselaje: el pico es el mismo y el reflejo de Blinn-Phong ocupa ~2/3 de los píxeles del de Phong.

**Por qué existe.** Fue el modelo de la tubería fija de OpenGL: con luz direccional y el ojo lejano, `H` es casi constante en toda la escena y se calcula una vez. Además, a ángulos rasantes no se corta de golpe, como pasa con `R·V` cuando el ángulo pasa de 90°. La guía y el trazador de rayos del TP10 usan Phong, por eso es el que queda por omisión (decisión 5).

---

# Consultas de la Parte 4 (niebla exponencial)

## 12. ¿Por qué la exponencial y no la lineal que ya teníamos?

**Porque así se comporta el aire.** Imaginemos el aire en capas de igual grosor. Cada capa deja pasar la **misma fracción** de la luz que le llega desde el objeto (y agrega un poco del color de la bruma). Si una capa de 6 km deja pasar el 37 %, dos capas dejan pasar el 37 % del 37 % (14 %), tres el 5 %, y así. Una cantidad que se multiplica por la misma fracción en cada tramo **es** una exponencial: lo que llega del objeto es `e^(−d/V)`, y lo que es niebla, `f = 1 − e^(−d/V)`.

**Lo que se nota en pantalla:**

| | Lineal (P07/P08) | Exponencial (P09) |
|---|---|---|
| Parámetros | Dos: inicio (3 km) y fin (18 km) | Uno: la visibilidad `V` (6 km) |
| Cerca del avión | Nada de niebla hasta 3 km | Un poco desde el principio (2 km: 28 %) |
| Lejos | 100 % justo desde 18 km: un **corte** | Se acerca al 100 % sin llegar nunca: sin corte |
| Qué tan lejos "se ve" | Hasta el fin, de golpe | Se va perdiendo de a poco |

La lineal tiene un **quiebre** en el inicio (de golpe empieza a cambiar) y otro en el fin. El ojo nota un quiebre así como una franja. La exponencial cambia suave en todo el recorrido.

**El `V` es una distancia fácil de pensar:** a `V`, el 63 % es niebla; a `3V`, el 95 % (casi no se distingue nada). Con `V = 6 km`, lo que está a 18 km prácticamente desaparece, como en un día con algo de bruma. (La gráfica de las dos curvas está en `Practico09-Decisiones.md`, Parte 4.)

## 13. Si la niebla depende de la distancia, ¿cómo se le pone niebla al cielo, que no tiene distancia?

El cubemap es una **foto**: cada píxel es una **dirección**, no un punto a cierta distancia. No hay `d` para poner en `1 − e^(−d/V)`.

**¿Y si igual le aplicamos la exponencial?** El cielo está "en el infinito": con `d = ∞`, `e^(−∞/V) = 0` y `f = 1` en **todo** el cielo. Quedaría entero del color de la niebla, también mirando para arriba. Hace falta otro criterio, y el único dato que tiene el cielo es **hacia dónde se mira**.

**La niebla sigue siendo exponencial.** El `smoothstep` no la reemplaza: se usa **solo** para el cielo.

| Qué se dibuja | Shader | Factor de niebla | Depende de |
|---|---|---|---|
| Terreno (malla) | `terreno.fs` | `1 − e^(−d/V)` (`aplicar_niebla`) | la distancia `d` al ojo |
| Avión | `aeronave.fs` | `1 − e^(−d/V)` (`aplicar_niebla`) | la distancia `d` al ojo |
| Suelo continuado | `fondo.fs` | `1 − e^(−d/V)` (`aplicar_niebla`) | la distancia `d` al punto del plano |
| **Cielo** | `fondo.fs` | `1 − smoothstep(0, 0.10, d.z)` | el **ángulo** de elevación de la vista |

**Lo que importa es el horizonte.** El suelo (malla y suelo continuado) llega al horizonte con `d → ∞`, o sea con `f → 1`: exactamente el color de la niebla. Si el cielo, justo arriba, tiene otro color, se ve una **línea**: es la del P08 (`Practico08-Consultas.md`, pregunta 22). Para que no haya línea, el cielo tiene que valer el color de la niebla **en el horizonte** y volver a su color un poco más arriba.

**Por eso la mezcla depende de la elevación:**

```glsl
float f_cielo = 1.0 - smoothstep(0.0, 0.10, d.z);   // d unitaria: d.z = sen(elevación)
cielo = mix(cielo, uNieblaColor, f_cielo);
```

| Elevación | `d.z` | `smoothstep` | `f_cielo` | Se ve |
|---|---|---|---|---|
| 0° (horizonte) | 0 | 0 | **1** | El color de la niebla, igual que el suelo lejano |
| ~2,9° | 0,05 | 0,5 | 0,5 | Mitad cielo, mitad niebla |
| 5,7° o más | ≥ 0,10 | 1 | **0** | El cielo tal cual |

El suelo y el cielo se encuentran en el horizonte con el mismo color, y desde ahí cada uno sigue su camino:

- **hacia abajo**, el suelo va saliendo de la niebla según la exponencial de la **distancia**;
- **hacia arriba**, el cielo sale de la niebla en los primeros 5,7° según el `smoothstep` de la **elevación**.

**La idea física.** La niebla real es una **capa** pegada al suelo. Mirando en horizontal, el rayo recorre kilómetros dentro de ella y todo se ve brumoso. Mirando para arriba, sale de la capa enseguida y el cielo se ve limpio. El `smoothstep` imita eso de la forma más simple: niebla total en el horizonte y nada a unos pocos grados de elevación.

**Qué calcula `smoothstep(a, b, x)`.** Es una función de GLSL que hace dos pasos:

```glsl
float t = clamp((x - a) / (b - a), 0.0, 1.0);   // 1) lleva x al rango [0, 1]
return t * t * (3.0 - 2.0 * t);                  // 2) le da forma de S: 3t² − 2t³
```

1. **Normaliza.** Con `x ≤ a` da 0, con `x ≥ b` da 1 y en el medio sube en línea recta. Hasta acá sería una rampa lineal.
2. **Suaviza.** Pasa la rampa por el polinomio `3t² − 2t³`, que arranca despacio, acelera en el medio y frena al llegar.

| `t` | Rampa lineal | `smoothstep` |
|---|---|---|
| 0 | 0 | 0 |
| 0,25 | 0,25 | 0,156 |
| 0,5 | 0,5 | 0,5 |
| 0,75 | 0,75 | 0,844 |
| 1 | 1 | 1 |

En la tabla se ve la forma de S: cerca de las puntas `smoothstep` cambia **menos** que la rampa (0,156 contra 0,25) y en el medio, más. La derivada es `6t − 6t²`: vale **0 en `t = 0` y en `t = 1`**, así que la curva entra y sale "acostada", sin quiebre ni al empezar ni al terminar la transición. La rampa lineal, en cambio, pasa de pendiente 0 a pendiente 1 de golpe en las dos puntas.

En nuestra línea, `x = d.z` (el seno de la elevación), `a = 0` (el horizonte) y `b = 0.10` (unos 5,7°). El `1 −` da vuelta la curva, porque queremos **mucha** niebla abajo y **nada** arriba.

**Por qué `smoothstep` y no una recta.** Una recta (`clamp(x / 0.10, 0, 1)`) tendría quiebres de pendiente en 0° y en 5,7°, y el ojo vería ahí dos líneas suaves. El ojo es muy sensible a esos cambios bruscos de pendiente en un degradé. Es el mismo motivo que en la consulta 12: cualquier quiebre en el color se nota como una franja.

**Por qué 5,7°.** Es lo que propone la guía (0,10 en el seno). Más chico, la bruma del horizonte queda más fina pero puede dejar ver la línea en el cielo con nubes de colores distintos; más grande, la bruma tapa la parte baja de las nubes. Con la tecla `J` (modo 2, "cielo sin niebla") se ve la línea que arregla.

## 14. Al subir, el terreno se ve cada vez más lavado. ¿No es mucha niebla?

**Pasa porque la distancia de la niebla es en el espacio, no la horizontal.** A una altura `h` sobre el terreno, el punto más cercano del suelo, el de abajo, ya está a `d = h`. Entonces ningún punto del terreno tiene menos niebla que `1 − e^(−h/V)`. Con `V = 6 km`: 3 % a 200 m, 15 % a 1 km, 39 % a 3 km, 63 % a 6 km. Y mirando hacia el horizonte los puntos están todavía más lejos.

**Se midió** cuánto contraste le queda al terreno (mirando hacia abajo, ~35°): 95 % a 200 m, 77 % a 1,2 km, 50 % a 3,2 km. Es la tabla de `Practico09-Decisiones.md`, Parte 4. Es lo que pide la guía: "subir y anotar a qué altitud el terreno empieza a perderse".

**Por qué se deja así (decisión 12).** El avión vuela a ~150 m del terreno, donde queda el 95 %. Más de 1 km sobre el terreno solo se llega a propósito, con `RePág`. Además es lo que pasa de verdad: desde un avión, en un día con bruma, el suelo se ve más lavado cuanto más alto se vuela.

**Si se quisiera menos bruma en altura**, hay dos caminos, en este orden:
1. **Abrir `V`** hasta ~10 km (el tope que impone el borde del clipmap, `L/2 ≥ 2,5·V`). Es un número en `main.cpp` (`kVisibilidad`).
2. **`V` como función de la altitud**, lo que sugiere la guía: por ejemplo, `V = V₀ + k·h`. Así, al subir, la niebla se abre y el terreno se sigue viendo. El costo es que el borde del clipmap puede quedar a la vista cuando `V` crece (habría que limitarla a ~10 km de todos modos).

Se decide después de la Parte 5: con la gamma, la niebla se ve distinta y hay que volver a mirarla (lo advierte la guía).

> **Actualización (Parte 5):** se tomó el camino 1. Con la corrección gamma la niebla se veía todavía más espesa (consulta 16), y `V` pasó a **10 km** (decisión 19). Con la corrección activa queda el 86 % del contraste a 200 m, el 64 % a 1,2 km y el 42 % a 3,2 km. `V` según la altitud sigue sin hacer falta.

---

# Consultas de la Parte 5 (corrección gamma)

## 15. Si la imagen ya se veía bien, ¿para qué hace falta la corrección gamma?

**Se veía bien porque los colores se eligieron a ojo, pero las cuentas de la luz estaban mal.** Una textura o un color elegido mirando la pantalla está en **sRGB**: no guarda la luz, sino la luz "comprimida" con `v^(1/2,2)`, porque así la pantalla, que eleva a 2,2, la muestra bien. Mientras el shader copiaba el color a la pantalla (P08), no había problema: entraba sRGB y salía sRGB.

**El problema aparece al hacer cuentas.** La luz se suma y se multiplica: `kd · luz · cos θ`. Eso supone que el número es **proporcional** a la luz. Con valores sRGB no lo es:

| | Lo que se calcula | Lo que se escribe | Lo que muestra la pantalla |
|---|---|---|---|
| Sin corrección | `N·L = 0,5`: la mitad de la luz | 0,5 | `0,5^2,2` = **0,22** (menos de un cuarto) |
| Con corrección | la mitad de la luz, en lineal | `0,5^(1/2,2)` = 0,73 | `0,73^2,2` = **0,5** (la mitad) |

**Lo que se nota:** sin corrección, la luz se cae demasiado rápido hacia la sombra (el paso de la cara iluminada a la oscura es brusco) y la sombra queda muy oscura. Con corrección, la transición es **gradual**, como en la realidad. Lo que mira de frente al sol no cambia: con la luz sumando 1, llega a `kd` con o sin corrección.

**La regla:** entrar en lineal (`a_lineal`) con cada **color**, hacer las cuentas, salir en sRGB (`a_srgb`) al final. Ver `Practico09-Decisiones.md`, Parte 5, la tabla de qué se convierte.

### ¿Por qué se pasa a lineal y después de vuelta a sRGB?

Porque hay **tres partes** que trabajan en escalas distintas, y el color tiene que traducirse al **entrar** a las cuentas y al **salir**:

| Parte | En qué escala trabaja |
|---|---|
| **Las imágenes y los colores elegidos a ojo** (texturas, cubemap, `kd`) | **sRGB**: valores "comprimidos", no proporcionales a la luz |
| **Las cuentas de luz y niebla** (`kd · luz · cos θ`, `mix`) | Necesitan **lineal**: valores proporcionales a la luz |
| **El monitor** | Recibe un número y muestra luz = número^2,2: **espera sRGB** |

El camino de cada color:

```
textura (sRGB) --a_lineal--> cuentas en lineal --a_srgb--> monitor (eleva a 2,2) --> luz correcta
```

- **`a_lineal` al entrar** "descomprime" el color: las cuentas operan sobre luz real.
- **`a_srgb` al salir** lo vuelve a "comprimir", porque el monitor le va a aplicar `^2,2`. Esa compresión compensa justo la del monitor.

**Ejemplo: el pasto con la mitad de la luz del sol.** El pasto vale 0,30 en pantalla, que es `0,30^2,2` = **7 %** de luz real. Con la mitad de la luz, tendría que verse con **3,5 %**:

| Qué se hace | Se escribe | El monitor muestra | Resultado |
|---|---|---|---|
| **Las dos conversiones** (lo correcto) | 0,07 × 0,5 = 0,035 → `a_srgb` → 0,22 | 0,22^2,2 = **3,5 %** | Justo la mitad ✔ |
| Ninguna conversión (hasta la Parte 4) | 0,30 × 0,5 = 0,15 | 0,15^2,2 = **1,5 %** | Menos de un cuarto: la sombra cae de golpe |
| Solo `a_lineal` (falta la salida) | 0,035 | 0,035^2,2 = **0,06 %** | Casi negro |
| Solo `a_srgb` (falta la entrada) | 0,15 → `a_srgb` → 0,42 | 0,42^2,2 = **15 %** | El doble de luz: lavado |

Solo con las **dos** conversiones el resultado es el que dicen las cuentas.

**Por qué el cielo se ve igual con y sin corrección.** Las dos funciones son **inversas**: `a_lineal` eleva a 2,2 y `a_srgb` a 1/2,2. Si en el medio no hay ninguna cuenta, como en el cielo, que no se ilumina, se cancelan: `a_srgb(a_lineal(c)) = c`. Es la verificación de la guía: si el cielo sale lavado, falta la conversión de **entrada** (es el caso "solo `a_srgb`" de la tabla; ver la imagen 17 de `Practico09-Decisiones.md`).

**Por qué las imágenes vienen en sRGB y no en lineal.** Con 8 bits hay 256 niveles por canal. El ojo distingue mucho mejor las diferencias en los **oscuros** que en los claros, y sRGB dedica más niveles a los oscuros. Además, el monitor ya espera esa escala: mientras no haya cuentas (como en el P08), el color se copia tal cual y se ve bien.

## 16. ¿Por qué, con la misma visibilidad, la niebla se ve más espesa con la corrección?

**Porque la mezcla ahora se hace en lineal, y la niebla es un color claro.** `mix(C, niebla, f)` es la misma fórmula, pero los números son otros. Ejemplo con el pasto en 0,30 y la niebla en 0,82 (sRGB):

| Niebla `f` | Sin corrección (mezcla en sRGB) | Con corrección (mezcla en lineal) |
|---|---|---|
| 10 % | el pasto se corre un **10 %** hacia la niebla | se corre un **18 %** |
| 30 % | 30 % | 43 % |
| 50 % | 50 % | 63 % |

En lineal, el pasto vale `0,30^2,2` = 0,07 y la niebla, `0,82^2,2` = 0,65: la niebla tiene **9 veces** más luz. Un poco de niebla suma mucha luz a un color oscuro. Es lo que pasa de verdad: el aire entre el ojo y el objeto **agrega** la luz que dispersa (por eso las montañas lejanas se ven azuladas y claras, nunca más oscuras).

**Por eso V pasó de 6 a 10 km (decisión 19).** Con 6 km y la corrección, a 1,2 km de altura quedaba el 53 % del contraste del terreno (en la Parte 4, el 79 %). Con 10 km queda el 64 %. Más no se puede abrir: 10 km es el tope del clipmap (`L/2 ≥ 2,5·V`), y en el borde queda justo el 92 % que pide la guía.

## 17. ¿Por qué la corrección se hace en el shader y no con las texturas sRGB y el framebuffer sRGB?

Las dos formas son válidas (lo dice la guía). Se eligió el **shader** (decisión 11) porque:

1. **Se puede apagar con una tecla** (`C`): la guía pide comparar la misma vista con y sin la corrección. Por hardware habría que volver a crear las texturas con otro formato.
2. **No depende de la ventana:** `GL_FRAMEBUFFER_SRGB` necesita que el framebuffer de la ventana lo admita (`GLFW_SRGB_CAPABLE`, el controlador de video).
3. **Se ve en el código** qué color se convierte y dónde.

**Lo que hay que cuidar: no convertir dos veces.** Las texturas siguen en `GL_RGBA8` (no `GL_SRGB8_ALPHA8`) y no se activa `GL_FRAMEBUFFER_SRGB`. Si una textura sRGB pasara además por `a_lineal`, se elevaría a 2,2 dos veces: todo muy oscuro. Si se activara el framebuffer sRGB además de `a_srgb`, todo lavado.

**Lo que se pierde: el filtrado en lineal.** Con texturas sRGB, la GPU convierte cada texel **antes** de promediarlo (mipmaps, filtro bilineal). Con `pow` en el shader, el promedio se hace sobre valores sRGB y después se convierte. Ejemplo extremo, un texel negro y uno blanco: el promedio sRGB es 0,5, que en lineal es 0,22; el correcto es 0,5 en lineal (0,73 en sRGB). Los bordes muy contrastados, vistos de lejos, quedan un poco más oscuros. En el pasto, en el cielo y en el camuflaje, los texels vecinos tienen colores parecidos y no se nota.

## 18. ¿Por qué el color de la niebla se pasa a lineal en la CPU y los demás colores en el shader? ¿Y el color de borrado?

**La niebla, en la CPU, porque es un solo color para toda la escena.** Se calcula una vez (promedio del horizonte del cubemap) y es el mismo en todos los fragmentos. Convertirlo en el shader sería repetir el mismo `pow` millones de veces por cuadro. Es lo que sugiere la guía: `glm::pow(color, glm::vec3(2.2f))` antes de enviarlo. Como depende de la tecla `C`, `main` lo convierte en cada cuadro solo si la corrección está activa.

**Las texturas, en el shader, porque cambian en cada fragmento:** cada uno lee otro texel. El `kd` del avión podría convertirse en la CPU (es uno por pieza, como sugiere la guía), pero en el shader queda junto a la textura que lo multiplica y la tecla `C` lo apaga sin tocar la CPU.

**El color de borrado (`glClearColor`) no se convierte.** `glClear` escribe ese color **directo** en la pantalla, sin pasar por ningún shader (ni por `a_srgb`). Ya está en sRGB, que es lo que la pantalla espera. Si se usara el de la niebla en lineal, saldría más oscuro. (Casi no se ve: el fondo tapa toda la pantalla.)

## 19. ¿Por qué se bajó la luz ambiente a 0,1? ¿No queda todo más oscuro?

**No: la luz ambiente solo decide qué tan oscura es la sombra.** Lo que mira al sol recibe además la difusa, y con el sol en 0,9 recibe más que antes.

| | Cara en sombra (solo ambiente) | Terreno plano (sol a 30°, `N·L` = 0,5) |
|---|---|---|
| Parte 4: sin corrección, 0,3 / 0,7 | 30 % de la textura | 65 % |
| Corrección con 0,3 / 0,7 | **58 %** | 82 % |
| **Corrección con 0,1 / 0,9** (decisión 18) | **35 %** | **76 %** |

- Con la corrección y 0,3 de ambiente, la sombra se ve al 58 %: es casi tan clara como lo iluminado y **el relieve se aplana** (lo advierte la guía).
- Con 0,1 la sombra vuelve a estar cerca del 30 % de antes, pero lo iluminado queda **más claro** que en la Parte 4 (76 % contra 65 %): el terreno se parece más al del Práctico 08 (consulta 3).
- Siguen sumando 1: una cara de frente al sol llega justo a `kd` y no se quema.

