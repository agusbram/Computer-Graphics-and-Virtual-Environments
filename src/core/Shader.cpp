// -----------------------------------------------------------------------------
// Shader.cpp — implementacion
// -----------------------------------------------------------------------------

#include "core/Shader.h"

#include <cstddef>              // size_t
#include <iostream>             // std::cout, std::endl
#include <string>

#include <glad/gl.h>

namespace {

// Compila una sola etapa (vertex/fragment) y devuelve su ID, o 0 en error.
// En error imprime el log del driver (largo dinamico, como en el Práctico 01)
// y libera el objeto fallido: el error nunca es silencioso (falla 2).
unsigned int compile_stage(const std::string& source, unsigned int type)
{
    unsigned int stage = glCreateShader(type);
    if (stage == 0U) {
        std::cout << "[Shader] no se pudo crear un shader (tipo " << type
                  << ")" << std::endl;
        return 0U;
    }

    const char* src_ptr = source.c_str();
    glShaderSource(stage, 1, &src_ptr, nullptr);
    glCompileShader(stage);

    GLint ok = GL_FALSE;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint log_length = 0;
        glGetShaderiv(stage, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(static_cast<size_t>(log_length), '\0');
        glGetShaderInfoLog(stage, log_length, nullptr, info_log.data());

        std::cout << "[Shader] compilación de la etapa " << type
                  << " fallida:\n" << info_log << std::endl;
        glDeleteShader(stage);
        return 0U;
    }
    return stage;
}

}   // namespace (anónimo)

Shader::~Shader()
{
    clear();
}

Shader::Shader(Shader&& other) noexcept
    : id_(other.id_)
{
    other.id_ = 0U;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        clear();
        id_ = other.id_;
        other.id_ = 0U;
    }
    return *this;
}

bool Shader::compile_from_source(const std::string& vs, const std::string& fs)
{
    // Si ya habia un programa, se libera primero (compile se puede rehacer).
    clear();

    const unsigned int vs_id = compile_stage(vs, GL_VERTEX_SHADER);
    const unsigned int fs_id = compile_stage(fs, GL_FRAGMENT_SHADER);
    if (vs_id == 0U || fs_id == 0U) {
        // Liberar la etapa que sí compiló, para no dejar objetos huérfanos.
        if (vs_id != 0U) glDeleteShader(vs_id);
        if (fs_id != 0U) glDeleteShader(fs_id);
        return false;                            // el objeto queda vacío
    }

    id_ = glCreateProgram();
    if (id_ == 0U) {
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
        std::cout << "[Shader] no se pudo crear el programa de shaders"
                  << std::endl;
        return false;
    }

    glAttachShader(id_, vs_id);
    glAttachShader(id_, fs_id);
    glLinkProgram(id_);

    // Las etapas ya estan dentro del programa: se liberan de inmediato.
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);

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

void Shader::use(void) const
{
    if (id_ != 0U) {
        glUseProgram(id_);
    }
}

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

void Shader::set_uniform(int ubicacion, const glm::mat4& m) const
{
    glProgramUniformMatrix4fv(id_, ubicacion, 1, GL_FALSE, &m[0][0]);
}

void Shader::set_uniform(int ubicacion, const glm::vec3& v) const
{
    glProgramUniform3f(id_, ubicacion, v.x, v.y, v.z);
}

void Shader::set_uniform(int ubicacion, float valor) const
{
    glProgramUniform1f(id_, ubicacion, valor);
}

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

void Shader::clear(void)
{
    // glDeleteProgram(0) no hace nada: clear() se puede llamar dos veces.
    glDeleteProgram(id_);
    id_ = 0U;
}