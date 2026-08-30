# Práctico 02 — Del triángulo a la malla: apuntes

Cubo indexado unitario de **24 vértices y 36 índices**, un color plano por cara,
dibujado con módulos. La idea del práctico no son las llamadas de dibujo (son las
mismas del triángulo) sino **encapsular por funcionalidad y por propiedad del
recurso (ownership)**: el `main.cpp` no posee nada de OpenGL.

## Estructura de los módulos

```
primitives::cube()                        Sprimitives (genera datos, sin GL)
  └── MeshData {vertices, indices}        Sparc: CPU
        └── Mesh::load(MeshData)           dueño de VAO/VBO/EBO (sube a GPU)
        ├── ResourceManager.load_shader_source()  lee el disco (strings)
        └── Shader::compile_from_source(vs, fs)   dueño del programa (compila)
              └── main: glDrawElements(...)       única parte de GL que queda
```

## Decisiones por módulo (el detalle completo está en cada header)

### Mesh (vao, vbo, ebo)
- **Dueño único**: copiar está `delete`-d (copiar el entero no copia el objeto en
  la GPU y dos dueños liberarían dos veces). El movimiento traspassa y **deja el
  origen en cero**: así el destructor del movido es un `glDelete*(0)` inofensivo.
- **`count()` = cantidad de índices** (lo que pide `glDrawElements`). Si la malla
  no viene indexada, `count()` cae a la cantidad de vértices y se dibuja con
  `glDrawArrays`.
- **`clear()` repetible sin banderas**: `glDelete*(0)` es legal y no hace nada;
  después de borrar los IDs vuelven a 0.
- **Atributos con `offsetof`** (no `0` y `12` a mano): el día que la tupla `Vertex`
  lleve la normal en el medio no hay que tocar nada.
- **EBO por la ranura dedicada** del VAO (`glVertexArrayElementBuffer`).

### Shader (id_ del programa)
- Igual estrategia que Mesh sobre un recurso distinto (un único entero).
- **No abre archivos**: recibe el fuente ya leído; lee el `ResourceManager`.
- **Log dinámico** (`GL_INFO_LOG_LENGTH`), no `char[512]`.
- **Error por valor de retorno (`bool`) + log impreso y objeto vacío.** Coherente
  pero distinto de la excepción del ResourceManager: el shader roto no corta el
  flujo con una excepción que el main no esperaba; igual **nunca pasa inadvertido**
  (falla 2 de la clínica): si devuelve `false`, main sale sin dibujar.

### primitives (nada que poseer)
- **Función libre, no clase**: este módulo no posee nada (ni estado ni recursos),
  y la forma sigue a la propiedad. `cylinder(radio, altura, gajos)` y `cone(...)`
  se agregan con la misma forma sin reescribir el módulo.
- **Colores fijos dentro del generador** (6 distintos; hoy no se parametrizan).
- **Cubo centrado en el origen** (lado 1 ⇒ −0.5..+0.5); `scale_*` estira por eje
  (permite placas/paralelepípedos).
- **Esquinas en antihorario visto desde afuera**: fija el criterio que usará el
  descarte de caras traseras del teórico. Cada cara = `unitU × unitV = normal
  exterior` con ejes (u,v) por cara.

### main.cpp
- Las 4 etapas del triángulo, en módulos: datos (`cube()`), GPU (`Mesh::load`),
  programa (`ResourceManager` + `Shader::compile_from_source`), dibujo
  (`glDrawElements`).
- **No hay ni un solo `glDelete*`** en `main.cpp`; los destructores liberan.
- `Mesh` y `Shader` viven en un `{}`: sus destructores corren con el contexto
  abierto (antes de `glfwTerminate`).

## Cajas negras de hoy
- `solid.vs`: matriz de transformación **escrita a mano** (copiada de las
  filminas). Sin ella el cubo se ve como un cuadrado. Ojo: GLSL recibe la matriz
  por columnas. Nota: no es una rotación pura (incluye escala; det ≈ −0.16); al
  ser invertible el cubo se ve como paralelepípedo convexo con 3 caras visibles.
- `glEnable(GL_DEPTH_TEST)` una vez antes del loop + `glClear(COLOR | DEPTH)`
  cada cuadro: el test decide qué cara gana cuando varias compiten por el mismo
  píxel.

## Cómo se sabe que está bien
1. Se ven **3 caras de 3 colores distintos**, estables (sin parpadeo).
2. La consola dice **24 y 36** (salida de `primitives::cube()`).
3. **Cero `glDelete*`** en `main.cpp` (`rg "glDelete" src/main.cpp`).
4. Compila **sin warning** (`-Wall -Wextra`).

## Clínica del práctico
Rotura: cambiar `glDrawElements(GL_TRIANGLES, count(), GL_UNSIGNED_INT, nullptr)`
por `glDrawArrays(GL_TRIANGLES, 0, 36)`, sin tocar nada más.

**Predicción (por escrito, antes de correr):** se ignora la lista de índices y se
ensamblan 12 triángulos leyendo el buffer en tríos consecutivos. Como los 24
vértices están agrupados por cara (4 por cara), los tríos cortan las caras: p. ej.
`(0,1,2)` deja afuera la 4ª esquina de la cara 0, y `(3,4,5)` mezcla la 4ª esquina
de la cara 0 con dos de la cara 1 → triángulos que cruzan de un color a otro.
Además se piden 36 vértices y hay 24: las 12 últimas lecturas salen del buffer
(indefinido, típicamente ceros) → triángulos que colapsan hacia el origen. **Sin
ningún error de OpenGL** — es una falla que solo se ve (cubo roto, caras cruzadas,
triángulos basura), igual que en la clínica del triángulo.