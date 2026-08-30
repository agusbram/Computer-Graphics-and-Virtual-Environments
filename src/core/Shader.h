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
// -----------------------------------------------------------------------------

#pragma once

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

    unsigned int id(void) const   { return id_; }
private:
    unsigned int id_ {0U};
};