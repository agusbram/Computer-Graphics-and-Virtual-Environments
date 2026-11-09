// -----------------------------------------------------------------------------
// main.cpp — la aplicación (Prácticos 04 a 08)
//
// Lo nuevo del Práctico 08 (texturas y cielo):
//   1. TEXTURAS: ResourceManager decodifica las imágenes (stb_image) y la clase
//      Texture las sube a la GPU (DSA, 4 etapas, mipmaps). Las texturas las
//      POSEE main; los RenderItem solo las apuntan (puntero no dueño).
//   2. TRES PROGRAMAS de shaders, uno por tipo de objeto: terreno (morph del
//      LOD + pasto), aeronave (color, chapa o imagen de prueba) y fondo (cielo +
//      suelo continuado, dentro del módulo Skybox).
//   3. EL FONDO (Skybox) se dibuja AL ÚLTIMO (método b: z = w, GL_LEQUAL).
//   4. Teclas de verificación: U (imagen de prueba), T (apariencia del avión:
//      camuflaje / metal gris / colores),
//      I (mipmaps: con / sin / trampa), K (cielo cubemap / degradé), G (suelo
//      continuado), 1 (solo el nivel fino del LOD), RePág/AvPág (altura sobre
//      el terreno, para medir la banda del problema de la altitud).
//
// Historia (Práctico 06): FDM, game loop e input.
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
#include <array>            // std::array (las 6 caras del cubemap)
#include <cmath>            // std::atan, std::floor
#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>        // std::exception
#include <iostream>         // std::cout, std::endl, std::cerr
#include <set>              // std::set (para contar mallas distintas)
#include <string>           // std::string
#include <utility>          // std::pair
#include <vector>           // std::vector

#include <glm/glm.hpp>                         // glm::vec3, mat4, inverse, degrees: posiciones, matrices y ángulos

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

// dlfdm: modelo de dinámica de vuelo (Práctico 06)
#include <dlfdm/defines.h>                     // tipos del FDM: AircraftState, AircraftParameters, TrimPoint
#include <dlfdm/fdmsolver.h>                   // dlfdm::FDMSolver: integra el movimiento del avión
#include <dlfdm/models/aircraft/jettrainer.h>  // dlfdm::jettrainer: los datos del avión (jet de entrenamiento)

#include "core/Aircraft.h"                     // Aircraft: el avión armado con primitivas
#include "core/CameraSystem.h"                 // CameraSystem: la cámara que sigue al avión
#include "core/FlightData.h"                   // FlightData y to_world: pasar el estado del FDM a la escena
#include "core/InputHandler.h"                 // InputHandler: teclado (mandos) y mouse (cámara)
#include "core/ResourceManager.h"              // ResourceManager: carga shaders e imágenes desde assets/
#include "core/Shader.h"                       // Shader: los programas "terreno" y "aeronave"
#include "core/Skybox.h"                       // Skybox: el fondo (cielo + suelo continuado)
#include "core/Terrain.h"                      // Terrain: el terreno con LOD (geometry clipmaps)
#include "core/Texture.h"                      // Texture: pasto, imagen de prueba, chapa y camuflaje

// Datos básicos de la ventana
static const char* kWindowTitle     = "Práctico 08 - Texturas y cielo"; // Título
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

// Práctico 08: TAMAÑO DE UNA TESELA del terreno (una copia de la textura del
// pasto), en unidades de escena: 5 u = 50 m (la referencia de la guía: 40
// teselas sobre 2 km). Tiene que dividir EXACTO a Terrain::kPeriodoUV (5120 u):
// 5120 / 5 = 1024 teselas por período. Con un pasto de 256 texels, cada texel
// cubre 50 m / 256 ≈ 0,2 m.
static constexpr float kTamTesela = 5.0f;

// Práctico 08: cuánto sube/baja el avión respecto del terreno con RePág/AvPág
// (para medir la "banda" del problema de la altitud sin tener que pilotear).
static constexpr float kPasoAltura = 10.0f;   // u (= 100 m) por pulsación

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
    bool colores_niveles = false;   // L: un color por nivel del clipmap
    bool morph = true;              // M: geomorphing de los clipmaps (etapa 2)
    bool niebla = true;             // N: niebla on/off (P07 mejora leve; se mejora en P09)
    // --- Práctico 08 ---
    bool  textura_prueba = false;   // U: imagen de prueba en el avión y el terreno (Parte 3)
    int   apariencia_avion = 0;     // T: 0 camuflaje, 1 metal gris, 2 colores de las piezas (P04)
    int   modo_mipmap = 0;          // I: 0 con mipmaps, 1 sin mipmaps, 2 "la trampa"
    bool  cielo_degrade = false;    // K: cielo con cubemap (false) o degradé 1D (true)
    bool  suelo_continuado = true;  // G: suelo continuado en el fondo (Parte 5)
    bool  solo_nivel_fino = false;  // 1: solo el nivel fino del LOD (ventana como el P07)
    float altura_extra = 0.0f;      // RePág/AvPág: u que se BAJA el terreno (= sube el avión)
};

// Práctico 08: ubicaciones de los uniforms "por pieza" de un programa. Se buscan
// UNA vez (con Shader::loc) y se usan en cada draw call. Las que un programa no
// tiene quedan en -1: glProgramUniform* con ubicación -1 no hace nada.
struct UniformsPieza {
    int modelo = -1, color = -1, usar_textura = -1, escala_uv = -1, morph = -1;
};

// Práctico 08: uniforms de ESCENA de un programa (cámara y niebla): iguales para
// todas las piezas, se suben una vez por cuadro. Con glProgramUniform* cada
// programa tiene SUS PROPIOS uniforms aunque se llamen igual, por eso se buscan
// y se envían por separado a cada uno.
struct UniformsEscena {
    int proyeccion = -1, vista = -1, niebla_color = -1, niebla_inicio = -1, niebla_fin = -1;
};

static UniformsEscena buscar_uniforms_escena(const Shader& shader)
{
    UniformsEscena u;
    u.proyeccion    = shader.loc("uProjection");
    u.vista         = shader.loc("uView");
    u.niebla_color  = shader.loc("uNieblaColor");
    u.niebla_inicio = shader.loc("uNieblaInicio");
    u.niebla_fin    = shader.loc("uNieblaFin");
    return u;
}

// Práctico 08: dibuja una lista de RenderItem con un programa. Por cada item
// sube sus uniforms, conecta su textura (si tiene) a la unidad 0 y emite UN
// draw call. Es el bucle que antes estaba en main, ahora reutilizado por los
// dos programas (terreno y aeronave).
static void dibujar_items(const Shader& shader, const UniformsPieza& u,
                          const std::vector<RenderItem>& items)
{
    shader.use();
    for (const RenderItem& item : items) {
        shader.set_uniform(u.modelo, item.model);
        shader.set_uniform(u.color, item.color);
        shader.set_uniform(u.morph, item.morph);
        shader.set_uniform(u.escala_uv, item.escala_uv);
        if (item.textura != nullptr) {
            item.textura->bind(0);                    // unidad 0 = layout(binding = 0)
            shader.set_uniform(u.usar_textura, 1);
        } else {
            shader.set_uniform(u.usar_textura, 0);    // color plano
        }
        glBindVertexArray(item.mesh->vao());
        glDrawElements(GL_TRIANGLES, item.mesh->count(), GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

// Módulo positivo en double (para acotar coordenadas de mundo grandes).
// Resto de x / periodo, SIEMPRE en [0, periodo), también para x negativos
// (std::fmod devolvería un resto negativo). Ejemplo con periodo 5120:
// -1000 -> floor(-0.20) = -1 -> -1000 + 5120 = 4120. Es la MISMA cuenta que la
// lambda `modulo` de Terrain::recalcular_vertices (explicada ahí en detalle):
// malla y suelo continuado tienen que usar el mismo origen de las (u, v).
static double modulo_positivo(double x, double periodo)
{
    return x - periodo * std::floor(x / periodo);
}

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
        // Práctico 08: un programa por tipo de objeto (las guías 08 y 09
        // proponen shaders separados para el terreno, el fondo y la aeronave).
        const ShaderSource& src_terreno = resources.load_shader_source(
            "terreno", "shaders/terreno.vs", "shaders/terreno.fs");
        const ShaderSource& src_aeronave = resources.load_shader_source(
            "aeronave", "shaders/aeronave.vs", "shaders/aeronave.fs");
        const ShaderSource& src_fondo = resources.load_shader_source(
            "fondo", "shaders/fondo.vs", "shaders/fondo.fs");

        {
            Shader shader_terreno;
            Shader shader_aeronave;
            if (!shader_terreno.compile_from_source(src_terreno.vs, src_terreno.fs) ||
                !shader_aeronave.compile_from_source(src_aeronave.vs, src_aeronave.fs)) {
                std::cout << "Un programa de shaders quedó vacío; saliendo sin "
                             "dibujar." << std::endl;
                glfwDestroyWindow(window);
                glfwTerminate();
                return EXIT_FAILURE;
            }

            // Uniforms de cada programa (se buscan una vez). Los de la cámara y
            // la niebla son de la ESCENA (iguales para todas las piezas); los de
            // UniformsPieza cambian en cada draw call.
            UniformsPieza u_terreno;
            u_terreno.modelo       = shader_terreno.loc("uModel");
            u_terreno.color        = shader_terreno.loc("uColor");
            u_terreno.usar_textura = shader_terreno.loc("uUsarTextura");
            u_terreno.morph        = shader_terreno.loc("uMorph");      // LOD (P07)
            UniformsPieza u_aeronave;
            u_aeronave.modelo       = shader_aeronave.loc("uModel");
            u_aeronave.color        = shader_aeronave.loc("uColor");
            u_aeronave.usar_textura = shader_aeronave.loc("uUsarTextura");
            u_aeronave.escala_uv    = shader_aeronave.loc("uEscalaUV");
            const UniformsEscena ue_terreno  = buscar_uniforms_escena(shader_terreno);
            const UniformsEscena ue_aeronave = buscar_uniforms_escena(shader_aeronave);
            const int loc_tam_tesela = shader_terreno.loc("uTamTesela");

            // Niebla lineal (mejora leve del P07): empieza a 300 u (3 km) y es
            // total a 1800 u (18 km), antes del borde del clipmap (2560 u) y del
            // far (3000 u). Su COLOR, desde el P08, es el del horizonte del
            // cielo (lo calcula el Skybox al cargar el cubemap, más abajo).
            const float kNieblaInicio = 300.0f;
            const float kNieblaFin    = 1800.0f;

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
            // Práctico 07, LOD: GEOMETRY CLIPMAPS (etapa 1). 5 niveles de 128x128
            // celdas; el fino con paso 2.5 u (25 m) y cada nivel el doble ->
            // el más grueso cubre 128·2.5·16 = 5120 u (51 km de lado).
            Terrain terreno(128U, 2.5f, 5U, 20.0f, altura_terreno, 1337, 0.005f);
            terreno.init();

            std::cout << "terreno  : clipmap de " << terreno.niveles() << " niveles de "
                      << terreno.celdas() << "x" << terreno.celdas()
                      << " celdas, paso fino " << terreno.paso()
                      << " u; lado total " << terreno.extension()
                      << " u = " << terreno.extension() * kMetrosPorUnidad
                      << " m; el avion vuela a " << kDespeje << " u del suelo"
                      << std::endl;

            // --- Texturas (Práctico 08) -------------------------------------
            // 1) ResourceManager LEE y DECODIFICA (C++ puro). Las texturas 2D se
            //    cargan INVERTIDAS (flip = true): el archivo guarda primero la
            //    fila de arriba y OpenGL pone el (0,0) abajo. Las caras del
            //    cubemap NO se invierten (su convención tiene el origen arriba).
            const ImageData& img_pasto  = resources.load_image(
                "pasto", "textures/suelo/grass.png", true);
            const ImageData& img_prueba = resources.load_image(
                "prueba_uv", "textures/prueba/prueba_uv.png", true);
            const ImageData& img_chapa  = resources.load_image(
                "chapa", "textures/avion/chapa_color.jpg", true);
            const ImageData& img_camuflaje = resources.load_image(
                "camuflaje", "textures/avion/camuflaje.png", true);
            // Caras del cubemap en el orden de OpenGL: +X -X +Y -Y +Z -Z.
            // ACÁ es donde se cargan las 6 imágenes del cielo (antes de esta
            // línea no se leyó ningún archivo de textures/cielo/). load_image
            // recibe TRES cosas, igual que en las llamadas de arriba:
            //   1. la CLAVE: el nombre con el que el ResourceManager la guarda
            //      en su caché, para pedirla o liberarla después. Se arma como
            //      "cielo_" + nombre de la cara: "cielo_right", "cielo_left", ...
            //      El prefijo "cielo_" evita choques con otras claves (una cara
            //      llamada "top" o "back" es un nombre muy genérico).
            //   2. el ARCHIVO, relativo a assets/: "textures/cielo/right.png", ...
            //   3. flip = false: las caras del cubemap NO se invierten.
            // load_image devuelve una REFERENCIA a la imagen guardada en el
            // caché; se guarda su DIRECCIÓN (&) en el arreglo `caras`, que es lo
            // que recibe Skybox::init. Con la misma clave se liberan más abajo
            // (unload_image), una vez que el cubemap ya está en la GPU.
            const char* kCaras[6] = {"right", "left", "top", "bottom", "front", "back"};
            std::array<const ImageData*, 6> caras {};
            for (int i = 0; i < 6; ++i) {
                caras[static_cast<std::size_t>(i)] = &resources.load_image(
                    std::string("cielo_") + kCaras[i],
                    std::string("textures/cielo/") + kCaras[i] + ".png", false);
            }

            // 2) Texture las SUBE a la GPU. main es el DUEÑO de las texturas 2D;
            //    los RenderItem y el fondo solo las apuntan.
            Texture tex_pasto;            // trilineal + GL_REPEAT (Muestreo por omisión)
            tex_pasto.load_2d(img_pasto);
            Texture tex_pasto_trampa;     // "la trampa": niveles reservados SIN generar
            // load_2d(imagen, muestreo = Muestreo{}, generar_mipmaps = true,
            //         srgb = false): los tres últimos tienen valor por omisión.
            // Acá solo interesa cambiar el TERCERO (generar_mipmaps = false),
            // pero en C++ los argumentos van por POSICIÓN: para llegar al
            // tercero hay que escribir también el segundo. Muestreo{} es el
            // mismo valor por omisión (trilineal + repetición), escrito a mano;
            // no cambia nada respecto de tex_pasto. srgb queda en false.
            tex_pasto_trampa.load_2d(img_pasto, Muestreo{}, false);
            Texture tex_prueba;
            tex_prueba.load_2d(img_prueba);
            Texture tex_chapa;            // chapa CC0 (tiñe el gris del modo "metal gris")
            tex_chapa.load_2d(img_chapa);
            Texture tex_camuflaje;        // camuflaje (tools/generar_camuflaje.py)
            tex_camuflaje.load_2d(img_camuflaje);

            // 3) El fondo: cubo + cubemap + degradé + su programa. Calcula el
            //    color del horizonte y la dirección del sol MIENTRAS las caras
            //    siguen en memoria.
            Skybox fondo;
            fondo.init(src_fondo, caras);

            std::cout << "texturas : pasto " << tex_pasto.ancho() << "x" << tex_pasto.alto()
                      << " (" << tex_pasto.niveles() << " niveles de mipmap), tesela "
                      << kTamTesela * kMetrosPorUnidad << " m; prueba "
                      << tex_prueba.ancho() << "x" << tex_prueba.alto() << "; chapa "
                      << tex_chapa.ancho() << "x" << tex_chapa.alto()
                      << " (brillo medio " << tex_chapa.brillo_medio() << "); camuflaje "
                      << tex_camuflaje.ancho() << "x" << tex_camuflaje.alto() << std::endl;
            {
                const glm::vec3 h = fondo.color_horizonte();
                const glm::vec3 s = fondo.direccion_sol();
                std::cout << "cielo    : cubemap " << caras[0]->width << "x" << caras[0]->height
                          << " x 6; horizonte (niebla) = (" << h.r << ", " << h.g << ", "
                          << h.b << "); sol pintado en (" << s.x << ", " << s.y << ", "
                          << s.z << "), elevacion " << fondo.elevacion_sol_grados()
                          << " grados" << std::endl;
            }

            // 4) Decisión 1: la GPU ya tiene su copia -> se libera la de la CPU
            //    (el cubemap solo son ~19 MB en RGBA). Las referencias img_* y
            //    caras quedan inválidas desde acá (no se vuelven a usar).
            for (const char* clave : {"pasto", "prueba_uv", "chapa", "camuflaje"}) {
                resources.unload_image(clave);
            }
            for (int i = 0; i < 6; ++i) {
                resources.unload_image(std::string("cielo_") + kCaras[i]);
            }

            std::cout << "aeronave : " << "escena compuesta por primitivas"
                      << std::endl;
            std::cout << "FDM      : jet trainer (Roskam 'C'), trim a 5000 m / "
                      << "150 m/s; dt = " << kFdmDt << " s" << std::endl;
            std::cout << "teclas   : flechas=cabeceo/alerones, Q/E=timon, "
                      << "W/S=potencia, ESPACIO=pausa, F=wireframe, "
                      << "L=colores por nivel, "
                      << "M=geomorphing on/off, N=niebla on/off" << std::endl;
            std::cout << "teclas P08: U=imagen de prueba UV, T=apariencia del avion, "
                      << "I=mipmaps (con/sin/trampa), K=cielo cubemap/degrade, "
                      << "G=suelo continuado, 1=solo nivel fino, "
                      << "RePag/AvPag=altura sobre el terreno" << std::endl;
            {
                GLint muestras = 0;
                glGetIntegerv(GL_SAMPLES, &muestras);
                std::cout << "MSAA     : " << muestras
                          << " muestras/pixel (antialiasing)" << std::endl;
            }

            // --- Cámara orbital (Práctico 05) + input ----------------------
            CameraSystem camara(kWindowWidth, kWindowHeight);

            // Contexto para las callbacks (cámara + toggles discretos).
            // ctx es el "tablero de interruptores" del programa: guarda el
            // estado de cada tecla de depuración (pausa, F, L, M, N, U, T, I,
            // K, G, 1, RePág/AvPág) y un puntero a la cámara.
            // El problema que resuelve: GLFW llama a key_callback y a
            // framebuffer_size_callback con SOLO la ventana y la tecla; son
            // funciones libres y no ven las variables de main. La solución de
            // GLFW: "colgar" un puntero propio de la ventana
            // (glfwSetWindowUserPointer) y recuperarlo dentro de la callback
            // (glfwGetWindowUserPointer). El circuito:
            //   1. el usuario aprieta una tecla -> GLFW llama a key_callback;
            //   2. key_callback recupera ctx y cambia el campo (por ejemplo
            //      ctx->niebla = !ctx->niebla);
            //   3. el bucle principal lee ctx en cada cuadro y actúa (manda la
            //      niebla al shader, cambia la textura, etc.).
            // Vive en main (pila): existe mientras la ventana existe.
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
                                  << "  sobre el terreno="
                                  << (flight.position.z - terreno.altura_terreno()) * kMetrosPorUnidad
                                  << " m" << std::endl;
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

                // --- Toggles de depuración -> módulos (Prácticos 07 y 08) ----
                terreno.set_colores_niveles(ctx.colores_niveles);
                terreno.set_morph(ctx.morph);
                terreno.set_solo_nivel_fino(ctx.solo_nivel_fino);
                // RePág/AvPág: el terreno se BAJA (o sube) respecto de su altitud
                // original: es como si el avión volara más alto, sin pilotear.
                terreno.set_altura_terreno(altura_terreno - ctx.altura_extra);

                // Textura del suelo según la tecla I (mipmaps) o U (prueba):
                //   I = 0: pasto con trilineal (lo correcto)
                //   I = 1: pasto SIN mipmaps (reducción GL_LINEAR): hacia el
                //          horizonte la textura titila y aparece moiré
                //   I = 2: "la trampa": niveles reservados pero NO generados
                //          (contenido indefinido; OpenGL no da ningún error)
                {
                    static int modo_aplicado = -1;   // para no re-parametrizar cada cuadro
                    if (ctx.modo_mipmap != modo_aplicado) {
                        Muestreo m;                  // trilineal + repetición
                        if (ctx.modo_mipmap == 1) {
                            m.reduccion = Filtro::Lineal;
                        }
                        tex_pasto.set_muestreo(m);
                        modo_aplicado = ctx.modo_mipmap;
                    }
                }
                const Texture* textura_suelo =
                    ctx.textura_prueba        ? &tex_prueba
                    : (ctx.modo_mipmap == 2)  ? &tex_pasto_trampa
                                              : &tex_pasto;
                terreno.set_textura(textura_suelo);

                // Avión: la imagen de prueba (U) tiene prioridad; si no, la
                // apariencia elegida con T (camuflaje / metal gris / colores).
                if (ctx.textura_prueba) {
                    avion.set_apariencia(AparienciaAvion::PruebaUV, &tex_prueba);
                } else if (ctx.apariencia_avion == 0) {
                    avion.set_apariencia(AparienciaAvion::Camuflaje, &tex_camuflaje);
                } else if (ctx.apariencia_avion == 1) {
                    avion.set_apariencia(AparienciaAvion::MetalGris, &tex_chapa);
                } else {
                    avion.set_apariencia(AparienciaAvion::ColoresPiezas, nullptr);
                }
                fondo.set_modo(ctx.cielo_degrade ? ModoCielo::Degrade : ModoCielo::Cubemap);

                terreno.update(pos_avion);                     // ventana infinita (LOD)

                flight.position = glm::vec3(0.0f);             // avión al origen

                // El avión se ubica en la escena y la cámara lo orbita (objetivo
                // en el origen, donde quedó el avión).
                avion.update(flight);
                camara.update(glm::vec3(0.0f),
                              glm::vec3(flight.theta, flight.phi, flight.psi),
                              input.camera_cmd());

                // Práctico 08: posición del OJO. La vista lleva el mundo al
                // sistema de la cámara; su INVERSA lleva el origen de la cámara
                // (el ojo) al mundo: la 4.ª columna de inverse(vista) es el ojo.
                // Está en coordenadas RELATIVAS al avión (origen flotante); el
                // ojo en el mundo real es pos_avion + ojo_rel.
                const glm::vec3 ojo_rel = glm::vec3(glm::inverse(camara.data().view)[3]);
                const glm::vec3 ojo_mundo = pos_avion + ojo_rel;

                // Altura sobre el terreno: se informa al cambiarla (RePág/AvPág)
                // junto con la banda TEÓRICA, atan(h / (L/2)), para el clipmap
                // completo y para el nivel fino solo (la tabla de la Parte 5).
                {
                    static float extra_informado = -1.0f;
                    if (ctx.altura_extra != extra_informado) {
                        extra_informado = ctx.altura_extra;
                        const float h = ojo_mundo.z - terreno.altura_terreno();   // u
                        const float banda_lod = glm::degrees(std::atan(h / (0.5f * terreno.extension())));
                        const float banda_fino = glm::degrees(std::atan(h / (0.5f * terreno.extension_nivel_fino())));
                        std::cout << "[altura] ojo a " << h * kMetrosPorUnidad
                                  << " m sobre el terreno -> banda teorica: clipmap "
                                  << banda_lod << " grados, solo nivel fino "
                                  << banda_fino << " grados" << std::endl;
                    }
                }

                // Color de la NIEBLA = horizonte del cielo (el mismo para el
                // cubemap y el degradé). El clear usa el mismo color, aunque el
                // fondo tapa toda la pantalla.
                const glm::vec3 color_niebla = fondo.color_horizonte();
                const float niebla_fin = ctx.niebla ? kNieblaFin : 0.0f;

                // niebla_fin = 0 es la señal de "niebla apagada" (tecla N): los
                // shaders no aplican niebla si uNieblaFin <= 0.
                //
                // Borrar la pantalla al empezar el cuadro, en dos pasos:
                //   glClearColor: solo ELIGE el color de borrado (R, G, B, A en
                //     [0, 1]). No dibuja nada; queda guardado como estado.
                //   glClear: BORRA de verdad lo que se le pide:
                //     GL_COLOR_BUFFER_BIT -> toda la imagen queda de ese color;
                //     GL_DEPTH_BUFFER_BIT -> el z-buffer vuelve a 1 (lo más
                //       lejos) en todos los píxeles. Esto es IMPRESCINDIBLE: si
                //       no, quedan las profundidades del cuadro anterior y nada
                //       se dibujaría (y el fondo con GL_LEQUAL tampoco pasaría).
                // El color de borrado casi no se ve: el fondo (Skybox) pinta
                // TODA la pantalla. Se usa el del horizonte como RED DE
                // SEGURIDAD: si el fondo no se dibujara (o algún píxel quedara
                // sin cubrir), se vería el color del cielo y no un negro.
                glClearColor(color_niebla.r, color_niebla.g, color_niebla.b, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Comando discreto: alternar relleno/wireframe.
                glPolygonMode(GL_FRONT_AND_BACK,
                              ctx.wireframe ? GL_LINE : GL_FILL);

                // Uniforms de ESCENA (iguales para todas las piezas), por
                // programa: con glProgramUniform* cada programa tiene los suyos.
                const std::pair<const Shader*, const UniformsEscena*> programas[] = {
                    {&shader_terreno, &ue_terreno}, {&shader_aeronave, &ue_aeronave}};
                for (const auto& [sh, ue] : programas) {
                    sh->set_uniform(ue->proyeccion, camara.data().projection);
                    sh->set_uniform(ue->vista, camara.data().view);
                    sh->set_uniform(ue->niebla_color, color_niebla);
                    sh->set_uniform(ue->niebla_inicio, kNieblaInicio);
                    sh->set_uniform(ue->niebla_fin, niebla_fin);
                }
                shader_terreno.set_uniform(loc_tam_tesela, kTamTesela);

                // LISTAS DE DIBUJO: cada módulo arma su parte como RenderItem(s)
                // (malla + modelo + color + textura). El main no sabe cuántas
                // piezas hay; solo elige CON QUÉ PROGRAMA se dibuja cada lista.
                std::vector<RenderItem> items_terreno;
                std::vector<RenderItem> items_avion;
                terreno.collect(items_terreno);   // 1 item por nivel del LOD
                avion.collect(items_avion);       // 7 items (piezas del avión)

                dibujar_items(shader_terreno, u_terreno, items_terreno);
                dibujar_items(shader_aeronave, u_aeronave, items_avion);

                // EL FONDO, AL ÚLTIMO (método b): solo se pinta donde la escena
                // no dibujó nada. Usa la MISMA textura de suelo que la malla, la
                // misma escala y el mismo origen de (u, v) -> sin costura.
                ParametrosFondo pf;
                pf.vista       = camara.data().view;
                pf.proyeccion  = camara.data().projection;
                pf.ojo_uv      = glm::vec2(
                    static_cast<float>(modulo_positivo(ojo_mundo.x, Terrain::kPeriodoUV)),
                    static_cast<float>(modulo_positivo(ojo_mundo.y, Terrain::kPeriodoUV)));
                pf.altura_ojo  = ojo_mundo.z - terreno.altura_terreno();
                pf.tam_tesela  = kTamTesela;
                pf.suelo       = textura_suelo;
                pf.suelo_continuado = ctx.suelo_continuado;
                pf.niebla_color  = color_niebla;
                pf.niebla_inicio = kNieblaInicio;
                pf.niebla_fin    = niebla_fin;
                fondo.draw(pf);

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
    AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx == nullptr) {
        return;
    }
    // Práctico 08: RePág/AvPág aceptan también REPEAT (mantener apretada la
    // tecla sigue subiendo/bajando): no es un toggle, es un ajuste por pasos.
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_PAGE_UP) {
            ctx->altura_extra += kPasoAltura;                   // más alto
        }
        if (key == GLFW_KEY_PAGE_DOWN) {
            ctx->altura_extra = std::max(0.0f, ctx->altura_extra - kPasoAltura);
        }
    }
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_SPACE) {
        ctx->paused = !ctx->paused;         // pausa/reanuda la simulación
    }
    if (key == GLFW_KEY_F) {
        ctx->wireframe = !ctx->wireframe;   // alterna relleno/wireframe
    }
    if (key == GLFW_KEY_N) {
        ctx->niebla = !ctx->niebla;         // niebla on/off (P07 mejora leve; P09)
    }
    if (key == GLFW_KEY_M) {
        ctx->morph = !ctx->morph;           // geomorphing on/off (Práctico 07, LOD)
    }
    if (key == GLFW_KEY_L) {
        ctx->colores_niveles = !ctx->colores_niveles;   // un color por nivel
    }

    // --- Práctico 08: verificación de texturas y cielo -----------------------
    // Cada tecla informa por consola el estado nuevo (sirve para las capturas).
    if (key == GLFW_KEY_U) {
        ctx->textura_prueba = !ctx->textura_prueba;
        std::cout << "[U] imagen de prueba UV: " << (ctx->textura_prueba ? "SI" : "no") << std::endl;
    }
    if (key == GLFW_KEY_T) {
        ctx->apariencia_avion = (ctx->apariencia_avion + 1) % 3;
        const char* modos[3] = {"camuflaje", "metal gris (chapa)",
                                "colores de las piezas (Practico 04)"};
        std::cout << "[T] apariencia del avion: " << modos[ctx->apariencia_avion] << std::endl;
    }
    if (key == GLFW_KEY_I) {
        ctx->modo_mipmap = (ctx->modo_mipmap + 1) % 3;
        const char* modos[3] = {"CON mipmaps (trilineal)", "SIN mipmaps (GL_LINEAR)",
                                "TRAMPA: niveles reservados sin generar"};
        std::cout << "[I] filtrado del pasto: " << modos[ctx->modo_mipmap] << std::endl;
    }
    if (key == GLFW_KEY_K) {
        ctx->cielo_degrade = !ctx->cielo_degrade;
        std::cout << "[K] cielo: " << (ctx->cielo_degrade ? "degrade 1D" : "cubemap") << std::endl;
    }
    if (key == GLFW_KEY_G) {
        ctx->suelo_continuado = !ctx->suelo_continuado;
        std::cout << "[G] suelo continuado: " << (ctx->suelo_continuado ? "SI" : "no (se ve la banda)") << std::endl;
    }
    if (key == GLFW_KEY_1) {
        ctx->solo_nivel_fino = !ctx->solo_nivel_fino;
        std::cout << "[1] terreno: " << (ctx->solo_nivel_fino ? "SOLO el nivel fino (como el P07)"
                                                              : "clipmap completo") << std::endl;
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
