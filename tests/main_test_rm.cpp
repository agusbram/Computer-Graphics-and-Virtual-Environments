// -----------------------------------------------------------------------------
// test del ResourceManager — C++ puro, sin OpenGL.
// Compilar (desde la raiz del proyecto). Desde el Práctico 08 el módulo usa
// stb_image, por eso hace falta también -I./third_party/stb:
//   g++ -std=c++17 -Wall -Wextra -I./src -I./third_party/stb src/core/ResourceManager.cpp tests/main_test_rm.cpp -o build/test_rm
// -----------------------------------------------------------------------------

#include <cstdlib>                 // EXIT_SUCCESS y EXIT_FAILURE: el resultado del programa de prueba
#include <iostream>                // std::cout y std::cerr: informar cada prueba
#include <stdexcept>               // std::runtime_error: se verifica que los errores se lancen
#include <string>                  // std::string: claves, rutas y contenidos que se comparan

#include "core/ResourceManager.h"  // la clase que se prueba

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

        // ==============================================================
        // Práctico 08 — IMÁGENES (stb_image). Siguen siendo C++ puro:
        // decodificar una imagen no necesita OpenGL.
        // ==============================================================

        // 7. Carga normal: grass.png es RGB (3 canales) de 256x256; se
        //    guarda SIEMPRE como RGBA (4 bytes por texel).
        const ImageData& pasto = rm.load_image("pasto", "textures/suelo/grass.png", true);
        std::cout << "7. Carga de una imagen" << std::endl;
        check(pasto.width == 256 && pasto.height == 256, "tamano 256x256");
        check(pasto.channels == 3, "el archivo tiene 3 canales (informativo)");
        check(pasto.pixels.size() == 256U * 256U * 4U, "pixels en RGBA: 4 bytes por texel");

        // 8. Caché: el mismo pedido devuelve la MISMA imagen (sin leer el disco).
        const ImageData& pasto2 = rm.load_image("pasto", "textures/suelo/grass.png", true);
        std::cout << "8. Cache de imagenes" << std::endl;
        check(&pasto == &pasto2, "el segundo pedido devuelve la copia en cache");

        // 9. Volteo vertical: la misma imagen con y sin flip (DOS claves). La
        //    fila 0 de una tiene que ser la ULTIMA fila de la otra. Esto
        //    tambien prueba que stbi_set_flip_vertically_on_load se fija en
        //    CADA carga (es un estado global de stb).
        const ImageData& sin_flip = rm.load_image("pasto_sin_flip", "textures/suelo/grass.png", false);
        const std::size_t fila = 256U * 4U;   // bytes por fila
        bool invertida = true;
        for (std::size_t b = 0; b < fila; ++b) {
            if (pasto.pixels[b] != sin_flip.pixels[255U * fila + b]) {
                invertida = false;
                break;
            }
        }
        std::cout << "9. Volteo vertical" << std::endl;
        check(invertida, "fila 0 con flip == ultima fila sin flip");

        // 10. Misma clave con OTRO flip: aviso + gana el cache (como shaders).
        std::cout << "10. Misma clave con otro flip" << std::endl;
        std::cout.flush();
        const ImageData& conflicto_img = rm.load_image("pasto", "textures/suelo/grass.png", false);
        check(&conflicto_img == &pasto, "se conserva la primera carga");

        // 11. unload_image: libera la copia de la CPU; get_image ya no la encuentra.
        std::cout << "11. unload_image" << std::endl;
        check(rm.unload_image("pasto"), "unload_image devuelve true si la clave estaba");
        check(!rm.unload_image("pasto"), "y false si ya no estaba");
        lanzo = false;
        try {
            rm.get_image("pasto");
        } catch (const std::runtime_error&) {
            lanzo = true;
        }
        check(lanzo, "get_image de una imagen liberada lanza runtime_error");

        // 12. Archivo inexistente -> excepción con mensaje claro.
        lanzo = false;
        try {
            rm.load_image("fantasma", "textures/no_existe.png", true);
        } catch (const std::runtime_error& e) {
            lanzo = true;
            std::cout << "12. Imagen inexistente" << std::endl;
            std::cout << "  [OK]   excepcion esperada: " << e.what() << std::endl;
        }
        check(lanzo, "load_image con archivo inexistente lanza runtime_error");
    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nTodas las pruebas pasaron." << std::endl;
    return EXIT_SUCCESS;
}
