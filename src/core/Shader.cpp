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
#include <vector>         // std::vector: las cadenas que se le pasan a glShaderSource (P09)

#include <glad/gl.h>      // funciones de OpenGL (glCreateShader, glProgramUniform*, ...)

namespace {

// Práctico 09: la línea de versión que se antepone a TODAS las etapas. Es la
// versión de GLSL que corresponde al contexto OpenGL 4.6 core que pide main
// (kGLVerMajor/kGLVerMinor). Tiene que ser la PRIMERA línea del texto que se
// compila, por eso ningún archivo la trae y la agrega este módulo.
constexpr const char* kLineaVersion = "#version 460 core\n";

// Compila una sola etapa (vertex/fragment/geometry) y devuelve su ID, o 0 en
// error. En error imprime el log del driver (largo dinamico, como en el
// Práctico 01) y libera el objeto fallido: el error nunca es silencioso (falla 2).
//   etapa:  los archivos de la etapa (rutas + textos, los leyó el ResourceManager).
//   type:   GL_VERTEX_SHADER, GL_FRAGMENT_SHADER o GL_GEOMETRY_SHADER.
//   nombre: "vertex", "fragment" o "geometry" (solo para el mensaje de error).
unsigned int compile_stage(const ShaderStageSource& etapa, unsigned int type,
                           const char* nombre)
{
    // 1. Crear un objeto shader vacío del tipo pedido. Devuelve su
    //    identificador (un número > 0), o 0 si OpenGL no pudo crearlo.
    unsigned int stage = glCreateShader(type);
    if (stage == 0U) {
        std::cout << "[Shader] no se pudo crear un shader (tipo " << type
                  << ")" << std::endl;
        return 0U;
    }

    // 2. Armar el texto con VARIAS cadenas (Práctico 09, Parte 1). La GPU las
    //    compila una a continuación de la otra, como si fueran un solo texto:
    //      cadena 0:  "#version 460 core"                (la agrega Shader)
    //      cadena 1:  "#line 1 1" + primer archivo       (p. ej. iluminacion.glsl)
    //      cadena 2:  "#line 1 2" + segundo archivo      (p. ej. terreno.fs)
    //    #line L N le dice al compilador "la línea que sigue es la L del
    //    archivo número N". Así un error en terreno.fs se informa como
    //    "2:14(...)" = archivo 2, línea 14 de ESE archivo (la misma que muestra
    //    el editor), y no con la línea contada desde el principio del texto
    //    completo. (Sin #line, el driver numera las líneas de corrido.)
    //    Se arma una copia de cada texto con su #line adelante: son unos pocos
    //    KB y solo se hace al compilar.
    std::vector<std::string> cadenas;
    cadenas.reserve(etapa.sources.size() + 1U);
    cadenas.emplace_back(kLineaVersion);
    for (std::size_t i = 0; i < etapa.sources.size(); ++i) {
        cadenas.push_back("#line 1 " + std::to_string(i + 1U) + "\n" + etapa.sources[i]);
    }
    // glShaderSource pide un arreglo de punteros a char (const char*), uno por
    // cadena; nullptr como largos = cada cadena termina en '\0' (como c_str()).
    std::vector<const char*> punteros;
    punteros.reserve(cadenas.size());
    for (const std::string& c : cadenas) {
        punteros.push_back(c.c_str());
    }
    // OpenGL COPIA los textos: `cadenas` se puede liberar después.
    glShaderSource(stage, static_cast<GLsizei>(punteros.size()), punteros.data(), nullptr);
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

        // El log del driver dice el archivo, la línea y el error (por ejemplo
        // "2:42(5): error: `uColr' undeclared" = archivo 2, línea 42). Abajo
        // se aclara qué archivo es cada número (P09).
        std::cout << "[Shader] compilación del " << nombre << " shader fallida:\n"
                  << info_log;
        std::cout << "  (el primer número de cada error es el ARCHIVO: 0 = la línea #version que agrega Shader";
        for (std::size_t i = 0; i < etapa.files.size(); ++i) {
            std::cout << ", " << (i + 1U) << " = " << etapa.files[i];
        }
        std::cout << ")" << std::endl;
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

// Compila las etapas y las enlaza en un programa. Devuelve true si todo
// salió bien; si algo falla, imprime el log, libera lo que creó y deja el
// objeto vacío (id_ = 0).
bool Shader::compile_from_source(const ShaderSource& fuente)
{
    // Si ya habia un programa, se libera primero (compile se puede rehacer).
    clear();

    if (fuente.vs.empty() || fuente.fs.empty()) {
        std::cout << "[Shader] falta el código del vertex o del fragment shader"
                  << std::endl;
        return false;
    }

    // Paso 1: compilar cada etapa (el geometry solo si hay archivos).
    const unsigned int vs_id = compile_stage(fuente.vs, GL_VERTEX_SHADER, "vertex");
    const unsigned int fs_id = compile_stage(fuente.fs, GL_FRAGMENT_SHADER, "fragment");
    const bool con_gs = !fuente.gs.empty();
    const unsigned int gs_id = con_gs ? compile_stage(fuente.gs, GL_GEOMETRY_SHADER, "geometry") : 0U;
    if (vs_id == 0U || fs_id == 0U || (con_gs && gs_id == 0U)) {
        // Liberar las etapas que sí compilaron, para no dejar objetos
        // huérfanos (glDeleteShader(0) no hace nada).
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
        glDeleteShader(gs_id);
        return false;                            // el objeto queda vacío
    }

    // Paso 2: crear el programa (vacío) ...
    id_ = glCreateProgram();
    if (id_ == 0U) {
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
        glDeleteShader(gs_id);
        std::cout << "[Shader] no se pudo crear el programa de shaders"
                  << std::endl;
        return false;
    }

    // ... adjuntarle las etapas y ENLAZAR. El enlace conecta las salidas
    // del vertex shader con las entradas del fragment shader (mismo nombre y
    // tipo) y asigna las ubicaciones de los uniforms.
    glAttachShader(id_, vs_id);
    glAttachShader(id_, fs_id);
    if (con_gs) {
        glAttachShader(id_, gs_id);
    }
    glLinkProgram(id_);

    // Las etapas ya estan dentro del programa: se liberan de inmediato.
    // (glDeleteShader solo las MARCA para borrar: OpenGL las borra de verdad
    // cuando ningún programa las tenga adjuntas. El programa enlazado ya tiene
    // su propio código y no las necesita.)
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    glDeleteShader(gs_id);

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
int Shader::loc(const std::string& nombre, bool avisar_si_falta) const
{
    const int ubicacion = glGetUniformLocation(id_, nombre.c_str());
    // P09: los uniforms de la ESCENA (sol, ojo, niebla) vienen del código común
    // y un programa puede no usar algunos: se buscan con avisar_si_falta =
    // false para no llenar la consola de avisos esperados.
    if (ubicacion == -1 && avisar_si_falta) {
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

// mat3 (P09: la matriz normal de cada pieza). Igual que la mat4: GL_FALSE =
// sin transponer, porque glm guarda las matrices por COLUMNAS, que es lo que
// espera OpenGL.
void Shader::set_uniform(int ubicacion, const glm::mat3& m) const
{
    glProgramUniformMatrix3fv(id_, ubicacion, 1, GL_FALSE, &m[0][0]);
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
