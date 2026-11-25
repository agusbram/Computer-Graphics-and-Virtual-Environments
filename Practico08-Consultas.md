# Práctico 08 — Consultas (preguntas y respuestas)

Preguntas frecuentes del **Práctico 08 (texturas y cielo)**, en el mismo formato que `Practico07-Consultas.md`.

La primera tanda son las dudas al leer la guía (`Practico 8/dudas`). Las respuestas están adaptadas a **este** proyecto: escena Z-arriba, origen flotante (el avión siempre en el origen) y terreno con LOD (geometry clipmaps de 51 km).

> Este archivo va a crecer durante la implementación. Al final se suman las respuestas a las "Algunas cuestiones a pensar" de la guía.

---

## Chuleta (resumen para refrescar)

| Concepto | En una línea |
|---|---|
| Vista sin traslación | `glm::mat4(glm::mat3(V))`: el cielo gira con la cámara pero nunca se acerca ni se aleja (sin paralaje). |
| `pos.xyww` | Fuerza `z = w` → después de dividir por `w`, la profundidad vale **1**, el plano lejano: el cielo queda detrás de todo. |
| `GL_LEQUAL` | El cielo (profundidad 1) pasa el test solo donde el z-buffer sigue en 1, o sea donde la escena no dibujó nada. |
| Cubo visto desde adentro | Sus triángulos se ven con el giro invertido (horario): el recorte de caras traseras los descarta todos. |
| Banda de altitud | El horizonte pintado queda a la altura del ojo y el borde del terreno "se hunde" `atan(h / (L/2))`; en el medio se ve la mitad de abajo del cubemap. |
| Tesela | Una copia de la imagen sobre el suelo; "metros por tesela" = cuánto suelo cubre esa copia. |
| `uniform` | Valor que pone la CPU, igual para todo el draw call. |
| `layout(binding = N)` | El sampler lee la **unidad de textura N**, sin tocar nada desde C++. |
| `GL_RGBA8` | Cómo se guarda cada texel en la GPU: 4 canales de 8 bits = 4 bytes. |
| `vDireccion` | Dirección desde el ojo (centro del cubo) hacia **ese** punto del cielo; cada píxel tiene la suya. |

---

## 1. ¿Por qué hay que quitarle la traslación a la matriz de vista para dibujar el cielo?

**Qué hace la matriz de vista.** `glm::lookAt(ojo, objetivo, arriba)` arma una matriz que se puede pensar en dos pasos:

```
V = R · T(−ojo)
      │     └── 1) traslada TODO el mundo para que el ojo quede en el origen
      └──────── 2) rota el mundo para que la cámara mire hacia −Z
```

**Qué tiene que pasar con el cielo.** El cielo representa algo **infinitamente lejano**. En la vida real, si caminás 100 m, las nubes y el horizonte no se corren: no hay **paralaje**, que es el efecto por el que lo cercano se desplaza más que lo lejano cuando uno se mueve. Lo único que cambia lo que ves del cielo es **girar la cabeza**. Entonces el cielo tiene que reaccionar a la rotación de la cámara (`R`) pero **no** a su posición (`T`).

**Qué pasaría si se usara la `V` completa.** El cubo quedaría **fijo en el mundo**. Al moverse el ojo:
- se acercaría a una cara y la vería más grande (paralaje: se nota que es una caja);
- si se alejara más de medio lado del cubo, **saldría del cubo**. Lo vería desde afuera, como una caja flotando, o no vería nada si el recorte de caras está activo.

**En tu proyecto pasaría enseguida.** Por el origen flotante, el cubo quedaría centrado en el **avión** (que está en el origen) y la cámara orbital está a 2–40 u. Con un cubo de lado 10, la cámara a 20 u queda **afuera**: verías una cajita con el cielo pegado alrededor del avión. Esa es la respuesta a la pregunta de la guía "¿qué pasa si se dibuja el fondo con la matriz de vista completa?".

**La solución, de dos formas equivalentes.**
1. **Mover el cubo con el ojo:** su matriz de modelo sería `M = T(ojo)`. Entonces `V · M = R · T(−ojo) · T(ojo) = R`: la traslación se **cancela**.
2. **Usar directamente `R`:** en una matriz 4×4 de este tipo, la parte de arriba a la izquierda de 3×3 es la rotación y la cuarta columna es la traslación.
   - `glm::mat3(V)` se queda con la 3×3.
   - `glm::mat4(...)` la vuelve a agrandar rellenando con la identidad, así que la traslación queda en cero.

   Es lo mismo que la opción 1, sin calcular nada nuevo: solo se copia y se recorta la matriz.

```cpp
const glm::mat4 vista_sin_traslacion = glm::mat4(glm::mat3(vista));   // solo la rotación
```

Por eso la guía dice que el **tamaño** del cubo no importa para la ilusión: siempre está centrado en el ojo y nunca te acercás a sus caras.

---

## 2. Método (b): `pos.xyww` y `glDepthFunc(GL_LEQUAL)`, ¿qué está pasando?

**Repaso: cómo se calcula la profundidad de un fragmento.**
1. El vertex shader escribe `gl_Position = (x, y, z, w)`, en **coordenadas de recorte**.
2. La GPU divide por `w`: `(x/w, y/w, z/w)`. Son las coordenadas normalizadas, todas en `[−1, 1]`.
3. La profundidad que se guarda en el z-buffer es `(z/w + 1) / 2`, en `[0, 1]`: **0 = plano cercano, 1 = plano lejano**.
4. `glClear(GL_DEPTH_BUFFER_BIT)` llena el z-buffer con **1.0**, lo más lejano posible.

**El truco `xyww`.** En el shader del cielo:

```glsl
vec4 pos    = uProjection * uView * vec4(aPos, 1.0);
gl_Position = pos.xyww;    // (x, y, w, w): se pisa z con w
```

- `x` e `y` no cambian, así que **la forma en pantalla es la misma**.
- `z` vale `w`, así que `z/w = 1` y la profundidad vale **1 para todos los fragmentos del cielo**. Queda "pegado" al plano lejano, sin importar el tamaño del cubo.

**Por qué hace falta `GL_LEQUAL`.** El test de profundidad compara el fragmento nuevo con lo que ya hay en el z-buffer. Por omisión (`GL_LESS`) pasa si `nuevo < guardado`.

| Píxel | Guardado en el z-buffer | Cielo con `GL_LESS` (`1 < guardado`) | Cielo con `GL_LEQUAL` (`1 ≤ guardado`) |
|---|---|---|---|
| Nadie dibujó ahí (solo el `glClear`) | 1.0 | 1 < 1: **falso**, no se dibuja ❌ | 1 ≤ 1: **verdadero**, se dibuja ✔ |
| Ahí hay terreno | 0.37 | 1 < 0.37: falso, no se dibuja ✔ | 1 ≤ 0.37: falso, no se dibuja ✔ |

- Con `GL_LESS` el cielo **no aparecería en ningún lado**.
- Con `GL_LEQUAL` aparece **justo** donde la escena no dibujó nada.
- Al terminar se vuelve a `GL_LESS` para no afectar el cuadro siguiente.

**La ventaja: no se calcula el color del cielo donde no hace falta.** El test de profundidad la GPU lo puede hacer **antes** del fragment shader (*early depth test*). Los píxeles del cielo tapados por el terreno o el avión se descartan sin ejecutar el shader, que en la Parte 5 va a ser el más caro (dos texturas y la intersección rayo-plano).

**Diferencia con el método (a).** En el (a) el cielo se dibuja primero, en todos los píxeles de la pantalla, y después la escena pinta encima. Se calcula el cielo en píxeles que después se tapan. Además, el cubo tiene que caber entre los planos `near` y `far`, porque si no se recorta. Con el (b) ese problema no existe, porque la profundidad se fuerza al plano lejano.

**Orden de dibujo en nuestro proyecto con (b):**

```
glClear  →  terreno + avión (z-buffer normal)  →  cielo (xyww + GL_LEQUAL)  →  glDepthFunc(GL_LESS)
```

---

## 3. ¿Por qué el recorte de caras traseras elimina las caras del cubo del cielo?

**Cómo decide OpenGL qué cara es trasera.** No mira las normales. Mira el **sentido de giro** (*winding*) de los tres vértices **tal como quedan proyectados en la pantalla**:
- Antihorario (`glFrontFace(GL_CCW)`): **cara frontal**, se dibuja.
- Horario: **cara trasera**, y `glCullFace(GL_BACK)` la descarta.

**El cubo de las primitivas.** Sus caras se armaron antihorarias **vistas desde afuera**, con la normal hacia afuera, porque es un objeto que se mira desde afuera.

**El cielo se mira desde adentro.** Un mismo triángulo, mirado del otro lado, muestra sus vértices en el orden **inverso**. Es como un reloj pintado en un vidrio: de frente gira en sentido horario y desde atrás se ve girar al revés.

```
Desde AFUERA del cubo:   A → B → C  antihorario   → frontal → se dibuja
Desde ADENTRO del cubo:  A → B → C  se ve horario → trasera → se DESCARTA
```

Como el ojo está **siempre adentro** del cubo, las **6 caras** se ven como traseras, se descartan todas y el cielo no aparece.

**Tres soluciones (alcanza con una):**
1. Desactivar el recorte para dibujar el cielo y reactivarlo después.
2. `glCullFace(GL_FRONT)` durante el cielo, para descartar las frontales. Desde adentro solo se ven traseras, así que quedan todas.
3. Invertir el orden de los índices del cubo, para armar un cubo "dado vuelta" que se vea de frente desde adentro.

---

## 4. Si ya tenemos niveles de detalle (LOD), ¿hace falta "continuar el suelo en el fondo"?

**Respuesta corta: sí, conviene hacer las dos cosas.** Son soluciones **complementarias**, y las texturas no cambian el problema: lo cambia el tamaño del terreno.

**Qué resuelve el LOD.** El LOD sí es la solución "de verdad", la que la guía dice que usan los simuladores. Tu terreno ya mide **51 km** de lado (borde a 25,6 km), así que la banda es mucho más chica que con la ventana de 5 km. Pero **no es cero**, porque el terreno sigue siendo finito:

| Altura del ojo sobre el terreno | Banda con el clipmap (borde a 25,6 km) | Banda solo con el nivel fino (borde a 1,6 km) | Cuánto baja el horizonte real (curvatura) |
|---|---|---|---|
| 150 m (vuelo actual) | **0,34°** | 5,4° | 0,39° |
| 1000 m | **2,2°** | 32° | 1,0° |
| 5000 m | **11°** | 72° | 2,3° |

Si el avión sube 1 o 5 km sobre el terreno, la banda vuelve a aparecer.

**Qué resuelve el suelo continuado.** Pinta la banda con la misma textura de suelo, **a cualquier altitud**, y cuesta casi nada porque se hace en el shader del fondo, que de todos modos se ejecuta. La guía además pide **implementarlo y verificarlo** ("verificar que la banda desaparece a cualquier altitud").

**Dónde entran las texturas.** El suelo continuado tiene que usar **la misma textura y la misma escala** que la malla. Si no, en el borde se ve el cambio de tamaño del pasto. Eso nos obliga a algo que el LOD ya pedía: calcular las coordenadas de textura desde la **posición en el mundo** (ver la pregunta 6).

**Cómo lo vamos a mostrar:**
1. Medir la banda **solo con el nivel fino**, como en el Práctico 07, y **con el clipmap completo**.
2. Comparar contra la fórmula.
3. Activar el suelo continuado y ver que desaparece.

Es un buen material para la defensa: el LOD reduce el problema y el suelo continuado lo tapa del todo.

---

## 5. ¿Qué es exactamente el problema de la altitud? ¿Qué pasa con "el plano" y con lo que se ve a lo lejos?

**Dos objetos que se comportan distinto:**
- **El cielo (cubemap)** está centrado en el ojo y lo acompaña. Su horizonte, la línea pintada a media altura de las caras laterales, queda **siempre a la altura del ojo**: en la dirección horizontal, a 0° de elevación.
- **El terreno** está quieto a una altura fija y es **finito**: su borde está a una distancia `L/2` del centro.

**Qué pasa al subir.** El borde del terreno queda `h` metros más abajo que el ojo y a `L/2` de distancia. Se ve **por debajo** de la horizontal, a un ángulo:

```
                     ojo ●───────────────────────────── horizonte pintado del cubemap (0°)
                         │ ╲  ← banda: acá NO hay terreno,
                       h │   ╲     se ve la mitad de ABAJO del cubemap
                         │     ╲ atan(h / (L/2))
  ═══════════════════════╧═══════● borde del terreno
  ◄──────────── L/2 ────────────►
```

Entre el horizonte pintado (0°) y el borde del terreno (`−atan(h/(L/2))`) queda una **franja** donde no se dibujó geometría. Ahí se ve el fondo, o sea la parte del cubemap por debajo del horizonte.
- En el cubemap del aula, esa mitad de abajo es un **reflejo del cielo**, parecido a agua.
- El efecto es una "alfombra de terreno flotando" sobre un cielo.
- Cuanto más alto (`h` mayor) o más chico el terreno (`L` menor), **más ancha la banda**.

**No es la curvatura de la Tierra.** El horizonte real también baja al subir, pero muy poco (última columna de la tabla de la pregunta 4). La causa es que **el terreno termina antes del horizonte**.

**"El plano" de la solución.** Es un **piso imaginario, plano e infinito**, a la altura media del terreno. Para cada píxel del fondo **por debajo del horizonte**, el shader calcula dónde el rayo que sale del ojo en esa dirección **choca contra ese piso**, y pinta el pasto que habría en ese punto. El rayo es `P(t) = ojo + t·d`, y la intersección es la misma fórmula rayo-plano que se vio en ray tracing el 4-nov.
- El plano no se dibuja como geometría: **es una cuenta en el shader del fondo**.
- Como el terreno real se dibuja **encima** (está más cerca), el piso imaginario solo se ve donde la malla no llega, es decir, en la banda.

**Adaptación a nuestro proyecto.** La guía usa `y` como altura. En nuestra escena Z-arriba:
- el plano es `z = altura_terreno`;
- la condición "debajo del horizonte" es `d.z < 0`;
- el punto del suelo es `(x, y)` en vez de `(x, z)`.

---

## 6. ¿Qué es una tesela en el terreno? ¿Para qué se usa en el problema de la altitud?

**Tesela es una "baldosa".** Es **una copia** de la imagen de la textura apoyada sobre el suelo. La textura del pasto es un cuadrado de 256×256 texels. Con `GL_REPEAT`, si las coordenadas de textura `(u, v)` van de 0 a 40, la imagen se repite **40 veces** por lado: 40 teselas. Los "**metros por tesela**" son cuánto suelo cubre **una** copia.

**Por qué no estirar una sola copia sobre todo el terreno:**

| | Una sola copia sobre 2 km | 40 teselas sobre 2 km (50 m cada una) |
|---|---|---|
| Tamaño de un texel en el suelo | 2000 m / 256 ≈ **7,8 m** | 50 m / 256 ≈ **0,2 m** |
| Cómo se ve de cerca | "píxeles enormes", borroso | pasto con detalle |

Lo malo de repetir es que, desde muy alto, se puede notar el patrón repetido (*tiling*). El mipmap lo suaviza hacia el horizonte.

**En nuestro proyecto, con LOD**, las coordenadas no pueden ser `(j/N, i/N)`, porque cada nivel tiene otro tamaño y el pasto cambiaría de escala en cada anillo. Se calculan desde la posición en el mundo:

```
(u, v) = (x, y) / tamaño_tesela          // ej.: 50 m = 5 u por tesela
```

Así, cada 50 m de mundo la imagen se repite, **igual en todos los niveles** y aunque la ventana se mueva.

**Uso en el problema de la altitud.** El suelo continuado calcula el punto del piso imaginario `(x, y)` y le aplica **la misma regla**: lo divide por los metros por tesela. Como la malla y el fondo usan la misma escala y el mismo origen, el patrón del pasto **sigue sin cortes** en el borde. Si el fondo usara otra escala, en el borde se vería el pasto cambiar de tamaño y se notaría dónde termina la malla.

---

## 7. ¿Qué hacen las variables `uniform`? ¿Y `layout (binding = …)`?

**Tres formas de que le llegue un dato a un shader:**

| Tipo | Qué es | Cambia… | Ejemplo en el proyecto |
|---|---|---|---|
| **Atributo** (`in` del vertex shader) | dato de **cada vértice** | por vértice | `aPos`, `aNormal`, `aTexCoords`, `aZc` |
| **Varying** (`out` del VS → `in` del FS) | lo que el VS le pasa al FS; el rasterizador lo **interpola** | por fragmento | `vNormal`, `vDistancia` |
| **Uniform** | valor que pone la **CPU** antes de dibujar | por draw call (igual para todos los vértices y fragmentos) | `uModel`, `uView`, `uProjection`, `uColor`, `uMorph`, la niebla |

Un uniform es como un "parámetro global" del shader: se setea con `shader.set_uniform(loc, valor)` y vale lo mismo para toda la malla de ese `glDrawElements`. Por ejemplo, `uModel` cambia de pieza en pieza, pero dentro de una pieza todos los vértices usan la misma.

**Los samplers son uniforms especiales.** `sampler2D` o `samplerCube` declaran "acá se lee una textura". Su valor **no** es el identificador de la textura, sino el **número de unidad de textura**:
- La GPU tiene **unidades de textura numeradas** (0, 1, 2…). OpenGL 4.6 garantiza al menos 16 por etapa.
- En la CPU se **enchufa** una textura en una unidad: `glBindTextureUnit(0, id)`, que va a ser `Texture::bind(0)`.
- En el shader, el sampler **lee de esa unidad**.

**`layout (binding = N)`** fija en el **código del shader** de qué unidad lee el sampler:

```glsl
layout (binding = 0) uniform samplerCube uCielo;          // lee la unidad 0
layout (binding = 1) uniform sampler2D   uTexturaSuelo;   // lee la unidad 1
```

Sin `binding`, habría que decírselo desde C++ con `glProgramUniform1i(programa, loc, 0)`. Con `binding`, alcanza con hacer `bind(0)` y `bind(1)` antes de dibujar. Es lo mismo que `layout (location = N)` para los atributos: el número se fija en el shader y la CPU solo respeta la convención.

---

## 8. ¿Qué hace `vec4 suelo = texture(uTexturaSuelo, xz / uMetrosTesela);`?

Paso a paso, con un ejemplo (50 m por tesela):

1. `xz` es la posición horizontal, en metros, del punto del **piso imaginario** donde chocó el rayo de ese píxel. En nuestra escena Z-arriba es `xy`. Ejemplo: `(1234, −560)`.
2. `xz / uMetrosTesela` lo pasa a **unidades de tesela**: `(24,68, −11,2)`. Significa "24,68 baldosas en x y −11,2 en y".
3. Con `GL_REPEAT`, la GPU usa solo la **parte fraccionaria**: `(0,68, 0,8)`, porque −11,2 se toma como 0,8. Es el punto al 68 % del ancho y al 80 % del alto **dentro de una baldosa**.
4. `texture(sampler, uv)` **lee la imagen** en ese punto con el filtrado configurado (lineal + mipmaps). La GPU elige el nivel de mipmap según cuánto cambian las coordenadas entre píxeles vecinos.
5. Devuelve un `vec4` (R, G, B, A) con valores en `[0, 1]`: **el color del pasto que habría en ese punto del piso**.

Es exactamente la **misma cuenta** que hace la malla del terreno (pregunta 6), por eso las dos coinciden en el borde.

---

## 9. ¿Qué es `GL_RGBA8`? ¿`vDireccion` es el vector desde el observador hacia donde se mira?

**`GL_RGBA8`** es el **formato interno**: cómo se guarda **cada texel en la memoria de la GPU**.
- **RGBA:** 4 canales (rojo, verde, azul y alfa, la opacidad).
- **8:** 8 bits por canal, o sea valores 0–255. En el shader llegan como `0.0–1.0` (formato "normalizado").
- Ocupa **4 bytes por texel**. El pasto de 256×256 ocupa 256 KiB, más 1/3 por los mipmaps: unos 341 KiB.
- `GL_RGB8` usa 3 canales, sin alfa. Para el cielo no hace falta transparencia: el cubemap de 6 × 900 × 900 ocupa ≈ 13,9 MiB en RGB8 contra 18,5 MiB en RGBA8. Algunos drivers igual lo guardan internamente con 4 bytes.
- **No confundir** con el par `GL_RGBA` + `GL_UNSIGNED_BYTE` de `glTextureSubImage2D`. Ese par describe **los bytes que manda la CPU**, que es la imagen decodificada por stb_image. Los dos formatos pueden ser distintos: OpenGL convierte.

**`vDireccion`, casi.** Es el vector que sale del **centro del cubo** hacia el **punto del cubo que se está dibujando**. Como el cubo está centrado en el ojo (pregunta 1), es la dirección **desde el observador hacia ese punto del cielo**.
- **No** es "hacia donde mira la cámara", que es un único vector para toda la pantalla. **Cada píxel tiene su propia dirección**: el rasterizador la interpola entre los vértices.
- Está en la orientación del **mundo**: no gira con la cámara, porque la rotación la aplica `uView` en `gl_Position`, no en `vDireccion`. Por eso el cielo queda "pegado al mundo" al girar.
- No hace falta que sea unitario para leer el cubemap: OpenGL elige la cara por la componente de mayor valor absoluto. Sí hay que normalizarlo para cuentas como la elevación o la intersección con el plano.
- **En nuestro proyecto** el cubemap de OpenGL tiene "arriba" en +Y y nuestra escena tiene arriba en +Z. Antes de leer el cubemap hay que reacomodar la dirección, `(x, y, z) → (x, z, −y)`, que es una rotación de −90° alrededor de X.

---

## 10. ¿Se puede usar el "cielo pintado por elevación" para hacer día y noche? ¿Hacen falta otras texturas? ¿Cómo se soluciona que el fondo quede "simple"?

**Sí se puede, y no es muy complicado si se hace por capas.** Es una **mejora** opcional, no parte obligatoria del Práctico 08. El profesor nombró el ciclo día/noche como mejora el 14-oct, y el proyecto pide al menos 3 efectos gráficos. Primero hay que tener el cubemap andando, porque el cielo es parte de la demo del checkpoint del 27-nov.

**El problema que menciona la guía.** Un degradé por elevación queda "liso": no tiene nubes, sol ni estrellas. La solución es **agregar capas encima** en el shader del fondo, todas controladas por **una sola variable, `hora`**, que se puede animar y adelantar con una tecla.

| Capa | Qué pinta | Cómo se hace | ¿Textura de internet? |
|---|---|---|---|
| 1. Degradé base | color del cielo según la elevación **y la hora** (noche, amanecer, día, atardecer) | textura chica generada por código de 64 × 4 texels: eje s = elevación, eje t = hora. Es la textura 1D de la guía "con una dimensión más". | no |
| 2. Sol | disco brillante + halo | en el shader: `dot(d, dir_sol)` cerca de 1 → brillo. `dir_sol` sale de la `hora`. | no |
| 3. Estrellas / Vía Láctea | cielo nocturno real | cubemap nocturno que se **funde** con `mix` según cuán de noche es | **sí** (ver abajo) |
| 4. Nubes | nubes que se mueven | plano de nubes a gran altura: **la misma intersección rayo-plano** del suelo continuado, pero con un plano arriba y una textura de nubes con alfa | sí (o ruido procedural) |
| 5. Suelo continuado y niebla | debajo del horizonte | Parte 5, con el color del horizonte **de esa hora** | no |
| 6. Oscurecer la escena | terreno y avión más oscuros de noche | por ahora un factor de luz que depende de la `hora`; en el Práctico 09 pasa a ser la luz direccional del sol, con la **misma** `dir_sol` | no |

**Por qué no alcanza con el cubemap del aula para día/noche.** Tiene el **sol pintado** (el resplandor de la cara izquierda) y nubes de día. Si el sol procedural se mueve, el pintado queda quieto y se contradicen. Por eso el cubemap del aula queda como el **cielo del Práctico 08** (lo que pide la guía) y el día/noche se arma con capas.

**Texturas encontradas (licencia libre):**
- **Cielo nocturno:** [Rogland Clear Night — Poly Haven](https://polyhaven.com/a/rogland_clear_night), con estrellas y la Vía Láctea. **CC0** según la [licencia de Poly Haven](https://polyhaven.com/license): se puede usar sin atribución. Viene como panorama (*equirectangular*), así que hay que convertirlo una vez a 6 caras con un script de Python (numpy + PIL, ya instalados). La versión JPG pesa ~54 MB y se reduce al convertir.
- **Paquete de cielos ya en cubemap:** [Cloudy Skyboxes — Screaming Brain Studios (OpenGameArt)](https://opengameart.org/content/cloudy-skyboxes-0), **CC0**. Son 50 cielos (días soleados, amaneceres/atardeceres y noches) en un zip de ~22 MB, pero con caras de **512 px**, más chicas que las 900 px del aula.
- **Nubes con transparencia:** [Clouds with Transparency — WickedInsignia (OpenGameArt)](https://opengameart.org/content/clouds-with-transparency-fxcloudalpha01png), **CC0**, 2048×2048 con canal alfa. Falta comprobar si son repetibles (*tileable*).
- **Evitar** [Night skyboxes — Humus (OpenGameArt)](https://opengameart.org/content/night-skyboxes): es CC-BY 3.0 (exige atribución) y tiene edificios en el horizonte.

**Dos caminos posibles:**
- **A, por capas** (recomendado): degradé por hora + sol + estrellas de Poly Haven + nubes en un plano. Todo es coherente, porque el sol y la luz del Práctico 09 salen de la misma `hora`, y cada capa es defendible por separado.
- **B, mezclar cubemaps:** día (aula) → atardecer → noche (Screaming Brain Studios), con `mix` según la hora. Es más simple, pero los soles pintados no se mueven y las caras son de 512 px.

Las tres fuentes dicen ser CC0, pero antes de usar cada archivo conviene confirmar la licencia en la página de descarga. Se van a anotar en `assets/textures/LICENCIAS.md`.

---

# Consultas sobre las decisiones del plan (después de leer el Práctico 09)

> Contexto: antes de implementar se leyó el **Práctico 09** (iluminación y niebla), para separar qué corresponde hacer ahora y qué después.

## 11. ¿Cómo quería el profe que usemos la arena (`sand.png`)? ¿Se puede poner pasto en una zona y arena en otra?

**La guía no indica cómo usarla.** Dice que en el aula quedan dos texturas de suelo "para poder hacer las pruebas; también puede usarse cualquier otra imagen". Son **alternativas para probar**: con una sola textura se cumple el práctico. Usar las dos es un **extra**.

Si se usan las dos, hay tres formas:

| Forma | Qué se ve | Cómo se decide qué textura va en cada punto |
|---|---|---|
| **Una sola** (pasto) | todo pasto | — (lo que pide la guía) |
| **Por altura** | arena en los valles (como lechos secos o playas), pasto en las lomas | en el fragment shader, con la **altura** del fragmento: `peso = smoothstep(h1, h2, altura)` y `mix(arena, pasto, peso)`; la franja `h1..h2` hace la transición suave |
| **Por regiones ("biomas")**, tu idea | volando un rato se pasa de una zona de pasto a un desierto | un **mapa de biomas**: ruido de **muy baja frecuencia** (FastNoiseLite, como el relieve) que dice "acá desierto, acá pradera"; se mezcla con `smoothstep` en el límite para que no haya un corte |

"Mezcla según la altura" quiere decir que la textura se elige mirando **a qué altura está cada punto** del terreno, no dónde está en el mapa.

**Cómo se haría la opción por regiones sin costuras.** El mapa de biomas se guarda en una **textura chica**, por ejemplo de 256×256, generada en la CPU con FastNoiseLite, con `GL_REPEAT` y `GL_LINEAR`. Se lee **con la posición en el mundo** desde el shader del terreno **y** desde el del fondo (suelo continuado), así los dos coinciden en el borde. Es un ejemplo de "una textura es un arreglo de datos" (filmina del 16-oct) y responde a la última cuestión de la guía: guardar datos del terreno en una textura.

**Recomendación:** hacer primero el práctico con **pasto** solo y dejar la mezcla por regiones como **extra**, cuando lo obligatorio ande.

## 12. ¿Cómo sabías que el Práctico 09 pide el promedio del horizonte para la niebla?

No lo sabía cuando lo recomendé: lo propuse porque era lo más simple. Al leer el Práctico 09 se confirma que es **exactamente** lo que pide la Parte 4:

> "el color de la niebla tiene que ser el del horizonte del cubemap. Se puede determinar una sola vez, en el CPU, al cargar las imágenes del cielo como el promedio de los píxeles de la fila central de las cuatro caras laterales del cubemap […] Las componentes del promedio se dividen por 255."

**Qué se hace en cada práctico:**
- **Práctico 08 (ahora):** se calcula ese promedio al cargar el cubemap, **antes** de liberar la imagen con `unload_image` (decisión 1). Se usa como color de la niebla que ya tenemos, para que no aparezca una franja gris contra el cielo nuevo. La niebla sigue siendo la lineal del Práctico 07.
- **Práctico 09:**
  - la niebla pasa a ser **exponencial** (`1 − e^(−d/V)`) y su código va a un archivo común;
  - el color se pasa a **lineal** (corrección gamma);
  - se aplica también a la **aeronave** y al **cielo cerca del horizonte** (`1 − smoothstep(0, 0.1, elevación)`).

**Dato a favor del LOD.** El Práctico 09 pide que en el borde del terreno la niebla sea del 92 %, o sea `L/2 ≥ 2,5·V`. Con la ventana de 2 km de la guía, la visibilidad máxima es 400 m ("muy cerrada para un simulador"). Con nuestro clipmap (L/2 = 25,6 km) puede llegar a **~10 km**.

## 13. ¿Me podés generar la imagen de prueba? ¿Lo propone la guía? ¿Cómo la hago yo para que el profe no vea que la hiciste vos?

**Qué pide la guía.** No dice quién hace la imagen: solo que sea **asimétrica en los dos ejes**, "por ejemplo: una letra, un número, un texto". Un damero no sirve, porque invertido o espejado se ve igual. Cualquier imagen que cumpla eso vale.

**Sobre "que no se note".** Las reglas del curso permiten usar IA, pero tiene que estar **declarada y ser defendible** (lo dice el README del proyecto). No te voy a ayudar a esconderlo, y tampoco hace falta. Tenés dos caminos válidos:
1. **La genero yo** con un script de Python que queda en el repositorio (`tools/`). Es reproducible y se declara como el resto del uso de IA.
2. **La hacés vos en 5 minutos**, y queda 100 % tuya:
   - En GIMP, Paint o LibreOffice Draw: lienzo de 512×512 con fondo claro.
   - Una **"F" grande** arriba a la izquierda (la F no tiene simetría ni horizontal ni vertical).
   - Un **número** (por ejemplo "2") abajo a la derecha, de otro color.
   - Una **flecha hacia arriba** con la palabra "ARRIBA".
   - Opcional: en la esquina de abajo a la izquierda el texto "(0,0)" y en la de arriba a la derecha "(1,1)". Así se ve enseguida si la orientación es la de OpenGL.
   - Exportar como PNG.

   Incluso sirve una **foto** con el celular de una hoja con eso escrito a mano.

Para defenderla en la evaluación, lo importante es saber **por qué** tiene que ser asimétrica y qué se mira en cada primitiva.

## 14. ¿Cuáles son las teclas de depuración para medir la banda?

Son tres. Las dos primeras eran las del plan; la tercera se agrega para poder comparar:

| Tecla (propuesta) | Qué hace | Para qué |
|---|---|---|
| `RePág` / `AvPág` | **sube o baja el terreno** respecto del avión, cambiando la altura sobre el terreno, sin tener que pilotear | medir a qué altura aparece la banda (pilotear hasta 1–5 km sobre el terreno con el elevador es lento e impreciso) |
| `1` | **solo el nivel fino** del clipmap: se dibuja únicamente la ventana de 3,2 km, como el terreno del Práctico 07 | reproducir el experimento de la guía, con una banda grande enseguida |
| `G` | **suelo continuado on/off** | ver la banda (off) y comprobar que desaparece (on) a cualquier altura |

Con las tres se arma la tabla de la guía, "altitud a la que aparece la banda" con y sin LOD, comparada con la fórmula `atan(h / (L/2))`.

## 15. Día/noche y nubes: ¿ahora o en el Práctico 09? ¿Se pueden reutilizar las texturas del profe?

**Va en el Práctico 09.** Es su apartado **Opcional — Ciclo día/noche**, y la forma que propone **reutiliza el cubemap del profe**:
- el sol gira alrededor de un eje horizontal (sale, mediodía, se pone);
- su color se vuelve anaranjado cerca del horizonte;
- se **apaga** debajo del horizonte, porque Phong no tiene sombras y si no iluminaría la panza del avión a través del terreno;
- la luz ambiente, **el cubemap y el color de la niebla se multiplican por un factor** que depende de la hora.

Así se cumple lo que querías: el cielo del profe se reutiliza, oscurecido de noche.

**Las nubes ya están** en el cubemap del profe, así que el plano de nubes CC0 deja de hacer falta. Solo valdría la pena si se quieren nubes que **se muevan**.

**Las capas propias quedan como agregados opcionales del Práctico 09**, encima de la propuesta de la guía: estrellas de noche (Poly Haven, CC0) y el degradé 1D del Práctico 08 para teñir el amanecer o el atardecer.

**Una limitación a tener en cuenta.** El cubemap tiene el sol **pintado**. Con el ciclo, ese sol pintado no se mueve, aunque de noche queda oscurecido. El Práctico 09 pide que `uSolDir` coincida con el sol pintado: en el Práctico 08 se va a **medir y anotar** la dirección de ese sol (el píxel más brillante de las caras) para usarla en el Práctico 09.

---

# "Algunas cuestiones a pensar" de la guía (respondidas después de implementar)

## 16. `stbi_set_flip_vertically_on_load` es un estado global. Si queda en 1 después del terreno, ¿qué le pasa al cielo? ¿Y si el caché guarda la misma imagen invertida y sin invertir?

**El cielo saldría mal.** El flip no es un parámetro de cada carga: queda "prendido" en la biblioteca. Si se cargara el pasto con flip 1 y después las caras del cubemap sin volver a fijarlo, las caras saldrían **patas arriba**: el cielo quedaría abajo y el horizonte no empalmaría entre caras.

**Nuestra solución:** `ResourceManager::load_image` llama a `stbi_set_flip_vertically_on_load(flip)` **en cada carga**, con el valor que pide esa imagen. El test 9 lo verifica: carga la misma imagen con y sin flip y comprueba que la fila 0 de una es la última de la otra.

**El caché.** Si el caché fuera **por archivo**, pedir `grass.png` invertida y después sin invertir devolvería la primera versión: la segunda vendría **con la orientación equivocada** sin ningún aviso. En nuestro diseño el caché es **por clave** y recuerda con qué flip se cargó cada una:
- si la misma clave llega con otro flip, **avisa** y gana la primera carga (test 10);
- para tener las dos versiones se usan **dos claves** distintas.

## 17. Con `GL_REPEAT` y `GL_MIRRORED_REPEAT` un damero se ve idéntico. ¿Por qué? ¿Qué tiene que tener una imagen de prueba para distinguirlos?

- `GL_REPEAT` pone las copias **una al lado de la otra**: F F F.
- `GL_MIRRORED_REPEAT` pone una normal y la siguiente **reflejada**: F ꟻ F.

Un damero típico es **simétrico** respecto de su eje central: reflejado queda igual (pasa, por ejemplo, con una cantidad **impar** de casillas por lado). Si la copia reflejada es idéntica a la original, "F F" y "F ꟻ" son el mismo dibujo, y las dos repeticiones no se pueden distinguir.

Para distinguirlos, la imagen tiene que ser **asimétrica en el eje en que se repite**. Así una copia reflejada se ve distinta. Nuestra `prueba_uv.png` sirve: con `MIRRORED_REPEAT` se verían "F" espejadas en una copia sí y otra no. También sirven las barras de color de un solo borde: con `REPEAT` la barra roja aparece cada una copia, y con `MIRRORED_REPEAT` aparecen **dos barras pegadas** cada dos copias.

## 18. Un cubemap de 1024×1024 ocupa unas diez veces más que el terreno entero. ¿Vale la pena generarle mipmaps? ¿Cuándo un texel del cielo se ve reducido?

**En nuestro caso no vale la pena, y no se generaron** (`niveles_ = 1`). Los mipmaps suman un 33 % de memoria y solo sirven cuando la textura se **reduce** (un fragmento abarca varios texels). El cielo casi siempre se **amplía**:
- una cara cubre 90° del campo visual;
- con `fovy` = 45°, 90° ocupan **dos veces la altura de la ventana** en píxeles: unos 1200 px en una ventana de 600;
- una cara de 900 o 1024 texels repartida en más de 1200 px se amplía.

**Cuándo se reduciría:**
- con una ventana **chica**: en 45°, con menos de unos 450–512 px de alto, 90° quedan en menos píxeles que texels;
- con un **campo visual muy amplio**: un gran angular comprime más grados en la misma pantalla;
- al usar el cubemap como **reflejo** sobre superficies curvas (opcional del Práctico 09): una esfera chica refleja todo el cielo en pocos píxeles. Ahí sí convendrían mipmaps.

## 19. ¿Qué pasa si se dibuja el fondo con la matriz de vista completa, sin quitarle la traslación?

**Predicción para nuestro proyecto.** Por el origen flotante, el mundo se dibuja relativo al avión, que está en el origen. Con la vista completa, el cubo del fondo (1 u de lado, centrado en el origen) quedaría **centrado en el avión**, como una caja chica a su alrededor. La cámara orbital está a 2–40 u, o sea **afuera** de esa caja. Se vería:
- una **cajita** alrededor del avión, del tamaño del avión, con pedazos del cielo pintados en sus caras;
- en el resto de la pantalla, el color de borrado, porque el cubo ya no cubre todo;
- al **mover** la cámara, la caja tendría paralaje como cualquier objeto: se acercaría, se alejaría y se vería en perspectiva.

Con `xyww` la caja sigue quedando "detrás de todo" en profundidad, pero ocupa poca pantalla.

Si la cámara estuviera **adentro** de un cubo grande y fijo, el cielo se vería bien hasta que el ojo se acercara a una cara: se notaría la caja (paralaje) y al cruzarla se "saldría" de la escena.

## 20. Con iluminación difusa, ¿qué hay que agregar al shader del fondo para que el suelo continuado no se distinga de la malla? ¿Qué normal tiene ese plano?

Hay que **iluminar el suelo continuado con la misma función, el mismo sol y el mismo material que la malla**: `kd` = la textura del pasto, `ks = 0`. Hay que usar el punto `P(t)` del plano y su normal. Esa normal es la del plano horizontal, **(0, 0, 1)** en nuestra escena Z-arriba ((0, 1, 0) en la guía, que usa Y-arriba).

**Limitación:** la malla tiene relieve y sus normales varían. El plano tiene una sola normal. Si el borde del terreno tiene laderas, se notaría la diferencia de luz, y la **niebla** es lo que la disimula. Con nuestro clipmap el borde está a 25,6 km, totalmente en la niebla. Esto se implementa en el Práctico 09 (Parte 2).

## 21. El heightmap del Práctico 07 es una grilla de alturas indexada por (i, j). ¿Qué haría falta para guardarlo como textura y qué ganaría el programa?

**Qué haría falta:**
- una textura de **1 canal float** (`GL_R32F`) por nivel, con la altura de cada nodo;
- en el vertex shader, leerla con `texelFetch` (*vertex texture fetch*) usando `(i, j)`;
- el vertex buffer tendría solo `(x, y)` de una grilla regular y la altura saldría de la textura;
- al moverse la ventana, en vez de recalcular y re-subir todos los vértices, se actualizaría solo la franja nueva de la textura (`glTextureSubImage2D`) con direccionamiento toroidal.

**Qué se ganaría** (es la versión GPU de los geometry clipmaps, GPU Gems 2 cap. 2, Tabla 2-1):
- **vertex e index buffers constantes**: se cargan una vez y no se tocan más;
- **menos trabajo de la CPU**: solo se escribe la franja nueva;
- la **z_c del morph y las normales** se podrían calcular en la GPU leyendo vecinos de la textura;
- el **suelo continuado** podría leer la altura real en lugar de un plano.

El costo es que la lectura de textura en el vertex shader es más cara: en GPU Gems sacarla subía de 130 a 185 fps.

---

# Consultas después de probarlo

## 22. Al subir se ve una franja grisácea con una línea recta en el horizonte. ¿Por qué no se ven las nubes ahí?

**Debajo de esa línea no hay cielo: es suelo.** Es el suelo continuado lejano (y el borde del terreno) **con niebla al 100 %**, o sea pintado exactamente del color de la niebla. Las nubes están en el cielo, por **encima** del horizonte. Visto desde arriba, lo que queda debajo de la línea del horizonte es siempre suelo, aunque esté muy lejos.

**Por qué crece al subir.** El suelo "totalmente en la niebla" empieza a la distancia donde la niebla llega al 100 % (18 km). Desde la altura `h`, ese límite se ve `atan(h / 18 km)` por debajo del horizonte: a ~1,2 km son unos 3,8°, una franja ancha y pareja.

**Por qué hay una línea.** El suelo llega al horizonte con el color **promedio** del horizonte (el de la niebla). El cielo justo arriba tiene el color del cubemap **en esa dirección**: nubes más oscuras o más claras. Dos colores distintos que se tocan sin mezclarse forman una línea recta.

**Cómo se arregla (en el Práctico 09):**
1. **Fundir el cielo con la niebla cerca del horizonte** (`fondo.fs`): `mix(cielo, niebla, 1 − smoothstep(0, 0.1, elevación))`. A elevación 0 cielo y suelo valen lo mismo, la línea desaparece y queda una bruma suave. Es justo lo que pide la **Parte 4 del Práctico 09**.
2. Con la niebla **exponencial** del 09, la franja depende de la visibilidad `V` y no de un "fin" fijo. Correr el fin de la niebla lineal más lejos (por ejemplo a 25 km) angosta la franja, pero **no** borra la línea.

**Decisión: queda como está en el 08.** Se probó adelantar los dos cambios y se volvió atrás: el arreglo es parte del Práctico 09, y la regla del proyecto es que cada práctico haga lo suyo para poder defenderlo por separado (ver `Practico09-Pendientes.md`). Con `N` se apaga la niebla y se ve el terreno hasta el borde; con eso se comprueba que la franja es niebla y no un error del cielo.

Algo de bruma en el horizonte va a quedar siempre que haya niebla, también en el 09. Es la **perspectiva atmosférica**: en la realidad, desde un avión, el horizonte también se ve brumoso.

## 23. La chapa no me gusta: el avión sigue pareciendo de juguete. ¿No se puede poner otra textura?

**El problema no era la textura sino los colores.** La chapa **tiñe** el color de cada pieza, y esos colores (rojo, azul, amarillo, violeta) se eligieron en el Práctico 04 para **distinguir las piezas**. Con cualquier textura que tiña esos colores, el avión sigue pareciendo de juguete.

**Solución: tres apariencias** con la tecla `T`:

| Apariencia | Qué es |
|---|---|
| **Camuflaje** (la inicial) | una textura de camuflaje de 4 tonos (verde oscuro, oliva, marrón y arena) **generada** con `tools/generar_camuflaje.py`. Se repite sin costura y tiene incorporados los rayones y remaches de la chapa. El color sale de la textura, no de la pieza. |
| **Metal gris** | la chapa tiñe un **gris metálico** igual para todas las piezas, con la nariz más oscura (radomo): un jet de entrenamiento sin pintar |
| **Colores de las piezas** | los del Práctico 04 (útiles para explicar el despiece) |

**¿Por qué generar el camuflaje y no bajarlo?** No apareció una textura de camuflaje **CC0** que se repitiera bien. Generada por script es reproducible, tiene licencia clara (propia, derivada de la chapa CC0) y se puede ajustar: el tamaño de las manchas y los colores son parámetros del script.

**Sin complicaciones de código:** el shader no cambió (sigue siendo `color × textura`). Solo cambia qué color manda `Aircraft`: blanco para el camuflaje, el gris compensado para el metal.

## 24. Sin niebla, si subo mucho, el campo verde lejano queda recto en el horizonte y no se nota que estoy subiendo. ¿Por qué?

**Porque el suelo lejano es un plano infinito.** Más allá del borde del clipmap (25,6 km) lo que se ve es el **suelo continuado**: un plano horizontal sin fin. La línea donde termina un plano infinito (su "línea de fuga") está **siempre a la altura del ojo**, a elevación 0, sea cual sea la altura. Por eso el horizonte no se mueve al subir.

**Y no tiene nada que cambie de tamaño.** Para notar la altura, el ojo compara tamaños: si las cosas del suelo se achican, uno está subiendo. Acá lo único con tamaño propio es:
- la **tesela del pasto** (50 m): de lejos, o desde muy alto, los **mipmaps** la promedian a un verde parejo (es lo correcto: si no, parpadearía);
- el **relieve** del terreno: termina en el borde del clipmap y después todo es plano.

Un plano de un solo color se ve **idéntico** desde 1 km o desde 10 km: si se duplica la altura, se duplican todas las distancias y la imagen no cambia. El campo verde lejano queda "recto" y sin pistas de altura.

**Qué da la sensación de altura en la realidad:**

| Pista | En la realidad | En el proyecto |
|---|---|---|
| **Curvatura de la Tierra** | el horizonte **baja** por debajo de la altura del ojo: ≈ 1° a 1 km, 2,3° a 5 km, 3,2° a 10 km (unos 13, 30 y 43 píxeles con nuestro campo de visión de 45° y 600 px) | no hay: el suelo es plano |
| **Bruma (perspectiva atmosférica)** | lo lejano se ve más claro y azulado; al subir se ve más lejos y la bruma ocupa más | la niebla (tecla `N`). Sin niebla, se pierde la pista principal |
| **Cosas de muchos tamaños** | campos, caminos, ríos y ciudades que se achican al subir | una sola textura de 50 m y el relieve hasta 25,6 km |

**¿Se puede agregar la curvatura?** Sí. No la piden ni el 08 ni el 09: quedó anotada como **mejora para el Práctico 09** en `Practico09-Pendientes.md` (sección 8), con el concepto y por qué va después de la niebla del 09. En resumen:
- **Malla:** en `terreno.vs`, bajar cada vértice según su distancia horizontal `r` al avión: `z -= r² / (2R)`, con `R` = 6371 km. En el borde del clipmap son unos 51 m.
- **Suelo continuado:** en `fondo.fs`, intersectar el rayo con una **esfera** de radio `R` en lugar de con el plano. El horizonte bajaría con la altura y, desde muy alto, se curvaría.

Mientras tanto, la niebla es lo que da la sensación de distancia y de altura. En el Práctico 09 pasa a ser exponencial, con una visibilidad `V`, y el cielo se mezcla con ella en el horizonte. Esa mezcla es la que permite agregar después la curvatura sin que se note el cielo debajo de la altura del ojo.

## 25. ¿Qué hace la "trampa" de la tecla `I` y qué se debería ver?

**Qué hace.** Es el error típico de los mipmaps, provocado a propósito con una segunda textura de pasto (`load_2d(img, Muestreo{}, false)` en `main.cpp`):
1. reserva los 9 niveles con `glTextureStorage2D`;
2. llena solo el nivel 0;
3. **no** llama a `glGenerateTextureMipmap`: los niveles 1 a 8 quedan con contenido **indefinido** (en este driver, ceros = negro);
4. el filtro sigue siendo trilineal, así que la GPU **sí** lee esos niveles.

**Qué se ve.** Cerca del avión el pasto está bien, porque la textura se **amplía** y se usa el nivel 0. Más lejos el suelo se pone **negro**, porque la textura se **reduce** y se leen los niveles vacíos. En el medio se oscurece de a poco (el trilineal mezcla los niveles 0 y 1). OpenGL **no da ningún error**: por eso es una trampa. Captura: `docs-p08/img/06-trampa-mipmaps.jpg`.

**Para qué sirve.** Muestra que los mipmaps son imágenes reales que hay que calcular, y cuándo se usa cada nivel (cerca el 0, lejos los reducidos). Si un objeto se ve negro de lejos y bien de cerca, la causa es esta.

## 26. Con `U`, la imagen de prueba del terreno no está en el mismo sentido que el avión. ¿Está bien?

**Sí.** Las `(u, v)` del terreno salen de la **posición en el mundo** (`u` hacia +x, `v` hacia +y): la imagen está pegada al suelo, como pintada en el campo. Según hacia dónde vuele el avión, la flecha "ARRIBA" queda adelante, de costado o atrás. Si girara con el avión estaría mal: el suelo daría vueltas al doblar.

**Cómo se comprueba:** al hacer un giro, las imágenes quedan quietas en el suelo y el avión gira encima.

**Lo que sí hay que verificar:**
- **Que no esté espejada:** con la flecha apuntando hacia arriba, la "F" queda arriba a la izquierda, el "2" abajo a la derecha, y "ARRIBA" y "(0,0)" se leen normales.
- **Que la grilla de bordes sea continua:** líneas rojas (`u = 0`) y verdes (`v = 0`) sin cortes, también entre los anillos del clipmap.
- **Que cada copia mida 50 m.**

En el avión, que el "2" se vea **estirado** en las alas es lo esperado: cada cara de la caja recibe una imagen completa de (0,0) a (1,1), y el ala es larga y fina.
