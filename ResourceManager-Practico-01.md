# Práctico 01 — ResourceManager: diseño y funcionamiento

> Materia: Computación gráfica y ambientes virtuales (0494) — CRUC-IUA
> Complemento de `Clinica-Practico-01.md`. Documenta el módulo de la Parte 2
> del práctico y las decisiones tomadas sobre la interfaz propuesta por el enunciado.

---

## ¿Qué problema resuelve?

En el programa básico los dos shaders viven adentro del `.cpp` como strings.
Para escalar hay que procesarlos como **archivos externos** (`*.vs`, `*.fs`), y
para eso hace falta un módulo que:

1. Lea del disco el fuente de un shader y lo deje disponible como texto.
2. Si el mismo recurso se pide dos veces, **no vuelva a leer del disco** (caché).
3. Si algo falla — archivo faltante, carpeta faltante, clave no cargada —
   **genere un aviso**: no se rompe en silencio.
4. Se pueda ampliar después para texturas, modelos de mallas, etc.

Restricción del enunciado: **C++ puro**, sin llamadas a OpenGL, solo biblioteca estándar.

---

## Interfaz (la propuesta por el enunciado, respetada)

```cpp
struct ShaderSource {           // codigo fuente de un programa de shader
    std::string vs;             // vertex shader
    std::string fs;             // fragment shader
    std::string gs;             // geometry shader (opcional, hoy no se usa)
};

class ResourceManager {
public:
    explicit ResourceManager(const std::filesystem::path& assets_root);

    const ShaderSource& load_shader_source(const std::string& key,
                                           const std::string& vs_file,
                                           const std::string& fs_file,
                                           const std::string& gs_file = "");
    const ShaderSource& get_shader_source(const std::string& key) const;
    void clear(void);
};
```

La **forma** es lo importante (enunciado): *un struct por tipo de recurso +
un `load_*` + un `get_*` + un mapa*. Ese patrón se repite al agregar texturas
o modelos más adelante.

---

## Decisiones de diseño (y su justificación)

| Pregunta del enunciado | Decisión | Por qué |
|---|---|---|
| ¿Cómo aviso fallos? | **Excepciones** `std::runtime_error` | Un fallo nunca pasa desapercibido; el llamador no puede ignorar el chequeo. Es C++ estándar puro |
| Clave repetida con archivos distintos | **Warning por stderr + gana el caché** | La primera carga es la fuente de verdad; se informa, no se rompe en silencio |
| ¿Referencia const o copia? | **`const ShaderSource&`** | Evita copiar strings grandes. ATENCIÓN: queda inválida tras `clear()` o si muere el manager |
| ¿Cache por clave o nombre de archivo? | **Por clave del usuario** | Permite referirse al recurso en forma semántica (`"triangle"`) aunque se renombren archivos |
| ¿Struct o fuentes sueltos? | **Struct** `ShaderSource` | Agrupa el programa completo (vs+fs+gs); escala a otros tipos de recursos |

---

## Funcionamiento interno

### El caché dentro de `load_shader_source`

El orden de las operaciones es la clave del diseño:

```cpp
const auto it = shaders_sources_.find(key);        // 1° busca en el MAPA (memoria)
if (it != shaders_sources_.end()) {                // ¿ya estaba la clave?
    if (mismos_archivos) {
        return cached.source;   // ← sale ACÁ, antes de tocar cualquier archivo
    }
    // archivos distintos → warning por stderr + devuelve el cache igual
}
// recién si NO estaba en el mapa se llega acá (único camino con disco):
entry.source.vs = read_shader_file(vs_file, ...);
```

- **Primera vez:** no está en el mapa → lee los archivos → guarda copia en RAM
  (`unordered_map<std::string, CachedShader>`) → devuelve referencia a esa copia.
- **Segunda vez (misma clave y archivos):** `find` la encuentra **antes** de llegar
  a cualquier línea de lectura → devuelve directamente la copia en RAM. El disco
  ni se entera. Esa es la definición de caché: *el trabajo caro (I/O) se hace una vez*.
- La entrada interna `CachedShader` recuerda además **qué archivos** originaron la
  carga: es lo que permite avisar cuando una clave repetida llega con otros archivos.

### `load_` vs `get_`: contratos distintos

| | `load_shader_source(...)` | `get_shader_source(key)` |
|---|---|---|
| Necesita | clave **+ nombres de archivos** | solo la **clave** |
| ¿Toca disco? | Sí, solo si es la primera vez | **Nunca** |
| Si no existe | lee y cachea | lanza excepción |

Escenario típico de uso combinado: `main()` carga todo al arranque (única lectura),
y más tarde otros módulos (una futura clase `Mesh`, el HUD...) recuperan lo ya
cargado sin conocer rutas ni re-leer nada:

```cpp
// main(), al inicio — ÚNICA lectura de disco:
ResourceManager resources("./assets");
resources.load_shader_source("cuadrado", "shaders/cuadrado.vs", "shaders/cuadrado.fs");

// después, cualquier módulo con acceso a ESA MISMA instancia:
const ShaderSource& s = resources.get_shader_source("cuadrado");  // directo de RAM
```

Condiciones: debe ser **la misma instancia** del manager (por eso se crea una sola
vez en `main`), y la clave debe haber sido cargada antes — si no, lanza
`"se pidió la clave X pero nunca fue cargada"`.

---

## Cómo agregar un shader nuevo (receta)

### 1. Crear los archivos en `assets/shaders/`

Ej.: `cuadrado.vs` y `cuadrado.fs` con el GLSL nuevo. No hay que tocar nada del
módulo: son solo archivos de texto.

### 2. Cargarlos con una clave distinta

```cpp
const ShaderSource& cuad_src = resources.load_shader_source(
    "cuadrado", "shaders/cuadrado.vs", "shaders/cuadrado.fs");

unsigned int prog_cuadrado = create_shader_program(cuad_src);
```

Cada clave distinta → entrada separada en el mapa → cada fuente compila su propio
programa con `glCreateShader/glCompileShader/glLinkProgram`.

### 3. Usar ese programa al dibujar ese objeto

```cpp
glUseProgram(prog_cuadrado);
glBindVertexArray(vao_cuadrado);
glDrawArrays(...);
```

Nota: para dos objetos hacen falta también **dos VAO/VBO** (uno por geometría) —
tema de la clase del 28-ago (`Mesh`, `Shader`).

¿Geometry shader? La interfaz ya lo contempla: pasar el cuarto parámetro
(`load_shader_source("x", "x.vs", "x.fs", "x.gs")`) hace que también se lea.

---

## Pruebas

### Test automatizado (`tests/main_test_rm.cpp`)

Sale de la recomendación 1 del enunciado: *"crear un main.cpp de prueba que
demuestre que el cacheo funciona"*. Cada bloque demuestra un requisito:

| Sección | Qué demuestra |
|---|---|
| 1 | Lee los `.vs`/`.fs` reales desde `./assets/shaders/` |
| 2 | **Cacheo:** pide la clave 2 veces y compara direcciones (`&a == &b`) → misma copia en memoria → no releyó |
| 3 | Clave repetida con archivos distintos → warning por stderr + gana el caché |
| 4 | Archivo inexistente → excepción (nunca silencio) |
| 5 | `get` de clave nunca cargada → excepción |
| 6 | `clear()` → las claves ya no resuelven |

Compilar y correr (desde la raíz del proyecto):

```bash
g++ -std=c++17 -Wall -Wextra -I./src src/core/ResourceManager.cpp tests/main_test_rm.cpp -o build/test_rm
./build/test_rm
```

Debe compilar **sin warnings** y todas las pruebas pasan.

### Experimentos manuales con la app

1. **Ver al manager fallando con aviso claro** (lo contrario de la rotura 2
   silenciosa de la clínica):

   ```bash
   mv assets/shaders/triangle.fs assets/shaders/triangle.fs.bak
   ./bin/ogl-app     # "Error cargando shaders: ... no existe el archivo ..."
   mv assets/shaders/triangle.fs.bak assets/shaders/triangle.fs
   ```

2. **Editar un shader sin recompilar:** cambiar el color naranja en
   `assets/shaders/triangle.fs` y correr `./bin/ogl-app` directamente. Esa es la
   gracia de tener los shaders fuera del `.cpp`.

---

## Integración actual en el proyecto

- `src/core/ResourceManager.h/.cpp`: el módulo (C++ puro, decisiones documentadas).
- `assets/shaders/triangle.vs/.fs`: los shaders que antes eran strings.
- `Makefile`: `./src/core` agregado a `LIB_DIRS` (compila automático).
- `main.cpp`: crea `ResourceManager(kAssetsRoot)` y carga `"triangle"` adentro
  de un `try/catch`; los logs GLSL usan tamaño dinámico (`GL_INFO_LOG_LENGTH`)
  en vez del buffer fijo `char[512]` que trunca mensajes largos.
