# Práctico 05 — Cámara en la escena: decisiones

Apunte de diseño del **Práctico 05**: cómo se reemplaza la "caja negra"
`uAjuste` por matrices de vista y proyección, y cómo se arma la cámara orbital
controlada con el mouse. Sigue el formato de los apuntes anteriores.

---

## Objetivo

Poner una cámara en la escena: reemplazar la constante `uAjuste` del vertex
shader por una **matriz de vista** y una **matriz de proyección** calculadas a
partir de la visualización elegida (cámara orbital + proyección en
perspectiva). La cámara se controla con el mouse.

---

## Parte 1 — El módulo CameraSystem

| Pieza | Contenido |
|---|---|
| `CameraData.h` | `{glm::mat4 view, projection}` — lo único que sale del módulo |
| `CameraCommand.h` | `{yaw_delta, pitch_delta, dist_delta}` — deltas del usuario |
| `CameraSystem.h/.cpp` | `update()` (vista) + `set_viewport()` (proyección) + `data()` |
| `InputHandler.h/.cpp` | mouse por polling → `CameraCommand` |

**Qué NO hace cada uno** (siguiendo la guía):

- `CameraSystem` no lee el mouse (recibe deltas ya interpretados), no dibuja
  (no toca OpenGL, solo glm), y no sabe qué hay en la escena (recibe un punto
  al que mirar).
- `InputHandler` no mueve la cámara y no conoce a `CameraSystem`: traduce
  píxeles a radianes y entrega un `CameraCommand`.

---

## Parte 2 — La proyección (`set_viewport()`)

- Se usa **`glm::perspective(fovy, aspect, near, far)`** con `fovy = 45°`
  (en radianes), `near = 0.1`, `far = 100`. `fovy` va **en radianes**: pasar
  grados no da error, da pantalla negra.
- `near`/`far` se eligen siguiendo la recomendación de la clase (near "tan
  lejos como se pueda sin eliminar lo que hace falta ver"; su elección
  impacta en la precisión del z-buffer, Unidad VIII).
- **La proyección se recalcula al redimensionar**, porque el `aspect` depende
  del framebuffer. Es un *evento* → callback
  (`glfwSetFramebufferSizeCallback`).

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| ¿Qué pasa si `height` llega en 0 (minimizar)? | `aspect = width/height` explotaría. La guarda está en `set_viewport()`: si `height == 0` se usa `1`. |
| El callback es función, no método: ¿cómo llega al objeto? | Con `glfwSetWindowUserPointer(window, &camara)`; el callback hace `glfwGetWindowUserPointer` y llama a `set_viewport`. |

---

## Parte 3 — La órbita y la vista (`update()`)

La cámara orbital se describe con tres números alrededor de un punto objetivo:
**yaw** (acimut), **pitch** (latitud) y **distancia**. El usuario pide *cambios*
(deltas), no valores absolutos; acumular y acotar es responsabilidad de la
cámara.

De esféricas a cartesianas sale la posición de la cámara y de ahí la vista:

```
eye = objetivo + d · (cos p·cos y,  cos p·sin y,  sin p)
view = glm::lookAt(eye, objetivo, up = (0, 0, 1))
```

**Decisión — el eje polar de la órbita es Z, no Y.** La fórmula genérica de la
guía usa Y como eje polar (`y = d·sin p`) porque sus ejemplos tienen Y "hacia
arriba". En este proyecto la aeronave se modeló con **Z arriba** (X+ cola, Y+
ala derecha, Z+ arriba, Práctico 04), así que se intercambian Y y Z: `pitch`
eleva en Z y `yaw` rota en el plano X-Y. Coherente con `up = (0,0,1)`; si se
usara `up = (0,1,0)` el avión quedaría "de costado".

**Acotar el estado** (para no producir matrices indefinidas ni perder el
modelo):

- `pitch ∈ [−81°, +81°]` (= 0.9·π/2): en ±90° el `up` y la dirección de vista
  son colineales y el producto vectorial de adentro de `lookAt` se anula.
- `distancia ∈ [2, 40]`: para no atravesar el modelo ni perderlo de vista.

### Cuestiones a pensar (respondidas)

| Pregunta | Respuesta |
|---|---|
| Primer cuadro sin "posición anterior" | El `InputHandler` usa una bandera `primer_cuadro_`: en el primer cuadro el delta vale 0 y solo se guarda la posición para el siguiente. |
| `update()` recibe `angulos_avion` y no se usa | Queda reservado para una futura **vista de cabina**: ahí el sistema de referencia de la cámara estaría atado a la orientación del avión. La cámara orbital solo necesita el punto a mirar. |
| ¿Qué se ve si se invierte el orden `P·V·M`? | Un orden distinto transforma mal la escena (p. ej. `V·P·M` aplica la proyección antes de estar en el espacio de la cámara y el resultado no tiene sentido). El compilador **no avisa**: son matrices, cualquier producto es "legal". Es un error silencioso de lógica, no de compilación. |
| "La escena gira a distinta velocidad en cada máquina, ¿por qué?" | Es la pista de la filmina sobre el tiempo. En esta implementación los deltas del mouse son **por píxel**: arrastrar acumula la misma cantidad de píxeles a cualquier FPS, así que la velocidad no depende de la máquina. El parámetro `dt` queda reservado para controles dependientes del tiempo (p. ej. una órbita automática). |

---

## El mouse (InputHandler)

- **Polling**, no callback: mover la cámara es un control *continuo*; en cada
  cuadro se consulta el estado (`glfwGetCursorPos` + `glfwGetMouseButton`).
- **Mapeo** (dos de tres controles al mouse, el tercero con un botón):
  - botón **izquierdo** arrastrado → `yaw` (eje X) y `pitch` (eje Y);
  - botón **derecho** arrastrado (vertical) → `distancia`.
- Deltas: `yaw_delta = −dx·g`, `pitch_delta = +dy·g`, `dist_delta = +dy·gd`.
  Convención **"orbitar alrededor"**: la cámara se mueve con el mouse y el
  avión gira en sentido contrario (arrastrar → nariz a la izquierda; arrastrar
  ↑ nariz baja). Arrastrar hacia abajo aleja.
- Ganancias: `0.008` rad/px (angular) y `0.03` unidades/px (distancia).

---

## El shader

`solid.vs` pasa de `uAjuste` a tres uniforms, en el orden **P · V · M**:

```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
...
gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
```

El vértice cambia de sistema de referencia cinco veces antes de ser píxel:
objeto (`uModel`) → mundo → cámara (`uView`) → recorte (`uProjection`) → NDC
(÷w) → pantalla (viewport). La división por w y el viewport son etapa fija
(no se programan).

---

## Cómo se sabe que está bien

1. **Hay perspectiva**: `uAjuste` era un escalado con cuarta fila `(0,0,0,1)`
   (w = 1, sin división); `glm::perspective` tiene cuarta fila `(0,0,−1,0)`
   (w = −z). Verificado por cálculo: un punto en `z = −5` sale con `w = 5` y
   uno en `z = −20` con `w = 20` → al dividir, el lejano queda más chico.
2. **Al arrastrar, la escena queda quieta y se mueve el punto de vista** (el
   objeto no se deforma ni cambia de tamaño): el único que cambia es `uView`.
3. **Al redimensionar no se deforma**: el callback recalcula `uProjection`
   con el nuevo aspect (y `glViewport` actualiza el rectángulo de píxeles).
4. **Pitch al tope no salta ni se pone negro**: el clamp a ±81° impide que
   `up` y la vista sean colineales.

---

## Arquitectura

Con este práctico se materializan **dos módulos del `Arquitectura-Proyecto.md`**:
`CameraSystem` (capa de sistemas) e `InputHandler` (capa de sistemas), más las
estructuras de datos `CameraData` y `CameraCommand`. El "Renderer" sigue siendo
el loop de dibujado dentro de `main` (se extraerá cuando haya más de una
cámara/vista). Ver `Arquitectura-Proyecto.md`.
