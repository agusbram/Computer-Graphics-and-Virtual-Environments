// -----------------------------------------------------------------------------
// main.cpp — Cámara en la escena (Práctico 05)
//
// Escena con la aeronave del Práctico 04 (quieta) y una CÁMARA ORBITAL en
// perspectiva. La "caja negra" uAjuste se reemplazó por dos matrices, vista y
// proyección (Unidad VII): gl_Position = uProjection * uView * uModel * v.
//
// Por cuadro:
//   - InputHandler lee el mouse (polling) y produce los deltas (CameraCommand).
//   - CameraSystem acumula/acota esos deltas y recalcula view (lookAt).
//   - Se dibuja la lista de piezas del avión con la proyección ya armada.
// La proyección se recalcula al redimensionar la ventana (callback).
//
// Controles: botón izquierdo arrastrado = orbitar (yaw/pitch); botón derecho
// arrastrado (vertical) = acercar/alejar.
//
// IMPORTANTE: este main.cpp NO posee recursos de OpenGL (no hay glDelete*).
// -----------------------------------------------------------------------------

#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <set>              // std::set (para contar mallas distintas)
#include <string>           // std::string
#include <vector>           // std::vector

#include <glm/glm.hpp>

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

#include "core/Aircraft.h"
#include "core/CameraSystem.h"
#include "core/InputHandler.h"
#include "core/ResourceManager.h"
#include "core/Shader.h"

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 05 - Cámara en la escena"; // Título
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
    // 2. Configurar el contexto de OpenGL ANTES de crear la ventana.
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
    glfwSwapInterval(1);

    // ------------------------------------------------------------------
    // Etapas del plasmado. Shader, Aircraft y CameraSystem viven dentro de
    // un bloque { } para que sus destructores corran con el contexto vivo.
    // ------------------------------------------------------------------
    try {
        // Etapa 3a: el ResourceManager lee los fuentes de disco.
        ResourceManager resources(kAssetsRoot);
        const ShaderSource& solid = resources.load_shader_source(
            "solid", "shaders/solid.vs", "shaders/solid.fs");

        {
            // Etapa 3b: compilar y linkear (falla nunca silenciosa).
            Shader shader;
            if (!shader.compile_from_source(solid.vs, solid.fs)) {
                std::cout << "El programa de shaders quedó vacío; saliendo sin "
                             "dibujar." << std::endl;
                glfwDestroyWindow(window);
                glfwTerminate();
                return EXIT_FAILURE;
            }

            // Ubicaciones de los uniform cacheadas UNA vez: dentro del loop
            // se setea con el entero y no se busca el string en cada cuadro.
            const int loc_model      = shader.loc("uModel");
            const int loc_view       = shader.loc("uView");
            const int loc_projection = shader.loc("uProjection");
            const int loc_color      = shader.loc("uColor");

            // Etapas 1 y 2: la aeronave arma sus mallas y matrices locales
            // UNA sola vez. El avión queda QUIETO (la pose en identidad):
            // la verificación de este práctico es la cámara, no el cabeceo.
            Aircraft avion;
            avion.init();

            // "Contar" (de la actividad aúlica del Práctico 04): cuántas
            // mallas distintas hay y cuántas matrices de modelo por cuadro.
            {
                std::vector<RenderItem> conteo;
                avion.collect(conteo);

                std::set<const Mesh*> mallas;
                for (const RenderItem& item : conteo) {
                    mallas.insert(item.mesh);
                }
                std::cout << "aeronave : " << conteo.size()
                          << " piezas con " << mallas.size()
                          << " mallas distintas" << std::endl;
            }

            // Cámara orbital + entrada. La cámara arma la proyección inicial
            // con el tamaño de la ventana; el InputHandler produce los deltas
            // del mouse.
            CameraSystem camara(kWindowWidth, kWindowHeight);
            InputHandler input;

            // El callback de redimensionado es una función libre (no un
            // método): llega al objeto de la cámara a través del user pointer
            // de GLFW.
            glfwSetWindowUserPointer(window, &camara);
            glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

            // Punto al que mira la cámara: el centro del avión. El modelo
            // tiene la nariz en el origen y llega hasta x=1, así que su
            // centro geométrico está en x=0.5.
            const glm::vec3 objetivo(0.5f, 0.0f, 0.0f);

            glEnable(GL_DEPTH_TEST);   // una vez, antes del loop (caja negra)

            double tiempo_previo = glfwGetTime();

            // ------------------------------------------------------------------
            // Bucle principal de renderizado.
            // ------------------------------------------------------------------
            while (!glfwWindowShouldClose(window)) {
                processInput(window);

                // 1) entrada: el mouse produce deltas (píxeles -> rad).
                const double tiempo = glfwGetTime();
                const float dt = static_cast<float>(tiempo - tiempo_previo);
                tiempo_previo = tiempo;

                input.update(window, dt);

                // 2) cámara: acumula/acota los deltas y recalcula la vista.
                camara.update(objetivo, glm::vec3(0.0f), input.command());

                // 3) la lista de piezas del avión (modelo ya compuesto).
                std::vector<RenderItem> items;
                avion.collect(items);

                glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                shader.use();
                shader.set_uniform(loc_projection, camara.data().projection);
                shader.set_uniform(loc_view, camara.data().view);

                // Etapa 4: dibujar. Cada pieza: cambiar uColor + uModel y
                // emitir el draw call.
                for (const RenderItem& item : items) {
                    shader.set_uniform(loc_color, item.color);
                    shader.set_uniform(loc_model, item.model);
                    glBindVertexArray(item.mesh->vao());
                    glDrawElements(GL_TRIANGLES, item.mesh->count(),
                                   GL_UNSIGNED_INT, nullptr);
                }

                glBindVertexArray(0);   // desactivar, para no estorbar

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }   // <-- acá se destruyen shader, aeronave y cámara
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
// -----------------------------------------------------------------------------
void error_callback(int error, const char *description){
    glfw_error_code = error;
    glfw_error_str = std::string(description);
}

// -----------------------------------------------------------------------------
// framebuffer_size_callback
// -----------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);   // el rectángulo de píxeles donde se dibuja

    // La proyección depende del aspect del framebuffer: se recalcula en la
    // cámara. El callback es función libre, así que llega al objeto por el
    // user pointer de GLFW.
    void* p = glfwGetWindowUserPointer(window);
    if (p != nullptr) {
        static_cast<CameraSystem*>(p)->set_viewport(width, height);
    }
}

// -----------------------------------------------------------------------------
// processInput
// -----------------------------------------------------------------------------
void processInput(GLFWwindow *window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
}

// -----------------------------------------------------------------------------
// print_gl_version
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
