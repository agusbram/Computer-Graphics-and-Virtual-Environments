// -----------------------------------------------------------------------------
// Shader.cpp — implementacion
//
// Un PROGRAMA DE SHADERS es el código que corre en la GPU para dibujar. Se arma
// en dos pasos, parecido a compilar C++:
//   1. COMPILAR cada etapa por separado (vertex shader y fragment shader):
//      texto GLSL -> objeto "shader" compilado.
//   2. ENLAZAR (link) las etapas en un PROGRAMA: se verifica que lo que sale
//      del vertex shader (out) coincida con lo que entra al fragment (in).
// El programa es lo que se activa para dibujar (glUseProgram) y al que se le
// pasan los uniforms (matrices, colores, parámetros de la niebla, ...).
//
// Los errores de GLSL NO frenan el programa en C++: OpenGL solo deja un log.
// Por eso acá se pregunta el estado después de cada paso y se imprime el log:
// un shader mal escrito tiene que AVISAR, no dibujar negro en silencio.
// -----------------------------------------------------------------------------

#include "core/Shader.h"  // la declaración de la clase que se implementa acá

#include <cstddef>              // size_t
#include <iostream>             // std::cout, std::endl
#include <string>         // std::string: el código fuente y el log de errores del compilador GLSL

#include <glad/gl.h>      // funciones de OpenGL (glCreateShader, glProgramUniform*, ...)

namespace {

// Compila una sola etapa (vertex/fragment) y devuelve su ID, o 0 en error.
// En error imprime el log del driver (largo dinamico, como en el Práctico 01)
// y libera el objeto fallido: el error nunca es silencioso (falla 2).
//   source: el código GLSL completo, como texto (lo leyó el ResourceManager).
//   type:   GL_VERTEX_SHADER o GL_FRAGMENT_SHADER.
unsigned int compile_stage(const std::string& source, unsigned int type)
{
    // 1. Crear un objeto shader vacío del tipo pedido. Devuelve su
    //    identificador (un número > 0), o 0 si OpenGL no pudo crearlo.
    unsigned int stage = glCreateShader(type);
    if (stage == 0U) {
        std::cout << "[Shader] no se pudo crear un shader (tipo " << type
                  << ")" << std::endl;
        return 0U;
    }

    // 2. Cargarle el código fuente. glShaderSource acepta un ARREGLO de
    //    cadenas (que concatena); acá se pasa una sola: count = 1, la dirección
    //    del puntero al texto, y nullptr como largos (= cada cadena termina en
    //    '\0', como las de c_str()). OpenGL COPIA el texto: `source` se puede
    //    liberar después. (El Práctico 09 va a usar el arreglo para anteponer
    //    el código GLSL común.)
    const char* src_ptr = source.c_str();
    glShaderSource(stage, 1, &src_ptr, nullptr);
    // 3. Compilar. No devuelve nada: el resultado se consulta aparte.
    glCompileShader(stage);

    // 4. ¿Compiló? glGetShaderiv lee un dato del shader; GL_COMPILE_STATUS
    //    vale GL_TRUE o GL_FALSE.
    GLint ok = GL_FALSE;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        // Leer el LOG de errores: primero su largo (GL_INFO_LOG_LENGTH, ya
        // incluye el '\0' final), después se reserva un string de ese tamaño
        // y glGetShaderInfoLog lo llena. Así el log nunca se corta (un buffer
        // fijo de, por ejemplo, 512 caracteres podría quedar chico).
        GLint log_length = 0;
        glGetShaderiv(stage, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(static_cast<size_t>(log_length), '\0');
        glGetShaderInfoLog(stage, log_length, nullptr, info_log.data());

        // El log del driver dice la línea y el error (por ejemplo
        // "0:42(5): error: `uColr' undeclared").
        std::cout << "[Shader] compilación de la etapa " << type
                  << " fallida:\n" << info_log << std::endl;
        glDeleteShader(stage);   // no dejar un objeto inútil en la GPU
        return 0U;
    }
    return stage;
}

}   // namespace (anónimo)

// Destructor: libera el programa de la GPU (dueño único, como Mesh y Texture).
Shader::~Shader()
{
    clear();
}

// Constructor por MOVIMIENTO: se "roba" el identificador del otro objeto y al
// otro se le deja 0. Así hay UN SOLO dueño y el programa se libera una vez
// (si los dos guardaran el mismo id, los dos destructores lo borrarían).
Shader::Shader(Shader&& other) noexcept
    : id_(other.id_)
{
    other.id_ = 0U;
}

// Asignación por MOVIMIENTO: igual que el constructor, pero antes libera el
// programa que este objeto ya tuviera (si no, quedaría perdido en la GPU).
// El if evita el caso raro de moverse a sí mismo (a = std::move(a)), que
// borraría el programa propio.
Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        clear();
        id_ = other.id_;
        other.id_ = 0U;
    }
    return *this;
}

// Compila las dos etapas y las enlaza en un programa. Devuelve true si todo
// salió bien; si algo falla, imprime el log, libera lo que creó y deja el
// objeto vacío (id_ = 0).
bool Shader::compile_from_source(const std::string& vs, const std::string& fs)
{
    // Si ya habia un programa, se libera primero (compile se puede rehacer).
    clear();

    // Paso 1: compilar cada etapa.
    const unsigned int vs_id = compile_stage(vs, GL_VERTEX_SHADER);
    const unsigned int fs_id = compile_stage(fs, GL_FRAGMENT_SHADER);
    if (vs_id == 0U || fs_id == 0U) {
        // Liberar la etapa que sí compiló, para no dejar objetos huérfanos.
        if (vs_id != 0U) glDeleteShader(vs_id);
        if (fs_id != 0U) glDeleteShader(fs_id);
        return false;                            // el objeto queda vacío
    }

    // Paso 2: crear el programa (vacío) ...
    id_ = glCreateProgram();
    if (id_ == 0U) {
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
        std::cout << "[Shader] no se pudo crear el programa de shaders"
                  << std::endl;
        return false;
    }

    // ... adjuntarle las dos etapas y ENLAZAR. El enlace conecta las salidas
    // del vertex shader con las entradas del fragment shader (mismo nombre y
    // tipo) y asigna las ubicaciones de los uniforms.
    glAttachShader(id_, vs_id);
    glAttachShader(id_, fs_id);
    glLinkProgram(id_);

    // Las etapas ya estan dentro del programa: se liberan de inmediato.
    // (glDeleteShader solo las MARCA para borrar: OpenGL las borra de verdad
    // cuando ningún programa las tenga adjuntas. El programa enlazado ya tiene
    // su propio código y no las necesita.)
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);

    // ¿Enlazó? Igual que con la compilación: estado + log. Un error típico de
    // enlace: el fragment shader usa un `in` que el vertex shader no escribe.
    GLint ok = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint log_length = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(static_cast<size_t>(log_length), '\0');
        glGetProgramInfoLog(id_, log_length, nullptr, info_log.data());

        std::cout << "[Shader] enlace del programa fallido:\n"
                  << info_log << std::endl;
        clear();
        return false;
    }
    return true;
}

// Activa el programa: los próximos glDraw* lo usan. Solo puede haber UN
// programa activo a la vez (por eso main llama a use() antes de dibujar cada
// lista: terreno, aeronave; el fondo lo activa Skybox::draw).
void Shader::use(void) const
{
    if (id_ != 0U) {
        glUseProgram(id_);
    }
}

// UBICACIÓN de un uniform: el número que OpenGL le asignó al enlazar. Los
// set_uniform la usan para saber dónde escribir. Buscarla cuesta (compara el
// nombre como texto), por eso main y Skybox la buscan UNA vez al inicio y
// guardan el número.
int Shader::loc(const std::string& nombre) const
{
    const int ubicacion = glGetUniformLocation(id_, nombre.c_str());
    if (ubicacion == -1) {
        // Uniform inexistente o eliminado por el compilador de GLSL por no
        // usarse: avisar ayuda a cazar el nombre mal escrito. Pasar -1 a
        // glProgramUniform* NO es un error (se ignora en silencio).
        std::cerr << "[Shader] aviso: el uniform '" << nombre
                  << "' no existe o fue eliminado por no usarse (ubicacion -1)."
                  << std::endl;
    }
    return ubicacion;
}

// --- set_uniform: escribir el valor de un uniform ---------------------------
// Se usan las funciones glProgramUniform* (DSA): reciben el programa (id_)
// como primer argumento, así que NO hace falta que esté activo con use().
// (Las viejas glUniform* escriben en el programa activo y obligan a activarlo
// antes.) Hay una versión por tipo: el sufijo dice el tipo del uniform en GLSL.
//   Matrix4fv -> mat4      3f -> vec3      2f -> vec2
//   1f        -> float     1i -> int (también los sampler y las banderas 0/1)
// El valor queda guardado en el programa hasta que se cambie: no hace falta
// re-enviar en cada cuadro lo que no cambia.

// mat4 (matrices de modelo, vista y proyección).
//   1        = cuántas matrices (podría ser un arreglo de mat4).
//   GL_FALSE = no transponer: glm ya guarda las matrices por COLUMNAS, el
//              mismo orden que espera OpenGL.
//   &m[0][0] = puntero al primer float; los 16 están seguidos en memoria.
void Shader::set_uniform(int ubicacion, const glm::mat4& m) const
{
    glProgramUniformMatrix4fv(id_, ubicacion, 1, GL_FALSE, &m[0][0]);
}

// vec3 (colores, posiciones, la dirección del sol).
void Shader::set_uniform(int ubicacion, const glm::vec3& v) const
{
    glProgramUniform3f(id_, ubicacion, v.x, v.y, v.z);
}

// float (distancias de la niebla, tamaño de la tesela, altura del ojo).
void Shader::set_uniform(int ubicacion, float valor) const
{
    glProgramUniform1f(id_, ubicacion, valor);
}

// vec2 (posición del ojo módulo el período, escala de la textura).
void Shader::set_uniform(int ubicacion, const glm::vec2& v) const
{
    glProgramUniform2f(id_, ubicacion, v.x, v.y);
}

// int (banderas 0/1 como uUsarTextura, el modo del cielo).
void Shader::set_uniform(int ubicacion, int valor) const
{
    glProgramUniform1i(id_, ubicacion, valor);
}

// Versiones por NOMBRE: buscan la ubicación y llaman a la de arriba. Son
// cómodas pero buscan el nombre en CADA llamada: para lo que se envía en cada
// cuadro conviene guardar la ubicación (loc) y usar la versión con int.
void Shader::set_uniform(const std::string& nombre, const glm::mat4& m) const
{
    set_uniform(loc(nombre), m);
}

void Shader::set_uniform(const std::string& nombre, const glm::vec3& v) const
{
    set_uniform(loc(nombre), v);
}

void Shader::set_uniform(const std::string& nombre, float valor) const
{
    set_uniform(loc(nombre), valor);
}

// Libera el programa de la GPU y deja el objeto vacío (id_ = 0).
void Shader::clear(void)
{
    // glDeleteProgram(0) no hace nada: clear() se puede llamar dos veces.
    glDeleteProgram(id_);
    id_ = 0U;
}
