// -----------------------------------------------------------------------------
// main.cpp — Primitivas paramétricas y matriz de modelo (Práctico 03)
//
// Escena con TRES piezas (cubo, cilindro y cono), cada una en distinto lugar,
// con distinto tamaño y distinto color. Las mallas se generan UNA sola vez;
// lo que cambia entre piezas son los uniform: la matriz de modelo (uModel) y
// el color (uColor).
//
// Etapas del plasmado, ahora con los módulos:
//   1. decidir los datos  -> primitives::cube/cylinder/cone  (generación paramétrica)
//   2. pasarlos a la GPU  -> Mesh::load()   (VAO con 3 atributos)
//   3. armar el programa  -> ResourceManager (lee) + Shader (compila + uniforms)
//   4. dibujar            -> glDrawElements + set_uniform (dentro del loop)
//
// IMPORTANTE: este main.cpp NO posee recursos de OpenGL: no hay ni un solo
// glDelete*. La caja negra que queda (uAjuste) corrige el aspect y niega Z;
// el test de profundidad se mantiene como en el Práctico 02.
// -----------------------------------------------------------------------------

#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <string>           // std::string

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

#include "core/Mesh.h"
#include "core/MeshData.h"
#include "core/Primitives.h"
#include "core/ResourceManager.h"
#include "core/Shader.h"

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 03 - Primitivas paramétricas"; // Título
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
    // Etapas del plasmado. Shader y Mesh viven dentro de un bloque { }
    // para que sus destructores corran con el contexto OpenGL vivo.
    // ------------------------------------------------------------------
    try {
        // Etapa 3a: el ResourceManager lee los fuentes de disco.
        ResourceManager resources(kAssetsRoot);
        const ShaderSource& solid = resources.load_shader_source(
            "solid", "shaders/solid.vs", "shaders/solid.fs");

        // Etapa 1: las mallas se GENERAN una sola vez (fuera del loop).
        // La cantidad de gajos es un parámetro: cambiarlo regenera la malla
        // sin tocar una sola coordenada a mano.
        const MeshData cube_data     = primitives::cube();
        const MeshData cylinder_data = primitives::cylinder(0.26f, 0.95f, 20U);
        const MeshData sphere_data   = primitives::sphere(0.26f, 20U, 12U);
        const MeshData cone_data     = primitives::cone(0.26f, glm::radians(60.0f), 20U);

        std::cout << "cubo     : " << cube_data.vertices.size()
                  << " vértices, " << cube_data.indices.size()
                  << " índices (se esperan 24 y 36)" << std::endl;
        std::cout << "cilindro : " << cylinder_data.vertices.size()
                  << " vértices, " << cylinder_data.indices.size()
                  << " índices (lateral 2(N+1) = 42 con N=20)" << std::endl;
        std::cout << "esfera   : " << sphere_data.vertices.size()
                  << " vértices, " << sphere_data.indices.size()
                  << " índices (anillos x 2(N+1) + polares)" << std::endl;
        std::cout << "cono     : " << cone_data.vertices.size()
                  << " vértices, " << cone_data.indices.size()
                  << " índices (N+1 del anillo + ápice)" << std::endl;

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

            // Etapa 2: subir las mallas a la GPU (cada Mesh posee su VAO/VBO/EBO).
            Mesh cube;
            cube.load(cube_data);
            Mesh cylinder;
            cylinder.load(cylinder_data);
            Mesh sphere;
            sphere.load(sphere_data);
            Mesh cone;
            cone.load(cone_data);

            // Caja negra de hoy (vista/proyección, Unidad VII): corrige la
            // relación de aspecto de la ventana y niega el eje Z. Es una
            // matriz CONSTANTE para toda la escena.
            const glm::mat4 ajuste = glm::scale(
                glm::mat4(1.0f),
                glm::vec3(static_cast<float>(kWindowHeight) / kWindowWidth,
                          1.0f, -1.0f));

            // Inclinación común de la vista de la escena. Como la cámara mira
            // de frente (en -z) y uAjuste todavia NO tiene perspectiva (Unidad
            // VII), una pieza sin rotar queda "de frente" y, pintada con un
            // color plano, se ve como una figura chata: el cubo como un
            // cuadrado, el cilindro como un rectángulo y el cono como un
            // triángulo. En el Práctico 02 esa rotación estaba adentro de la
            // matriz "caja negra"; a partir de ahora se arma con la MATRIZ DE
            // MODELO, que es exactamente para eso.
            // Se aplica PRIMERO Ry(35) y después Rx(-25) (lo de más a la
            // derecha se aplica primero): una vista "de esquina".
            const glm::mat4 inclinacion =
                glm::rotate(glm::rotate(glm::mat4(1.0f),
                                        glm::radians(-25.0f),
                                        glm::vec3(1.0f, 0.0f, 0.0f)),
                            glm::radians(35.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

            // Posiciones x (layout: cubo, cilindro, esfera, cono, de izquierda a
            // derecha: -0.92 / -0.32 / 0.26 / 0.82). Elegidas con un
            // chequeador de bounding-box en NDC sobre los vértices REALES
            // (con inclinación y uAjuste): todas quedan dentro de [-1,1] y
            // con un hueco claro entre piezas (cubo-cilindro 0.06, etc.).
            glm::mat4 model_cube = glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(-0.92f, 0.0f, 0.0f));
            model_cube = model_cube * inclinacion;
            model_cube = glm::scale(model_cube, glm::vec3(0.40f));

            glm::mat4 model_cylinder = glm::translate(glm::mat4(1.0f),
                                                      glm::vec3(-0.32f, 0.0f, 0.0f));
            model_cylinder = model_cylinder * inclinacion;

            glm::mat4 model_sphere = glm::translate(glm::mat4(1.0f),
                                                    glm::vec3(0.26f, 0.0f, 0.0f));
            model_sphere = model_sphere * inclinacion;

            glm::mat4 model_cone = glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(0.82f, 0.0f, 0.0f));
            model_cone = model_cone * inclinacion;

            glEnable(GL_DEPTH_TEST);   // una vez, antes del loop (caja negra)

            // ------------------------------------------------------------------
            // Bucle principal de renderizado.
            // ------------------------------------------------------------------
            while (!glfwWindowShouldClose(window)) {
                processInput(window);

                glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                shader.use();
                shader.set_uniform(loc_ajuste, ajuste);

                // Etapa 4: dibujar. Cada pieza: cambiar uColor + uModel y
                // dibujar la MISMA malla con distinta transformación.
                shader.set_uniform(loc_color, glm::vec3(1.0f, 0.25f, 0.25f));
                shader.set_uniform(loc_model, model_cube);
                glBindVertexArray(cube.vao());
                glDrawElements(GL_TRIANGLES, cube.count(), GL_UNSIGNED_INT,
                               nullptr);

                shader.set_uniform(loc_color, glm::vec3(0.25f, 1.0f, 0.35f));
                shader.set_uniform(loc_model, model_cylinder);
                glBindVertexArray(cylinder.vao());
                glDrawElements(GL_TRIANGLES, cylinder.count(), GL_UNSIGNED_INT,
                               nullptr);

                shader.set_uniform(loc_color, glm::vec3(1.0f, 0.85f, 0.25f));
                shader.set_uniform(loc_model, model_sphere);
                glBindVertexArray(sphere.vao());
                glDrawElements(GL_TRIANGLES, sphere.count(), GL_UNSIGNED_INT,
                               nullptr);

                shader.set_uniform(loc_color, glm::vec3(0.35f, 0.55f, 1.0f));
                shader.set_uniform(loc_model, model_cone);
                glBindVertexArray(cone.vao());
                glDrawElements(GL_TRIANGLES, cone.count(), GL_UNSIGNED_INT,
                               nullptr);

                glBindVertexArray(0);   // desactivar, para no estorbar

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }   // <-- acá se destruyen shader y meshes: destructores con contexto vivo
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