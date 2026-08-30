// -----------------------------------------------------------------------------
// main.cpp — Cubo indexado con módulos (Práctico 02: "Del triángulo a la malla")
//
// Las cuatro etapas del triángulo, ahora repartidas en módulos:
//   1. decidir los datos  -> primitives::cube()   (24 vértices, 36 índices)
//   2. pasarlos a la GPU  -> Mesh::load()
//   3. armar el programa  -> ResourceManager (lee el disco) + Shader (compila)
//   4. dibujar            -> glDrawElements      (cada cuadro, dentro del loop)
//
// IMPORTANTE: este main.cpp NO posee recursos de OpenGL. No hay ni un solo
// glDelete* (los destructores de Mesh y Shader liberan). Lo único que hace
// main con la GPU, además del flujo normal de la ventana (clear/swap/vsync),
// es dibujar dentro del loop: glBindVertexArray + glDrawElements.
// -----------------------------------------------------------------------------

#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <string>           // std::string

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

#include "core/Mesh.h"
#include "core/MeshData.h"
#include "core/Primitives.h"
#include "core/ResourceManager.h"
#include "core/Shader.h"

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 02 - Cubo indexado"; // Título
static constexpr int kWindowWidth   = 800;  // Ancho inicial en píxeles
static constexpr int kWindowHeight  = 600;  // Alto inicial en píxeles
static constexpr int kGLVerMajor    = 4;    // Versión mayor de OpenGL pedida
static constexpr int kGLVerMinor    = 6;    // Versión menor de OpenGL pedida

// Ruta de los recursos del proyecto (relativa a donde se ejecuta el binario)
static const char* kAssetsRoot = "./assets";

// Variables globales para guardar el último error reportado por GLFW
// (las rellena la función error_callback, ver más abajo)
static int glfw_error_code{};
static std::string glfw_error_str{};

// Declaraciones anticipadas (prototipos) de las funciones auxiliares
static void error_callback(int error, const char *description);
static void framebuffer_size_callback(GLFWwindow* window,
                                      int width, int height);
static void processInput(GLFWwindow *window);
static void print_gl_version(void);

int main()
{
    // Registrar el callback de errores ANTES de inicializar GLFW,
    // así nos enteramos de cualquier fallo que ocurra en glfwInit()
    glfwSetErrorCallback(error_callback);

    // ------------------------------------------------------------------
    // 1. Inicializar GLFW
    // ------------------------------------------------------------------
    if (!glfwInit()) {
        std::cout << "GLFW initialization failed! - GLFW("
                  << glfw_error_code << "): " << glfw_error_str << std::endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------
    // 2. Configurar el contexto de OpenGL (versión y perfil core) ANTES
    //    de crear la ventana.
    // ------------------------------------------------------------------
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLVerMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLVerMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ------------------------------------------------------------------
    // 3. Crear la ventana y su contexto OpenGL asociado.
    // ------------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(kWindowWidth,
                                          kWindowHeight,
                                          kWindowTitle, nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        std::cout << "GLFW window creation failed! - GLFW("
                  << glfw_error_code << "): " << glfw_error_str << std::endl;
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    // ------------------------------------------------------------------
    // 4. Cargar las funciones de OpenGL con GLAD.
    // ------------------------------------------------------------------
    if (!gladLoadGL(glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "GLAD initialization failed!" << std::endl;
        return EXIT_FAILURE;
    }

    print_gl_version();

    // ------------------------------------------------------------------
    // 5. Registrar el callback de redimensionado de la ventana.
    // ------------------------------------------------------------------
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ------------------------------------------------------------------
    // 6. Sincronización con el monitor (vsync).
    // ------------------------------------------------------------------
    glfwSwapInterval(1);

    // ------------------------------------------------------------------
    // Etapas del plasmado (1 a 4), con los módulos.
    // Shader y Mesh viven dentro de un bloque { }: sus destructores tienen
    // que correr con el contexto OpenGL todavía vivo (antes de cerrar GLFW),
    // o liberar los recursos de la GPU puede ser un segfault al salir.
    // ------------------------------------------------------------------
    try {
        // Etapa 3a: el ResourceManager lee los fuentes de disco.
        ResourceManager resources(kAssetsRoot);
        const ShaderSource& solid = resources.load_shader_source(
            "solid", "shaders/solid.vs", "shaders/solid.fs");

        // Etapa 1: la malla se GENERA (ya no se escribe a mano en main).
        // Consola: "se sabe que está bien" si dice 24 y 36.
        const MeshData cube_data = primitives::cube();
        std::cout << "Malla generada: " << cube_data.vertices.size()
                  << " vértices y " << cube_data.indices.size()
                  << " índices (se esperan 24 y 36)" << std::endl;

        {
            // Etapa 3b: compilar y linkear. Si el shader está roto, se imprime
            // el log del driver y compile_from_source devuelve false; el
            // programa queda vacío y no se dibuja nada (falla 2 de la clínica).
            Shader shader;
            if (!shader.compile_from_source(solid.vs, solid.fs)) {
                std::cout << "El programa de shaders quedó vacío; saliendo sin "
                             "dibujar." << std::endl;
                glfwDestroyWindow(window);
                glfwTerminate();
                return EXIT_FAILURE;
            }

            // Etapa 2: subir la malla a la GPU y armar el VAO.
            Mesh cube;
            cube.load(cube_data);

            // Caja negra de hoy: test de profundidad. Determina qué triángulo
            // se ve cuando varios compiten por el mismo píxel (el más cercano).
            glEnable(GL_DEPTH_TEST);

            // ------------------------------------------------------------------
            // Bucle principal de renderizado.
            // ------------------------------------------------------------------
            while (!glfwWindowShouldClose(window)) {
                processInput(window);

                // Limpiar COLOR y PROFUNDIDAD en cada cuadro: sin esto las
                // caras de cuadros anteriores "ganarían" el test de profundidad.
                glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Etapa 4: dibujar con índices. count() = cantidad de INDICES
                // (36); el VAO ya sabe dónde está el buffer de índices.
                shader.use();
                glBindVertexArray(cube.vao());
                glDrawElements(GL_TRIANGLES, cube.count(), GL_UNSIGNED_INT,
                               nullptr);
                glBindVertexArray(0);   // desactivar, para no estorbar

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }   // <-- acá se destruyen shader y cube: destructores con contexto vivo
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------------
// error_callback
// Callback que GLFW invoca cuando ocurre un error interno.
// Guarda el código y la descripción del error en variables globales para
// poder mostrarlos después.
// -----------------------------------------------------------------------------
void error_callback(int error, const char *description){
    glfw_error_code = error;
    glfw_error_str = std::string(description);
}

// -----------------------------------------------------------------------------
// framebuffer_size_callback
// Callback que GLFW invoca cuando el usuario redimensiona la ventana.
// Ajusta el viewport (la región donde OpenGL dibuja) al nuevo tamaño,
// para que la imagen no quede deformada ni cortada.
// -----------------------------------------------------------------------------
void framebuffer_size_callback([[maybe_unused]]  GLFWwindow* window,
                               int width, int height){
    glViewport(0, 0, width, height);
}

// -----------------------------------------------------------------------------
// processInput
// Consulta el estado del teclado en cada frame.
// Si se presionó ESC, pedimos que la ventana se cierre (el bucle principal
// detectará glfwWindowShouldClose y terminará).
// -----------------------------------------------------------------------------
void processInput(GLFWwindow *window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
}

// -----------------------------------------------------------------------------
// print_gl_version
// Imprime por consola la información del driver OpenGL del sistema:
// fabricante, tarjeta gráfica, versión de OpenGL y versión de GLSL.
// Útil para depurar y saber qué características están disponibles.
// -----------------------------------------------------------------------------
void print_gl_version(void){
    std::cout << " OpenGL Vendor: "
              << glGetString(GL_VENDOR) << std::endl;
    std::cout << " OpenGL Renderer: "
              << glGetString(GL_RENDERER) << std::endl;
    std::cout << " OpenGL Version: "
              << glGetString(GL_VERSION) << std::endl;
    std::cout << " GLSL Version: "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}