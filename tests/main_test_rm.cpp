// -----------------------------------------------------------------------------
// test del ResourceManager — C++ puro, sin OpenGL.
// Compilar (desde la raiz del proyecto):
//   g++ -std=c++17 -Wall -Wextra -I./src src/core/ResourceManager.cpp tests/main_test_rm.cpp -o build/test_rm
// -----------------------------------------------------------------------------

#include <iostream>
#include <stdexcept>
#include <string>

#include "core/ResourceManager.h"

namespace {

void check(bool cond, const std::string& description)
{
    std::cout << (cond ? "  [OK]   " : "  [FALLO] ") << description
              << std::endl;
    if (!cond) {
        throw std::runtime_error("prueba fallida: " + description);
    }
}

} // namespace

int main()
{
    try {
        // --------------------------------------------------------------
        // 1. Carga normal: lee los dos archivos desde ./assets/shaders
        // --------------------------------------------------------------
        ResourceManager rm("./assets");
        const ShaderSource& tri = rm.load_shader_source(
            "triangle", "shaders/triangle.vs", "shaders/triangle.fs");

        std::cout << "1. Carga inicial de 'triangle'" << std::endl;
        check(!tri.vs.empty(), "el vertex shader se leyo de disco");
        check(!tri.fs.empty(), "el fragment shader se leyo de disco");
        check(tri.vs.find("#version 460 core") != std::string::npos,
              "el fuente vs tiene contenido esperado");

        // --------------------------------------------------------------
        // 2. Cacheo: pedir la MISMA clave con LOS MISMOS archivos no vuelve
        //    a leer del disco. Si devuelve exactamente la misma direccion,
        //    es la misma copia en memoria: no hubo segunda lectura.
        // --------------------------------------------------------------
        const ShaderSource& otra_vez = rm.load_shader_source(
            "triangle", "shaders/triangle.vs", "shaders/triangle.fs");

        std::cout << "2. Segundo pedido de la misma clave" << std::endl;
        check(&tri == &otra_vez,
              "misma direccion => devolvio el cache (no releyo)");

        // get_ tambien sirve sin tocar disco.
        const ShaderSource& por_get = rm.get_shader_source("triangle");
        check(&tri == &por_get, "get_shader_source devuelve el cache");

        // --------------------------------------------------------------
        // 3. Clave repetida con OTROS archivos: warning y gana el cache.
        // --------------------------------------------------------------
        const ShaderSource& conflicto = rm.load_shader_source(
            "triangle", "shaders/inexistentes.vs", "shaders/inexistentes.fs");

        std::cout << "3. Misma clave con archivos distintos" << std::endl;
        std::cout.flush(); // para que el aviso del modulo (stderr) salga en orden
        check(&tri == &conflicto,
              "se conservo la copia cacheada original");

        // --------------------------------------------------------------
        // 4. Fallo: archivo inexistente -> excepcion, nunca silencio.
        // --------------------------------------------------------------
        bool lanzo = false;
        try {
            rm.load_shader_source("fantasma", "shaders/no_existe.vs",
                                  "shaders/triangle.fs");
        } catch (const std::runtime_error& e) {
            lanzo = true;
            std::cout << "4. Archivo inexistente" << std::endl;
            std::cout << "  [OK]   excepcion esperada: " << e.what()
                      << std::endl;
        }
        check(lanzo, "load con archivo inexistente lanza runtime_error");

        // --------------------------------------------------------------
        // 5. Fallo: clave que nunca se cargo -> excepcion en get_.
        // --------------------------------------------------------------
        lanzo = false;
        try {
            rm.get_shader_source("nadie_cargo_esto");
        } catch (const std::runtime_error& e) {
            lanzo = true;
            std::cout << "5. Clave inexistente" << std::endl;
            std::cout << "  [OK]   excepcion esperada: " << e.what()
                      << std::endl;
        }
        check(lanzo, "get de clave sin cargar lanza runtime_error");

        // --------------------------------------------------------------
        // 6. clear(): vacia el cache; las referencias viejas quedan
        //    invalidas y las claves ya no resuelven.
        // --------------------------------------------------------------
        rm.clear();
        lanzo = false;
        try {
            rm.get_shader_source("triangle");
        } catch (const std::runtime_error&) {
            lanzo = true;
        }

        std::cout << "6. clear()" << std::endl;
        check(lanzo, "tras clear(), la clave ya no esta disponible");
    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nTodas las pruebas pasaron." << std::endl;
    return EXIT_SUCCESS;
}
