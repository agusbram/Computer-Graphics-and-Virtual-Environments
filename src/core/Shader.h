// -----------------------------------------------------------------------------
// Shader.h
//
// Administra la vida de un programa de shaders en la GPU: recibe el codigo
// fuente ya leido, compila el vertex y el fragment, los linkea en un
// programa y lo deja listo para usar. Las MISMAS tres decisiones que Mesh
// (dueno unico: copiar borrado, mover traspasa y deja el movido en cero).
//
// Esta clase NO abre archivos: quien lee el disco es el ResourceManager.
// Separacion de responsabilidades: el ResourceManager lee, Shader compila.
//
// Decisiones de diseño (Practico 02, Parte 2):
//   1. ERROR SEMICIANCO: valor de retorno bool + log del driver impreso.
//      Es coherente pero deliberadamente distinto a la excepcion del
//      ResourceManager: el error de un shader no corta todo el programa con
//      una excepcion que main no espera; el llamador decide y el objeto
//      queda VACIO (id() == 0). Nunca en silencio: si algo falla se imprime
//      el log de todos modos.
//   2. LOG DE TAMAÑO DINAMICO: se consulta GL_INFO_LOG_LENGTH (como en el
//      Practico 01); un char[512] fijo truncaria los mensajes largos, que
//      son los que mas se necesitan.
//   3. FALLA 2 DE LA CLINICA: un shader roto NO puede pasar desapercibido.
//      Si compile_from_source devuelve false, main sale sin dibujar en vez
//      de seguir con un programa vacio.
//
// Ampliacion Practico 03 (Parte 3) — los uniform:
//   4. UN METODO POR TIPO DE UNIFORM (mat4, vec3, float). No es una lista
//      cerrada: cada tipo nuevo que haga falta agrega una sobrecarga.
//   5. ACCESO DIRECTO AL ESTADO (DSA): se usan glProgramUniform*, que
//      reciben el programa como argumento y por lo tanto NO exigen haberlo
//      activado antes con use(). (Los tutoriales usan glUniform*, que actua
//      sobre el programa ACTIVO: llamarlo sin activarlo cambia el uniform
//      de otro programa o falla en silencio, y es dificil de encontrar.)
//   6. DOS SABORES: por NOMBRE (legible, busca el string cada vez) y por
//      UBICACION cacheada (se consulta glGetUniformLocation UNA vez con
//      loc() y se setea con el entero, ideal dentro del loop de dibujado).
//   7. UBICACION -1 AVISADA. Si el uniform no existe o el compilador de GLSL
//      lo elimino por no usarse, glGetUniformLocation devuelve -1 y loc()
//      avisa por consola. Pasar -1 a glProgramUniform* no es error: la
//      llamada se ignora en silencio.
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader();                                  // libera el programa

    Shader(const Shader&) = delete;             // las MISMAS tres decisiones
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Compila las dos etapas, las linkea y libera los objetos intermedios.
    // true en exito. Si algo falla se imprime el log del driver y el objeto
    // queda vacio (id() == 0).
    bool compile_from_source(const std::string& vs, const std::string& fs);
    void use(void) const;
    void clear(void);

    // Ubicacion de un uniform en el programa linkeado. Consultar UNA vez y
    // guardar el entero para no buscar el string en cada cuadro. Devuelve -1
    // (y avisa por consola) si el uniform no existe o fue eliminado por no
    // usarse. El valor guardado deja de ser valido si se recompila el
    // programa (compile_from_source vuelve a linkear).
    int loc(const std::string& nombre) const;

    // Setters por ubicacion cacheada (para el loop de dibujado).
    void set_uniform(int ubicacion, const glm::mat4& m) const;
    void set_uniform(int ubicacion, const glm::vec3& v) const;
    void set_uniform(int ubicacion, float valor) const;

    // Setters por nombre (legibles; buscan la ubicacion en cada llamado).
    void set_uniform(const std::string& nombre, const glm::mat4& m) const;
    void set_uniform(const std::string& nombre, const glm::vec3& v) const;
    void set_uniform(const std::string& nombre, float valor) const;

    unsigned int id(void) const   { return id_; }
private:
    unsigned int id_ {0U};
};