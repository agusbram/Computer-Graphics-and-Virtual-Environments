// -----------------------------------------------------------------------------
// main.cpp — Armado de la aeronave (Práctico 04)
//
// Escena con UN solo modelo: la aeronave, compuesta por varias primitivas
// del Práctico 03 (nariz, fuselaje, cola, alas y empenajes). Las mallas y
// las matrices LOCALES se arman una sola vez en Aircraft::init(); por cuadro
// solo se recalcula la POSE (posición + orientación del avión) y se pide la
// lista de piezas ya transformadas con collect().
//
// La verificación del práctico: un cabeceo oscilante (la nariz sube y baja)
// que se aplica a TODAS las piezas a la vez, girando alrededor del punto de
// referencia (centro de gravedad) y no de un punto arbitrario.
//
// Etapas del plasmado:
//   1. decidir los datos  -> Aircraft::init()      (primitivas + matrices locales)
//   2. pasarlos a la GPU  -> Mesh::load()          (dentro de init())
//   3. armar el programa  -> ResourceManager (lee) + Shader (compila + uniforms)
//   4. dibujar            -> collect() + glDrawElements + set_uniform
//
// La caja negra que queda (uAjuste) corrige el aspect y niega Z; la rotación
// de vista es la misma idea del Práctico 03 (mirar el modelo "de esquina"),
// pero ahora vive en main y NO se mezcla con las matrices de modelo de las
// piezas: uModel = vista * (pose * local). La matriz de vista real llega en
// la Unidad VII.
// -----------------------------------------------------------------------------

#include <cmath>            // std::sin
#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <set>              // std::set (para contar mallas distintas)
#include <string>           // std::string
#include <vector>           // std::vector

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

#include "core/Aircraft.h"
#include "core/ResourceManager.h"
#include "core/Shader.h"

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 04 - Armado de la aeronave"; // Título
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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    // ------------------------------------------------------------------
    // Etapas del plasmado. Shader y Aircraft viven dentro de un bloque { }
    // para que sus destructores corran con el contexto OpenGL vivo.
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
            const int loc_model  = shader.loc("uModel");
            const int loc_color  = shader.loc("uColor");
            const int loc_ajuste = shader.loc("uAjuste");

            // Etapas 1 y 2: la aeronave arma sus mallas y matrices locales
            // UNA sola vez (Aircraft::init no se vuelve a llamar).
            Aircraft avion;
            avion.init();

            // "Contar" (actividad aúlica): cuántas mallas distintas hay y
            // cuántas matrices de modelo se calculan por cuadro. Se cuenta
            // con la propia lista de RenderItems: piezas vs mallas distintas.
            {
                avion.update(glm::vec3(0.0f), glm::vec3(0.0f));
                std::vector<RenderItem> conteo;
                avion.collect(conteo);

                std::set<const Mesh*> mallas;
                for (const RenderItem& item : conteo) {
                    mallas.insert(item.mesh);
                }
                std::cout << "aeronave : " << conteo.size()
                          << " piezas con " << mallas.size()
                          << " mallas distintas" << std::endl;
                std::cout << "  (las cuatro placas -alas y empenajes- "
                             "comparten la malla del cubo;" << std::endl;
                std::cout << "   las matrices de modelo por cuadro son "
                          << conteo.size() << ", una por pieza)" << std::endl;
            }

            // Caja negra de hoy (vista/proyección, Unidad VII): corrige la
            // relación de aspecto de la ventana y niega el eje Z. Es una
            // matriz CONSTANTE para toda la escena.
            const glm::mat4 ajuste = glm::scale(
                glm::mat4(1.0f),
                glm::vec3(static_cast<float>(kWindowHeight) / kWindowWidth,
                          1.0f, -1.0f));

            // Rotación de vista: mirar el modelo "de esquina" para que se
            // vea el volumen (la cámara frontal + color plano dejan todo
            // chato). Se compone Ry(25) · Rx(-65) (lo escrito más a la
            // derecha se aplica primero: primero se inclina -65° alrededor de
            // X y después se rota 25° alrededor de Y). Deja el fuselaje
            // horizontal en pantalla (el eje X del modelo apunta a la derecha),
            // el "arriba" del avión casi vertical y las alas diagonales hacia
            // adentro: una vista 3/4 en la que el cabeceo de la verificación
            // se ve bien. Vive separada de las matrices de modelo de las
            // piezas: la pose y las locales describen el MODELO; la vista
            // describe cómo se lo mira. uModel = vista * item.model.
            const glm::mat4 vista =
                glm::rotate(glm::rotate(glm::mat4(1.0f),
                                        glm::radians(25.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f)),
                            glm::radians(-65.0f),
                            glm::vec3(1.0f, 0.0f, 0.0f));

            glEnable(GL_DEPTH_TEST);   // una vez, antes del loop (caja negra)

            // ------------------------------------------------------------------
            // Bucle principal de renderizado.
            // ------------------------------------------------------------------
            while (!glfwWindowShouldClose(window)) {
                processInput(window);

                // Actitud de verificación: cabeceo oscilante (la nariz sube
                // y baja). Solo cambia la pose; las piezas y las matrices
                // locales no se tocan.
                const double t = glfwGetTime();
                const float cabeceo = glm::radians(20.0f) *
                                      static_cast<float>(std::sin(t * 0.75));

                avion.update(glm::vec3(0.0f), glm::vec3(cabeceo, 0.0f, 0.0f));

                std::vector<RenderItem> items;
                avion.collect(items);

                glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                shader.use();
                shader.set_uniform(loc_ajuste, ajuste);

                // Etapa 4: dibujar. La lista ya viene con la transformación
                // compuesta (pose * local): acá solo se cambian uniforms y
                // se emite el draw call.
                // Línea por línea:                                                                                                                              
                //  1. set_uniform(loc_color, ...) — le dice al shader de qué color pintar esta pieza (uColor). Usa la                                               
                    // ubicación cacheada (loc_color), no busca el string en cada cuadro.                                                                            
                //  2. set_uniform(loc_model, vista * item.model) — le pasa la matriz que transforma el vértice. Acá se                                              
                   // compone la vista con la matriz de la pieza (que ya traía pose·local). Es el producto final vista                                              
                   // · pose · local; el shader lo aplica: gl_Position = uAjuste · uModel · v.                                                                      
                //  3. glBindVertexArray(item.mesh->vao()) — "conecta" la geometría de esa pieza (sus vértices/índices)                                              
                   // para que las próximas llamadas de dibujo usen esa malla. El VAO guarda toda la configuración de                                               
                   // atributos.                                                                                                                                    
                //  4. glDrawElements(...) — es el draw call: manda a dibujar. GL_TRIANGLES = la malla está hecha de                                                 
                   // triángulos; count() = cuántos índices tiene (lo que se dibuja); GL_UNSIGNED_INT = el tipo de los                                              
                   // índices; nullptr = los índices están en el EBO ya conectado al VAO (no hay que pasar un puntero).                                             
                //  En resumen: el main no "dibuja el avión" — recorre la lista que le entregó collect() y, por cada                                                 
                //  pieza, configura los uniformes (color + matriz) y emite un draw call. El shader, con esos uniformes                                              
                //  + la geometría del VAO, es quien produce los píxeles en la GPU.
                for (const RenderItem& item : items) {
                    shader.set_uniform(loc_color, item.color);
                    shader.set_uniform(loc_model, vista * item.model);
                    glBindVertexArray(item.mesh->vao());
                    glDrawElements(GL_TRIANGLES, item.mesh->count(),
                                   GL_UNSIGNED_INT, nullptr);
                }

                glBindVertexArray(0);   // desactivar, para no estorbar

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }   // <-- acá se destruyen shader y aeronave: destructores con contexto vivo
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
void framebuffer_size_callback([[maybe_unused]]  GLFWwindow* window,
                               int width, int height){
    glViewport(0, 0, width, height);
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
