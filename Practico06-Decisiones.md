# Práctico 06 — FDM, game loop e input: decisiones

Apunte de diseño del **Práctico 06**: cómo se sincronizan el reloj de la
máquina y el del modelo físico (game loop de paso fijo), cómo se amplía
`InputHandler` con teclado y cómo se integra el modelo de dinámica de vuelo
(`dlfdm`). Sigue el formato de los apuntes anteriores.

---

## Objetivo

Que el usuario pueda interactuar con la escena y que la aeronave se mueva según
un modelo físico. Hay que sincronizar dos relojes —el real (cuánto tarda un
cuadro) y el del modelo (paso fijo del FDM)— e incorporar las entradas del
usuario.

---

## Integración de la librería `dlfdm`

- Se copió a **`libs/dlfdm`** y se integra como `glad`: el módulo declara sus
  fuentes e includes en `libs/dlfdm/dlfdm.mk`, que se incluye desde el
  `Makefile` del proyecto:

  ```make
  DLFDM_DIR = ./libs/dlfdm
  include $(DLFDM_DIR)/dlfdm.mk
  INC_DIRS = $(GLAD_INC_DIRS) $(DLFDM_INC_DIRS) ./src
  LIB_DIRS = $(GLAD_LIB_DIRS) $(DLFDM_LIB_DIRS) ./src/core
  ```

- Se agregó **`-lm`** al linkeo (dlfdm usa funciones matemáticas).
- El ejecutable de prueba de dlfdm (`dlfdm-test`) **no abre ventana**: es una
  simulación off-line que vuelca la traza a `salida.csv`. La ventana aparece
  recién al integrarlo al proyecto gráfico (este práctico).

---

## Parte 1 — Game loop con paso fijo + acumulador

Dos relojes que hay que sincronizar:

- **Tiempo real** (`frame_dt`): cuánto tarda en generarse cada cuadro. Depende
  de la máquina.
- **Tiempo del modelo** (`dt = 1/120 s`): paso **fijo** del FDM. Debe ser
  pequeño y constante (el error de Euler se acumula; con pasos grandes los
  modos rápidos divergen) y no depende de la máquina (para que la trayectoria
  sea reproducible).

Estrategia (acumulador):

```cpp
acc += std::min(frame_dt, 0.25);   // el tope corta la divergencia
while (acc >= dt) {
    fdm.update(input.controls());  // avanza SIEMPRE dt, nunca frame_dt
    acc -= dt;
}
// una vez por cuadro: to_world() -> escena -> cámara -> render
```

- El **resto** que queda en `acc` no se descarta: es tiempo que no alcanzó para
  un paso y se suma al próximo cuadro. Invariante: `simulado + acc = reloj`.
- El **tope de 0.25 s** evita la espiral: si un cuadro tarda mucho, el próximo
  tendría que hacer cada vez más pasos, realimentándose hasta congelarse. El
  tope deja de simular en tiempo real a propósito antes que clavarse.

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| Primer cuadro: `acc = 0` | Corre **0 pasos** (0 < dt): solo dibuja el estado inicial (el trim). Está bien: no hay tiempo acumulado todavía. |
| Un cuadro muy largo sin el tope | `acc` crece, el cuadro siguiente hace más pasos, tarda más, `acc` crece más… **diverge**. El tope de 0.25 lo corta. |
| ¿Por qué la lectura de entrada va FUERA del while interno? | Es un control **continuo por cuadro** (cámara y teclas), no por paso de simulación. Además el FDM puede correr 0, 1 o muchos pasos por cuadro y la entrada no cambia dentro de un mismo cuadro. |

---

## Parte 2 — Teclado en InputHandler

`InputHandler` ahora produce **dos cosas**: el `CameraCommand` (mouse, P05) y
las posiciones de los cuatro comandos de la aeronave (`dlfdm::ControlInputs`).

### Estado o evento

- Los **cuatro mandos** son control **continuo** → **polling** una vez por
  cuadro (`glfwGetKey`).
- **Pausa** y **wireframe** son comandos **discretos** → **callback**
  (`glfwSetKeyCallback`), que GLFW ya entrega como flanco (distingue
  `GLFW_PRESS` de `GLFW_REPEAT`; se filtra solo `PRESS` para que un toggle
  cambie una vez por pulsación).

### Rampa de velocidad constante + centrado automático

La tecla **no fija el valor** del comando sino su **velocidad** de cambio, que
es **constante** (rampa de primer orden, lo que pide la guía); al soltar, las
superficies **vuelven solas** al neutro:

```cpp
if (dir != 0) {                              // tecla apretada
    valor += dir * rate * dt;                // cambia a velocidad fija
} else if (centra) {                         // tecla suelta
    valor += (neutro - valor) * kCentrado * dt;   // retorno de resorte
}
valor = std::clamp(valor, min, max);
```

- **Mantener** la tecla → el comando sigue cambiando a velocidad constante.
- **Soltar** → el comando **vuelve al neutro de a poco** (retorno de
  resorte/exponencial: rápido al principio, se frena al acercarse). Es el
  comportamiento real de un bastón (la guía lo menciona como el natural).
- **A qué vuelve cada uno**:
  - **elevador → valor de trim** (`−0.0937 rad`),
  - **alerones y timón → 0**,
  - **potencia (throttle) → NO centra**: se queda donde se dejó (se reduce a
    mano, como un acelerador real).
- Ritmos: `rate = 0.25 rad/s` al mantener (poco sensible), `kCentrado = 5 1/s`
  para el resorte de retorno. Fáciles de ajustar.
- Decisión registrada: la guía deja el **centrado manual** como la alternativa
  simple; acá se eligió el **automático** por ser más realista (se documenta
  como decisión propia, como pide la consigna).

### Convención de signos (elegida y registrada)

La tecla representa el **bastón** (como en un avión real). De la tabla de la
guía, y verificado contra el FDM:

| Tecla | Mando | Signo | Efecto (verificado) |
|---|---|---|---|
| ↑ / ↓ | elevador | ↑ → δe **+** | nariz **abajo** (↓ → nariz arriba) |
| → / ← | alerones | → → δa **+** | ala derecha **abajo** |
| E / Q | timón | E → δr **−** | nariz a la **derecha** |
| W / S | potencia | W ↑ | más empuje (throttle ∈ [0,1]) |

### La condición inicial (set_controls)

El FDM arranca en un **trim** que incluye una deflexión de elevador distinta de
cero (`-0.0937 rad`). Si `InputHandler` arrancara en cero, en el primer cuadro
enviaría al FDM un comando que no es el del equilibrio y **el avión se
descompensaría solo**. Por eso `main` llama `input.set_controls(trim.controls)`
**antes del loop**. Estado y comandos de trim son un solo dato repartido en dos
lugares.

---

## Parte 3 — El FDM en el loop y `to_world()`

- El **solver** se queda con una copia de los parámetros
  (`dlfdm::jettrainer::load_model()`), el paso se fija en su construcción
  (`1/120 s`), y la condición inicial (trim) se carga con `setState()`.
- `fdm.update(controls)` va **adentro** del while de paso fijo;
  `getState()` se lee **una vez por cuadro** (para dibujar).
- `to_world()` lleva la salida NED a coordenadas de la escena (única, para no
  repetir la conversión con errores).

### Mapeo de sistemas de referencia (la parte delicada)

El FDM entrega **posición y actitud en NED** (North, East, Down). La escena es
**Z arriba** y el modelo del avión se construyó (P04) con **nariz −X, ala
derecha +Y, arriba +Z**. Se eligió el mapeo de posición para que en trim
(rumbo norte, ψ=0) la nariz (−X) apunte al norte y el ala derecha (+Y) al este:

```
x_escena = −norte      y_escena = este       z_escena = −abajo (altitud)
```

Se niegan **dos** de los tres ejes (negar uno solo daría una reflexión y el
mundo saldría espejado).

La **actitud** se compone en la escena con los ejes del modelo (`Aircraft::
update`):

```
pose = T(pos) · Rz(−ψ) · Ry(θ) · R_{eje nariz}(φ)
```

Verificado **por cálculo** (antes de codear) que con esto:

| Ángulo | Efecto comprobado |
|---|---|
| θ (cabeceo) > 0 | la nariz **sube** |
| φ (alabeo) > 0 | el ala derecha **baja** |
| ψ (guiñada) > 0 | la nariz gira a la **derecha** (norte → este) |

### Escala

El FDM trabaja en SI (metros, m/s) y el avión está a ~5000 m; el modelo del
avión mide 1 unidad. Como la cámara **orbita al avión** (su objetivo sigue su
posición), el tamaño en pantalla no depende de la magnitud de la posición. El
ajuste fino de escala/mundo llegará con el terreno (próximo tema). Por eso no
se escaló nada.

---

## Cómo se sabe que está bien

1. **La escena se mueve igual en máquinas distintas** (y con vsync on/off):
   el avance del FDM usa `dt` fijo, no `frame_dt`.
2. **Sin tocar teclas, el avión mantiene la altura y la velocidad**
   (trim): verificado con la lib — tras 5 s sin input, `theta≈0`, alt ≈ 4993 m,
   TAS ≈ 150.5 m/s (oscilación fugoide residual).
3. **Una deflexión sostenida gira en el eje y sentido elegidos**: verificado
   con la lib — elevador −0.25 → `theta`+ (nariz arriba); elevador +0.10 →
   `theta`−; alerón +0.15 → `phi`+; timón −0.15 → `psi`+.
4. **Al arrastrar la ventana no se congela**: el tope de 0.25 s del acumulador.
5. **El comando discreto (ESPACIO / F) cambia una vez por pulsación**: el
   callback filtra `GLFW_PRESS`.

## Controles (resumen)

- `ESC` cierra; `ESPACIO` pausa/reanuda; `F` alterna relleno/wireframe.
- Flechas = cabeceo/alerones; `Q`/`E` = timón; `W`/`S` = potencia.
- Mouse: botón izquierdo = orbitar la cámara; derecho = zoom.

---

## Arquitectura

Se materializan módulos del `Arquitectura-Proyecto.md`:

- **FDM** (capa de sistemas) — la librería `dlfdm`.
- **InputHandler** (capa de sistemas) — ampliado con teclado.
- **FlightData** (estructura de datos compartida) — salida del FDM hacia la
  escena.
- El **game loop** sigue en `main` (Application), que orquesta. El "Renderer"
  sigue dentro del `main` (se extraerá cuando haya más escena que dibujar).
