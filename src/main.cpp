// -----------------------------------------------------------------------------
// main.cpp — FDM, game loop e input (Práctico 06)
//
// Escena con la aeronave (Práctico 04) + cámara orbital (Práctico 05) cuyo
// movimiento ahora lo dicta el modelo de dinámica de vuelo (dlfdm), con el
// teclado controlando los cuatro mandos.
//
// Lo nuevo del Práctico 06:
//   1. GAME LOOP con PASO FIJO + ACUMULADOR: el FDM necesita un paso pequeño y
//      constante (dt = 1/120 s) para que el error de integración no se acumule,
//      mientras que los cuadros dependen de la máquina. Se acumula el frame_dt
//      (con tope 0.25 s para cortar la divergencia) y se consume a pasos
//      enteros.
//   2. TECLADO en InputHandler: los cuatro mandos por polling, con RAMPAS.
//   3. FDM en el loop: update() adentro del while interno y getState() una vez
//      por cuadro; to_world() lleva la salida NED a coordenadas de la escena.
//
// NOTA de escala: el FDM trabaja en SI (metros, m/s) y el avión está a ~5000 m;
// la cámara lo orbita (el objetivo sigue su posición), así que el tamaño en
// pantalla no depende de esa magnitud. El modelo del avión mide 1 unidad.
//
// IMPORTANTE: este main.cpp NO posee recursos de OpenGL (no hay glDelete*).
// -----------------------------------------------------------------------------

#include <algorithm>        // std::min
#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <set>              // std::set (para contar mallas distintas)
#include <string>           // std::string
#include <vector>           // std::vector

#include <glm/glm.hpp>

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

// dlfdm: modelo de dinámica de vuelo (Práctico 06)
#include <dlfdm/defines.h>
#include <dlfdm/fdmsolver.h>
#include <dlfdm/models/aircraft/jettrainer.h>

#include "core/Aircraft.h"
#include "core/CameraSystem.h"
#include "core/FlightData.h"
#include "core/InputHandler.h"
#include "core/ResourceManager.h"
#include "core/Shader.h"
#include "core/Terrain.h"

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 06 - FDM, game loop e input"; // Título
static constexpr int kWindowWidth   = 800;  // Ancho inicial en píxeles
static constexpr int kWindowHeight  = 600;  // Alto inicial en píxeles
static constexpr int kGLVerMajor    = 4;    // Versión mayor de OpenGL pedida
static constexpr int kGLVerMinor    = 6;    // Versión menor de OpenGL pedida

// Ruta de los recursos del proyecto (relativa a donde se ejecuta el binario)
static const char* kAssetsRoot = "./assets";

// Paso FIJO del FDM (debe coincidir con el que usa el solver: 1/120 s).
static constexpr double kFdmDt = 1.0 / 120.0;

// ESCALA GLOBAL (Práctico 07): 1 unidad de escena = 10 m. El modelo del avión
// (1 unidad) representa ~10 m (el real mide ~9 m); el terreno, sus alturas y la
// posición del FDM se llevan a unidades de escena dividiendo por este valor.
static constexpr float kMetrosPorUnidad = 10.0f;

// Variables globales para guardar el último error reportado por GLFW
static int glfw_error_code{};
static std::string glfw_error_str{};

// Contexto que se cuelga de la ventana (glfwSetWindowUserPointer) para que las
// funciones callback (que son libres, no métodos) puedan llegar a los objetos
// de la aplicación. Guarda la cámara (para el redimensionado) y los toggles
// discretos (pausa / wireframe) que cambian por eventos de teclado.
struct AppContext {
    CameraSystem* camara = nullptr;
    bool paused    = false;
    bool wireframe = false;
};

static void error_callback(int error, const char *description);
static void framebuffer_size_callback(GLFWwindow* window,
                                      int width, int height);
static void key_callback(GLFWwindow* window, int key, int scancode,
                         int action, int mods);
static void processInput(GLFWwindow *window);
static void print_gl_version(void);

int main()
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cout << "GLFW initialization failed! - GLFW("
                  << glfw_error_code << "): " << glfw_error_str << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLVerMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLVerMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // MSAA: suaviza los bordes de las figuras (sin esto titilan/escalonan al
    // moverse). El framebuffer se crea con 4 muestras por pixel.
    glfwWindowHint(GLFW_SAMPLES, 4);

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
    glfwSetWindowPos(window, 80, 60);   // asegura que la ventana quede visible en pantalla

    if (!gladLoadGL(glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "GLAD initialization failed!" << std::endl;
        return EXIT_FAILURE;
    }

    print_gl_version();
    glfwSwapInterval(1);   // 1 = vsync (los cuadros siguen el refresco del monitor)

    try {
        ResourceManager resources(kAssetsRoot);
        const ShaderSource& solid = resources.load_shader_source(
            "solid", "shaders/solid.vs", "shaders/solid.fs");

        {
            Shader shader;
            if (!shader.compile_from_source(solid.vs, solid.fs)) {
                std::cout << "El programa de shaders quedó vacío; saliendo sin "
                             "dibujar." << std::endl;
                glfwDestroyWindow(window);
                glfwTerminate();
                return EXIT_FAILURE;
            }

            const int loc_model      = shader.loc("uModel");
            const int loc_view       = shader.loc("uView");
            const int loc_projection = shader.loc("uProjection");
            const int loc_color      = shader.loc("uColor");

            // --- Escena: la aeronave (Práctico 04) -------------------------
            Aircraft avion;
            avion.init();

            // --- FDM (Práctico 06) -----------------------------------------
            // FDMSolver es la CAJA NEGRA que resuelve el movimiento del avión
            // en el tiempo: integra numéricamente las ecuaciones de la dinámica
            // de cuerpo rígido (ΣF=m·a, ΣM=I·ω̇) para ir del estado actual al
            // siguiente, con Euler y paso fijo (x += ẋ·dt). En cada update():
            // (1) con la velocidad/actitud actuales y los comandos calcula las
            // fuerzas y momentos; (2) divide por masa/inercia -> aceleraciones;
            // (3) integra -> velocidades y luego posición/actitud; (4) guarda
            // el estado nuevo. No hace falta entender la física de adentro (es
            // de otra materia); solo qué pide y qué devuelve.
            // El solver se queda con una copia de los parámetros; el paso de
            // integración se fija en su construcción (1/120 s por defecto).
            dlfdm::AircraftParameters parametros = dlfdm::jettrainer::load_model();
            dlfdm::FDMSolver fdm(parametros);

            // Condición inicial de equilibrio (trim): estado Y comandos.
            // Es un par: el estado va al solver y los comandos al InputHandler.
            dlfdm::TrimPoint trim = dlfdm::jettrainer::get_trim_condition(
                dlfdm::jettrainer::TrimCondition::kISA5000TAS150);
            fdm.setState(trim.state);                 // estado -> solver
            // los comandos -> InputHandler (antes del loop, si no el avión se
            // descompensa solo en el primer cuadro)
            InputHandler input;
            input.set_controls(trim.controls);
            input.set_limits(parametros);

            // --- Escena: el terreno procedural (Práctico 07) ---------------
            // N=200 celdas, paso 2.5 u (25 m) -> 5 km de lado; amplitud de
            // relieve 20 u (el real es ~1/3). El terreno se ubica a una ALTITUD
            // FIJA en el mundo (no sigue al avión en vertical): a la altitud de
            // trim menos un despeje chico, así el avión vuela BAJO sobre él y,
            // con el elevador, puede descender hasta tocarlo. Semilla y
            // frecuencia fijan un relieve reproducible.
            const float kDespeje = 15.0f;   // u (~150 m) de vuelo sobre el terreno
            const float altura_terreno =
                -trim.state.inertial_position.z / kMetrosPorUnidad - kDespeje;
            Terrain terreno(200U, 2.5f, 20.0f, altura_terreno, 1337, 0.005f);
            terreno.init();

            std::cout << "terreno  : " << terreno.celdas() << "x" << terreno.celdas()
                      << " celdas, paso " << terreno.paso()
                      << " u (terreno de " << terreno.celdas() * terreno.paso()
                      << " u = " << terreno.celdas() * terreno.paso() * kMetrosPorUnidad
                      << " m de lado); culling activado; el avion vuela a "
                      << kDespeje << " u del suelo" << std::endl;

            std::cout << "aeronave : " << "escena compuesta por primitivas"
                      << std::endl;
            std::cout << "FDM      : jet trainer (Roskam 'C'), trim a 5000 m / "
                      << "150 m/s; dt = " << kFdmDt << " s" << std::endl;
            std::cout << "teclas   : flechas=cabeceo/alerones, Q/E=timon, "
                      << "W/S=potencia, ESPACIO=pausa, F=wireframe" << std::endl;
            {
                GLint muestras = 0;
                glGetIntegerv(GL_SAMPLES, &muestras);
                std::cout << "MSAA     : " << muestras
                          << " muestras/pixel (antialiasing)" << std::endl;
            }

            // --- Cámara orbital (Práctico 05) + input ----------------------
            CameraSystem camara(kWindowWidth, kWindowHeight);

            // Contexto para las callbacks (cámara + toggles discretos).
            AppContext ctx;
            ctx.camara = &camara;
            glfwSetWindowUserPointer(window, &ctx);
            glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
            glfwSetKeyCallback(window, key_callback);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_MULTISAMPLE);   // activa el antialiasing (MSAA) pedido arriba

            // Eliminación de superficies ocultas (Práctico 07, Parte 3):
            //   - depth test (z-buffer): resuelve, por fragmento, quién está
            //     adelante (independiente del orden de dibujo).
            //   - recorte de cara trasera (back-face culling): descarta los
            //     polígonos que miran hacia el otro lado ANTES de rasterizarlos.
            //     OpenGL decide por el WINDING (sentido antihorario = cara
            //     frontal), NO usa el atributo normal. El terreno se generó
            //     antihorario visto desde arriba -> sus caras miran hacia +Z y
            //     desde arriba no se descartan.
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);     // descartar las traseras (default)
            glFrontFace(GL_CCW);     // antihorario = frontal (default)

            // =============================================================
            // GAME LOOP con PASO FIJO + ACUMULADOR (Práctico 06, Parte 1)
            // =============================================================
            double antes = glfwGetTime();
            double acc   = 0.0;

            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
                processInput(window);   // ESC cierra

                // Dos relojes: el real (frame_dt) y el del modelo (kFdmDt).
                const double ahora    = glfwGetTime();
                const double frame_dt = ahora - antes;
                antes = ahora;

                // La entrada se lee UNA vez por cuadro (afuera del while
                // interno): mover la cámara/leer teclas es un control continuo
                // por cuadro, no por paso de simulación.
                input.update(window, static_cast<float>(frame_dt));

                if (!ctx.paused) {
                    // El tope de 0.25 s corta la divergencia del acumulador:
                    // si un cuadro tardó muchísimo, en vez de encadenar cada
                    // vez más pasos (y congelarse) se deja de simular en tiempo
                    // real a propósito.
                    acc += std::min(frame_dt, 0.25);
                    while (acc >= kFdmDt) {
                        // UN paso de integración (el solver hace la física de
                        // adentro) con los comandos actuales. Siempre kFdmDt,
                        // nunca frame_dt: el paso del FDM es fijo.
                        fdm.update(input.controls());
                        acc -= kFdmDt;
                    }
                }

                // Una vez por cuadro: leer el estado ya integrado (posición NED
                // + actitud φ/θ/ψ) y llevarlo a la escena en UNIDADES (1u = 10 m).
                FlightData flight = to_world(fdm.getState());
                flight.position /= kMetrosPorUnidad;   // metros -> unidades de escena

                // Lectura de vuelo (una vez por segundo): da feedback de W/S.
                // (La velocidad NO se ve en pantalla: la cámara sigue al avión y
                // el terreno también -> no hay referencia de movimiento.)
                {
                    static double ult = -1.0;   // instante del último mensaje (para no imprimir cada cuadro)
                    if (ahora - ult >= 1.0) {
                        ult = ahora;
                        const dlfdm::AircraftState& st = fdm.getState();
                        const float tas = glm::length(st.body_velocity);
                        std::cout << "[vuelo] TAS=" << tas << " m/s ("
                                  << tas * 3.6f << " km/h)  throttle="
                                  << input.controls().throttle
                                  << "  alt=" << -st.inertial_position.z << " m"
                                  << std::endl;
                    }
                }

                // ORIGEN FLOTANTE ("floating origin"): el FDM trabaja en metros
                // y el avión está a ~5000 m (y se aleja volando), pero la cámara
                // lo sigue a pocas unidades. Con `float` (≈7 dígitos), números
                // grandes tienen un ULP grande; restar la posición de la cámara
                // (~500) a la del mundo (~500) para obtener una distancia chica
                // PIERDE PRECISIÓN y contamina la PROFUNDIDAD -> Z-FIGHTING (en
                // las uniones del avión y en el terreno). Solución: renderizar
                // RELATIVO a un punto cercano. Acá la referencia es el avión (lo
                // que la cámara mira): mismo resultado visual, coordenadas chicas
                // -> profundidad exacta. El terreno también queda relativo a él.
                const glm::vec3 pos_avion = flight.position;   // posición real (u)
                terreno.update(pos_avion);                     // ventana infinita

                flight.position = glm::vec3(0.0f);             // avión al origen

                // El avión se ubica en la escena y la cámara lo orbita (objetivo
                // en el origen, donde quedó el avión).
                avion.update(flight);
                camara.update(glm::vec3(0.0f),
                              glm::vec3(flight.theta, flight.phi, flight.psi),
                              input.camera_cmd());

                glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Comando discreto: alternar relleno/wireframe.
                glPolygonMode(GL_FRONT_AND_BACK,
                              ctx.wireframe ? GL_LINE : GL_FILL);

                shader.use();
                shader.set_uniform(loc_projection, camara.data().projection);
                shader.set_uniform(loc_view, camara.data().view);

                // LISTA DE DIBUJO: cada módulo arma su parte y la agrega como
                // RenderItem(s). Un RenderItem es una DESCRIPCIÓN de qué dibujar:
                //   mesh  -> la malla en la GPU (VAO/VBO/EBO), subida en load()
                //   model -> la matriz de modelo, calculada en CPU por update()
                //   color -> el color de la pieza
                // El main NO conoce ni a Aircraft ni a Terrain: solo recorre la
                // lista (no sabe cuántas piezas hay ni cómo están armadas).
                std::vector<RenderItem> items;
                avion.collect(items);      // agrega 7 items (piezas del avión)
                terreno.collect(items);    // agrega 1 item  (el terreno)

                // Se consume la lista: por item se suben los uniforms (color +
                // matriz de modelo) y se emite UN draw call contra su malla.
                // La geometría dibujada es la de la GPU (vao/count), NO datos de CPU.
                for (const RenderItem& item : items) {
                    shader.set_uniform(loc_color, item.color);   // color (uniform)
                    shader.set_uniform(loc_model, item.model);   // matriz de modelo (CPU)
                    glBindVertexArray(item.mesh->vao());         // malla de la GPU
                    glDrawElements(GL_TRIANGLES, item.mesh->count(),  // draw call
                                   GL_UNSIGNED_INT, nullptr);
                }
                glBindVertexArray(0);

                glfwSwapBuffers(window);
            }
        }
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
    glViewport(0, 0, width, height);   // 1) el rectángulo de píxeles donde se dibuja

    // 2) Recalcular la proyección. La función es libre: recupera el contexto
    // que main dejó en el user pointer de la ventana.
    AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx != nullptr && ctx->camara != nullptr) {
        ctx->camara->set_viewport(width, height);
    }
}

// -----------------------------------------------------------------------------
// key_callback — comandos DISCRETOS (eventos), Práctico 06
// -----------------------------------------------------------------------------
// GLFW llama a esta función en cada evento de teclado y YA entrega el flanco:
// distingue GLFW_PRESS de GLFW_REPEAT. Por eso se filtra solo PRESS (si no, un
// toggle alternaría decenas de veces por segundo mientras se mantiene la tecla).
void key_callback(GLFWwindow* window, int key, [[maybe_unused]] int scancode,
                  int action, [[maybe_unused]] int mods){
    if (action != GLFW_PRESS) {
        return;
    }
    AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx == nullptr) {
        return;
    }
    if (key == GLFW_KEY_SPACE) {
        ctx->paused = !ctx->paused;         // pausa/reanuda la simulación
    }
    if (key == GLFW_KEY_F) {
        ctx->wireframe = !ctx->wireframe;   // alterna relleno/wireframe
    }
}

// -----------------------------------------------------------------------------
// processInput — ESC para cerrar
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
