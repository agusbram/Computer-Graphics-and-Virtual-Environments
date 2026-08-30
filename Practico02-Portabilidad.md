# Práctico 02 — Portabilidad: cómo agregar una figura nueva

Análisis de por qué el diseño por módulos permite dibujar cualquier figura nueva
sin reescribir nada más que un generador de datos. El ejemplo concreto ya se hizo
y se verificó: se agregó `primitives::triangle()` y, con **una sola línea
cambiada** en el `main`, el mismo `Mesh`, el mismo `Shader` y el mismo
`glDrawElements` dibujaron el triángulo (la consola decía "3 vértices y 3
índices").

---

## 1. La respuesta corta: ¿es portable?

**Sí.** `Mesh` y `Shader` no conocen la figura: trabajan con cualquier `MeshData`
(una tupla `Vertex` + una lista de índices). La "forma" vive entera dentro del
módulo `primitives`, que no toca OpenGL ni conoce el resto del programa.

Dibujar una figura nueva = **agregar un generador en `primitives`** + **cambiar
una línea en `main`**. Nada más.

## 2. Por qué es portable

La clave es cómo quedó repartida la responsabilidad:

| Módulo | Qué conoce | Qué NO conoce |
|---|---|---|
| `primitives` | la figura (genera `MeshData`) | el GPU, los shaders, la ventana |
| `Mesh` | cómo subir cualquier `MeshData` a la GPU | la figura en particular |
| `Shader` | cómo compilar cualquier par de fuentes | la figura, los datos |
| `main` | qué figura quiere dibujar + el loop | cómo arma el VAO ni el programa |

El **contrato** que une todo es la tupla `Vertex` (posición + color) y los
`layout (location = 0/1)` del vertex shader. Mientras la figura nueva respete esa
tupla, `Mesh::load()`, `Shader` y `glDrawElements` funcionan sin tocarlos:
`Mesh::load()` arma el VAO/VBO/EBO y configura los atributos **con `offsetof`**
(no dependen de la cantidad ni la forma de la figura, solo de la tupla).

La garantía más fuerte es que `Mesh::load()` ya maneja **indexada o no** y
**cualquier cantidad** de vértices/índices: no hay ceros escritos a mano, todo se
calcula del `MeshData`.

## 3. Qué tenés que hacer (receta)

### Paso 1 — Agregar el generador en `src/core/Primitives.h/.cpp`

Una función que devuelva `MeshData`:

- esquinas con la misma tupla `Vertex` (posición + color);
- en **sentido antihorario visto desde afuera** (el criterio fijo del práctico,
  del que depende el futuro descarte de caras traseras);
- índices `(i0, i1, i2)` por triángulo.

```cpp
// Ejemplo hecho: primitives::triangle() -> 3 vértices, 3 índices
MeshData triangle(void)
{
    MeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices.resize(3);

    mesh.vertices[0] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f};  // rojo
    mesh.vertices[1] = { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f};  // verde
    mesh.vertices[2] = { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f};  // azul

    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;

    return mesh;
}
```

### Paso 2 — Cambiar una línea en `main`

```cpp
const MeshData cube_data = primitives::cube();       // antes
const MeshData cube_data = primitives::triangle();   // después: la misma
```

El resto (etapa 2: `Mesh::load`, etapa 3: programa de shaders, etapa 4:
`glDrawElements`) se **reutiliza tal cual**.

> Nota: el mensaje de consola "se esperan 24 y 36" es específico del cubo; con otra
> figura se ajusta solo el texto, no la lógica.

### Primitivas que vienen (roadmap del módulo)

`cylinder(radio, altura, gajos)` y `cone(...)` se agregan con la **misma forma**:
un loop por gajo que genera los vértices de la pared (y las tapas) reutilizando
los vértices entre gajos donde la tupla coincida, más los índices de los
triángulos. No hay que reescribir el módulo porque la forma — *función que
devuelve `MeshData` con parámetros de tamaño* — sigue siendo la misma.

## 4. Los casos en los que SÍ hay que tocar más

El diseño es portable *dentro del contrato*. Si se cambia el contrato, hay más
trabajo:

| Caso | Qué hay que tocar | Qué NO |
|---|---|---|
| **Malla sin índices** (ej. un triángulo suelto) | Nada en la práctica: `Mesh` detecta índices vacíos y `count()` = cantidad de vértices. Solo, si se quiere, el `glDrawArrays` en main | La tupla, el shader, el VAO |
| **Otra tupla de atributos** (ej. hay que agregar normales) | `Vertex` (agregar campo), `Mesh::load()` (un par `AttribFormat` + `EnableVertexArrayAttrib` más) y el `in` del vertex shader | El patrón del módulo: con `offsetof` no se recalculan offsets a mano |
| **Otra visual / más de una figura en pantalla** | Crear más instancias de `Mesh`/`Shader` (cada uno posee los suyos) | Los módulos en sí |

### ¿Por qué "malla sin índices" es casi gratis?

En `Mesh::load()` el EBO se crea **solo si** `data.indices` no está vacío, y
`count()` se define así:

```cpp
if (!data.indices.empty()) {
    // ... crear EBO, bindear a la ranura dedicada
    count_ = data.indices.size();      // glDrawElements usa esto
} else {
    count_ = data.vertices.size();     // glDrawArrays usa esto
}
```

## 5. Cómo se verificó el ejemplo

1. Se agregó `triangle()` al módulo (solo datos, sin OpenGL).
2. Se cambió UNA línea en `main`: `primitives::cube()` → `primitives::triangle()`.
3. `make` sin warnings; la app corrió y la consola confirmó el cambio de figura
   ("3 vértices y 3 índices").
4. Se revirtió el `main` al cubo del práctico (se dejó `triangle()` en el módulo
   como segunda primitiva disponible).

Eso confirma la afirmación de portabilidad: la figura nueva no requirió tocar
`Mesh`, `Shader`, el sistema de build (`LIB_DIRS` ya cubre `src/core`) ni la
lógica de dibujo.