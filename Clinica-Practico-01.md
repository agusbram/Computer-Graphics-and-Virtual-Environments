# Práctico 01 — La clínica: romper el triángulo a propósito

> Materia: Computación gráfica y ambientes virtuales (0494) — CRUC-IUA
> Basado en las filminas del 21/08/2026 y la experiencia realizada sobre el template (`ogl-app`).

---

## ¿Qué es la clínica?

Es un ejercicio de **debugging metódico**: se rompe el triángulo que ya funciona,
de **tres formas distintas**, siguiendo siempre la misma regla:

> **La regla del bloque:** antes de correr, escribir qué se espera ver.
> *Una predicción equivocada es el único resultado que enseña algo.*

Después de cada rotura se observa qué pasó y **se deja el código andando otra vez**.
El objetivo no es romper por romper: es **verificar que nuestros instrumentos de
diagnóstico funcionan**.

> *"Un chequeo que nunca falló todavía no es un chequeo."*

---

## Conceptos previos necesarios

### 1. Hay DOS compiladores de DOS lenguajes distintos

| Compilador | Lenguaje | Dónde vive | Cuándo corre |
|---|---|---|---|
| `g++` | C++ | En nuestro disco | Al compilar con `make` |
| Compilador **GLSL** | GLSL | Dentro del **driver de video** (Mesa/Intel, NVIDIA, AMD) | **En tiempo de ejecución**, cuando la app llama `glCompileShader` |

Para `g++`, el shader es solo un *string* de C++. Un string con GLSL roto es un
string igualmente válido: **los strings siempre compilan**. Por eso un error de
GLSL **nunca** aparece al compilar el proyecto.

### 2. El camino "de string a programa"

```
glCreateShader   → crea el objeto shader en la GPU
glShaderSource   → le adjunta el código fuente (el string)
glCompileShader  → ACÁ corre el compilador GLSL del driver (en runtime)
glGetShaderiv(GL_COMPILE_STATUS) → hay que PREGUNTAR si compiló
glGetShaderInfoLog               → y LEER el mensaje de error
glAttachShader ×2 + glLinkProgram → se combinan en un programa
glUseProgram     → el pipeline usa ese programa al dibujar
```

OpenGL casi nunca interrumpe el programa por su cuenta: **si no preguntás, no te enterás**.
Los errores son silenciosos salvo que pidas explícitamente el estado y el log.

### 3. Espacio de recorte (clip space)

El vertex shader escribe `gl_Position` en *coordenadas de recorte*. Después de la
división perspectiva, todo lo visible vive en el cubo **NDC**: `x, y, z ∈ [-1; 1]`.
Fuera de ese rango simplemente no hay pantalla.

### 4. El clipping es parte del diseño, no un error

Entre *ensamblado de primitivas* y *rasterizado* hay una etapa **fija de hardware**
que corta las primitivas contra ese cubo. Un objeto que sale de pantalla no genera
ningún aviso: la GPU hace exactamente su trabajo. Por eso este tipo de situación
**no puede reportarse por log**: no falló nada.

### 5. Valores por defecto de los atributos

Si el array de atributos está deshabilitado, cada invocación del vertex shader
recibe para esa variable el valor genérico por defecto: **(0, 0, 0, 1)**.
No hay error: es comportamiento definido por la especificación.

---

## Rotura 1 — Apagar el atributo del VAO

### Qué se cambió

```cpp
// glEnableVertexAttribArray(0); // ← comentada
```

### Qué esperábamos (predicción)

Pantalla sin triángulo y **sin ningún mensaje de error**: si nadie habilita la
lectura del atributo 0, los tres vértices reciben `(0, 0, 0, 1)` y el triángulo
degenera en un punto invisible.

### Qué mostró

- Ventana solo con el color de fondo: **sin triángulo** ✔️
- Consola: **cero mensajes** ✔️
- `make` compiló perfecto (obvio: el código C++ era válido)

### Por qué (conceptos)

1. Sin `glEnableVertexAttribArray(0)`, el atributo queda en su valor por defecto.
2. Los 3 vértices colapsan al mismo punto `(0, 0, 0)`: triángulo de área cero →
   el rasterizador no pinta nada.
3. Es una **falla silenciosa**: nada falló "para OpenGL", el programa hizo lo que
   la especificación dice. No existe log posible que lo detecte.

### Qué NO mostró

- ❌ Ni warnings de `g++` ni logs del driver.
- ❌ La checklist de errores GLSL no sirve acá: **esta verificación es manual**,
  leyendo el código (es uno de los ítems "manuales" de la checklist de pantalla negra).

---

## Rotura 2 — Sacar un punto y coma al vertex shader

### Qué se cambió

Dentro del string GLSL:

```glsl
gl_Position = vec4(aPos, 1.0)   // ← sin ';'
}
```

### Predicción correcta (la que suele confundir)

| Momento | Qué pasa | Por qué |
|---|---|---|
| `make` | ✅ compila, sin warning alguno | Para g++ el shader es un string válido |
| Ejecutar la app | ❌ falla la compilación GLSL **en runtime** | El compilador vive en el driver |
| Consola | ✅ mensaje `Shader compilation failed!` + log | El template ya consulta el estado y el log |

### Qué mostró (salida real en esta máquina)

```
OpenGL Vendor: Intel
OpenGL Renderer: Mesa Intel(R) UHD Graphics (TGL GT2)
...
Shader compilation failed!
0:7(1): error: syntax error, unexpected '}', expecting ',' or ';'
exit code: 1
```

La ventana ni siquiera llega a abrirse: `create_shader_program()` devuelve `0`,
la app limpia GLFW y termina con `EXIT_FAILURE`.

### Por qué (conceptos)

1. El shader viaja como string hasta que **la aplicación corre**; ahí recién el
   driver lo compila.
2. El chequeo con `glGetShaderiv(GL_COMPILE_STATUS)` + `glGetShaderInfoLog`
   **funcionó**: apareció el mensaje con línea (`0:7`) e incluso qué esperaba el
   compilador (`;`). Eso era justo el objetivo: **verificar el instrumento**.
3. Si no hubiera salido ningún mensaje, significaría que nunca se pidió el log —
   y la próxima pantalla negra costaría una tarde.

### Qué NO mostró

- ❌ Nada en tiempo de compilación de C++ (ni error ni warning).
- ❌ La ventana abierta: el fallo ocurre antes del bucle de render.

---

## Rotura 3 — Vértice superior fuera de rango (`0.5 → 1.5`)

### Qué se cambió

```cpp
const float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  1.5f, 0.0f   // ← fuera de NDC [-1; 1]
};
```

### Predicción

El triángulo **no desaparece entero**: sobrevive el pedazo dentro del rango.
No hay mensajes en consola porque **nada falló**.

### Qué mostró

- Un **trapecio**: el triángulo con la punta cortada plana pegada al borde superior.
- Consola: **cero mensajes**. La app corrió indefinidamente, normal.

### Por qué (conceptos)

Geometría del recorte: el vértice superior quedó en `(0, 1.5)`. Las aristas
laterales cruzan el borde `y = 1` en `x ≈ ±0.125`. Lo visible es el cuadrilátero:

```
(-0.5,-0.5) → (0.5,-0.5) → (0.125, 1) → (-0.125, 1)
```

1. La etapa de **clipping** corta la primitiva contra el cubo NDC **por diseño**
   (es lo que pasa en cualquier juego cuando un objeto sale de pantalla).
2. No es error ⇒ no hay log posible ni necesario.
3. Hoy escribimos vértices a mano en `[-1; 1]`; a partir del módulo de matrices
   (04-sep) las transformaciones se encargan de llevar cualquier objeto a ese rango.

### Qué NO mostró

- ❌ Ninguna advertencia de "vértice fuera de rango": no existe tal cosa.
- ❌ Una recta horizontal entre los dos vértices de abajo (lo que queda es el
  **área rellena** que sobrevive al corte, no líneas).

---

## Síntesis: tres pantallas distintas, tres mundos distintos

| # | Rotura | Resultado visual | ¿Aviso en consola? | Naturaleza | ¿Dónde se detecta? |
|---|---|---|---|---|---|
| 1 | Atributo VAO apagado | Pantalla vacía (triángulo degenerado) | ❌ Nada | **Falla silenciosa** | Leyendo el código a mano |
| 2 | `;` faltante en el VS | No llega a abrirse | ✅ Log del driver GLSL | **Error real** | Chequeo de compilación (`GL_COMPILE_STATUS` + log) |
| 3 | Vértice en `y = 1.5` | Trapecio recortado | ❌ Nada (no hay error) | **Comportamiento normal** | Entender el pipeline (clip space) |

### Las lecciones

1. **Dos compiladores:** g++ nunca va a avisar errores de GLSL; el compilador de
   shaders vive en el driver y corre en runtime.
2. **OpenGL es mudo:** hay que pedir explícitamente estados y logs
   (`glGetShaderiv`/`glGetShaderInfoLog`, `glGetProgramiv`/`glGetProgramInfoLog`).
3. **Tres diagnósticos distintos:** tanteando no se diagnostica ninguna. Cada una
   requiere su instrumento: lectura de código, chequeo de logs, o modelo mental
   del pipeline.
4. **Romper a propósito verifica los instrumentos:** si el chequeo no gritó cuando
   sabíamos que había algo roto, el chequeo estaba mal — mejor descubrirlo en la
   clínica que en plena entrega.

### Anexo: checklist de pantalla negra (de las filminas)

Antes de correr: **pensar qué se espera ver.**

1. ¿Compiló el shader? → `glGetShaderiv(GL_COMPILE_STATUS)` + log
2. ¿Linkeó el programa? → `glGetProgramiv(GL_LINK_STATUS)` + log
3. ¿Está activo el VAO al momento de dibujar?
4. ¿Los vértices están en `[-1; 1]`?
5. ¿Winding / culling? (`GL_CULL_FACE` hoy deshabilitado: último sospechoso)

Las últimas tres son **manuales**: no hay API que las reporte.

> Nota de implementación: tras la clínica, el template mejoró el pedido de logs
> usando tamaño dinámico (`GL_INFO_LOG_LENGTH`) en vez de un buffer fijo
> `char[512]`, que trunca justamente los mensajes largos, cuando más se los necesita.
