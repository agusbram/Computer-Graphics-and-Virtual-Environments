// -----------------------------------------------------------------------------
// main.cpp — Triángulo simple con OpenGL (GLFW + GLAD)
//
// Flujo general de la aplicación:
//   1. Inicializar GLFW y crear la ventana.
//   2. Cargar las funciones de OpenGL con GLAD.
//   3. Compilar y enlazar los shaders (vértice + fragmento).
//   4. Subir la geometría (triángulo) a la GPU con un VAO/VBO.
//   5. Bucle principal: procesar entrada, dibujar e intercambiar buffers.
//   6. Liberar recursos al cerrar la ventana.
// -----------------------------------------------------------------------------

#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <iostream>         // std::cout, std::endl, std::cerr
#include <string>           // std::string

#include <glad/gl.h>        // Funciones de OpenGL (cargador de extensiones)
#include <GLFW/glfw3.h>     // Gestión de ventana y contexto OpenGL

#include "core/ResourceManager.h" // Cache de recursos (fuentes de shaders)

// Datos básicos de la ventana
static const char* kWindowTitle     = "OpenGL template project"; // Título
static constexpr int kWindowWidth   = 800;  // Ancho inicial en píxeles
static constexpr int kWindowHeight  = 600;  // Alto inicial en píxeles
static constexpr int kGLVerMajor    = 4;    // Versión mayor de OpenGL pedida
static constexpr int kGLVerMinor    = 6;    // Versión menor de OpenGL pedida

// Ruta de los recursos del proyecto (relativa a donde se ejecuta el binario)
static const char* kAssetsRoot = "./assets";

// -----------------------------------------------------------------------------
// Los shaders ya no viven como strings acá: se leen desde archivos externos
// (assets/shaders/triangle.vs y .fs) mediante el ResourceManager. Así se
// pueden editar sin recompilar y escalar a muchos shaders distintos.
// -----------------------------------------------------------------------------

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
static unsigned int compile_shader(const std::string& source,
                                   unsigned int type);
static unsigned int create_shader_program(const ShaderSource& source);

int main()
{
    // Registrar el callback de errores ANTES de inicializar GLFW,
    // así nos enteramos de cualquier fallo que ocurra en glfwInit()
    glfwSetErrorCallback(error_callback);

    // ------------------------------------------------------------------
    // 1. Inicializar GLFW
    //    Devuelve GLFW_TRUE si todo salió bien; GLFW_FALSE en caso de error.
    // ------------------------------------------------------------------
    if (!glfwInit()) {
        const std::string error_msg = "GLFW initialization failed!";
        const std::string glfw_error_msg = std::to_string(glfw_error_code) +
                                           "): " + glfw_error_str;
        std::cout << error_msg + " - GLFW(" + glfw_error_msg << std::endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------
    // 2. Configurar el contexto de OpenGL que queremos:
    //    - Versión 4.6 (kGLVerMajor.kGLVerMinor)
    //    - Perfil core: solo funciones modernas (sin funciones obsoletas
    //      del modo inmediato como glBegin/glEnd).
    //    IMPORTANTE: los hints deben fijarse ANTES de crear la ventana.
    // ------------------------------------------------------------------
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLVerMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLVerMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ------------------------------------------------------------------
    // 3. Crear la ventana y su contexto OpenGL asociado.
    //    Parámetros: ancho, alto, título, monitor (nullptr = ventana),
    //    share (nullptr = no compartir recursos con otra ventana).
    // ------------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(kWindowWidth,
                                          kWindowHeight,
                                          kWindowTitle, nullptr, nullptr);

    if (window == nullptr){
        // Limpiar los recursos ya creados (GLFW) antes de salir
        glfwTerminate();

        const std::string error_msg = "GLFW window creation failed!";
        const std::string glfw_error_msg = std::to_string(glfw_error_code) +
                                           "): " + glfw_error_str;
        std::cout << error_msg + " - GLFW(" + glfw_error_msg << std::endl;

        return EXIT_FAILURE;
    }

    // Hacer que el contexto OpenGL de esta ventana sea el actual:
    // todas las llamadas a OpenGL a partir de aquí actúan sobre él.
    glfwMakeContextCurrent(window);

    // ------------------------------------------------------------------
    // 4. Cargar las funciones de OpenGL con GLAD.
    //    GLAD obtiene los punteros a las funciones usando el proc address
    //    de GLFW. Sin esto, no podríamos llamar a las funciones de OpenGL
    //    (glDrawArrays, glGenBuffers, etc.).
    // ------------------------------------------------------------------
    if (!gladLoadGL(glfwGetProcAddress)) {
        // Limpiar los recursos ya creados (ventana y GLFW)
        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "GLAD initialization failed!" << std::endl;

        return EXIT_FAILURE;
    }

    // Mostrar por consola la información del driver OpenGL instalado
    print_gl_version();

    // ------------------------------------------------------------------
    // 5. Registrar el callback de redimensionado de la ventana.
    //    Se llama cada vez que el usuario cambia el tamaño de la ventana
    //    para ajustar el viewport (la zona donde OpenGL dibuja).
    // ------------------------------------------------------------------
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ------------------------------------------------------------------
    // 6. Sincronización con el monitor (vsync):
    //    glfwSwapInterval(n) indica cuántos refrescos del monitor esperar
    //    antes de intercambiar los buffers:
    //      0 -> sin sincronizar (máxima velocidad, puede causar tearing)
    //      1 -> sincronizado con el refresco del monitor (recomendado)
    //      2 -> a la mitad del refresco del monitor
    // ------------------------------------------------------------------
    glfwSwapInterval(1);

    // ------------------------------------------------------------------
    // 7. Cargar los fuentes de los shaders desde disco con el
    //    ResourceManager y compilarlos/enlazarlos.
    //    Si algo falla (archivo faltante, error de compilación o enlace)
    //    se informa y se sale limpiamente.
    // ------------------------------------------------------------------
    unsigned int shader_program = 0;
    try {
        ResourceManager resources(kAssetsRoot);
        const ShaderSource& triangle_src = resources.load_shader_source(
            "triangle", "shaders/triangle.vs", "shaders/triangle.fs");
        shader_program = create_shader_program(triangle_src);
    } catch (const std::exception& e) {
        std::cerr << "Error cargando shaders: " << e.what() << std::endl;
    }
    if (shader_program == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------------
    // 8. Definir la geometría del triángulo.
    //    Tres vértices en coordenadas normalizadas de dispositivo (NDC):
    //    x e y van de -1.0 a 1.0, z de -1.0 a 1.0 (aquí z = 0).
    //      (-0.5, -0.5): esquina inferior izquierda
    //      ( 0.5, -0.5): esquina inferior derecha
    //      ( 0.0,  0.5): vértice superior
    // ------------------------------------------------------------------
    const float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
     // Rotura 3: Triangulo se ve cortado saliendo afuera de la pantalla  
     //  0.0f, 1.5f, 0.0f
    };

    // ------------------------------------------------------------------
    // 9. Subir la geometría a la GPU:
    //    - VAO (Vertex Array Object): guarda la configuración de los
    //      atributos de vértice (cómo interpretar los datos del VBO).
    //    - VBO (Vertex Buffer Object): bloque de memoria en la GPU que
    //      contiene los datos de los vértices.
    // ------------------------------------------------------------------
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao); // Crear 1 VAO y guardar su ID en vao
    glGenBuffers(1, &vbo);      // Crear 1 VBO y guardar su ID en vbo

    // A partir de aquí configuramos el VAO
    glBindVertexArray(vao);     // Activar el VAO (toda config. posterior
                                // queda guardada dentro de él)

    glBindBuffer(GL_ARRAY_BUFFER, vbo); // Activar el VBO como buffer
                                        // de tipo GL_ARRAY_BUFFER
    // Copiar los datos de los vértices del CPU a la GPU.
    // GL_STATIC_DRAW: los datos se envían una vez y se dibujan muchas veces.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Definir cómo interpretar los datos del VBO:
    //  - índice 0: corresponde al atributo "location = 0" del shader (aPos)
    //  - 3: cada vértice tiene 3 componentes (x, y, z)
    //  - GL_FLOAT: cada componente es un float
    //  - GL_FALSE: no normalizar los datos
    //  - 3 * sizeof(float): separación (stride) entre vértice y vértice
    //  - (void*)0: los datos empiezan en el byte 0 del buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);

    // ROTURA 1 (clinica): atributo apagado --> Se ve el fondo gris claro sin el triangulo pintado
    // y sin ningun mensaje de error. Es una falla silenciosa que unicamente se puede detectar 
    // a mano leyendo el codigo.
    glEnableVertexAttribArray(0); // Activar el atributo 0

    // Desenlazar (desactivar) para no modificar por accidente;
    // se volverán a enlazar al momento de dibujar
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------
    // 10. Bucle principal de renderizado.
    //     Se repite mientras el usuario no cierre la ventana.
    // ------------------------------------------------------------------
    while(!glfwWindowShouldClose(window)){
        // a) Procesar entrada del teclado/mouse (ESC cierra la ventana)
        processInput(window);

        // b) Comandos de dibujo:
        //    - Fijar el color con el que se limpia la pantalla (gris azulado)
        //    - Limpiar el buffer de color con ese color
        glClearColor(51.0f/256, 55.0f/256, 76.0f/256, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // c) Dibujar el triángulo:
        //    - Activar el programa de shaders a usar
        glUseProgram(shader_program);
        //    - Activar el VAO que contiene la configuración de vértices
        glBindVertexArray(vao);
        //    - Dibujar 3 vértices como triángulos, empezando en el 0
        glDrawArrays(GL_TRIANGLES, 0, 3);
        //    - Desactivar el VAO
        glBindVertexArray(0);

        // d) Intercambiar los buffers: muestra en pantalla lo dibujado
        //    (doble buffer: se dibuja en uno mientras se muestra el otro)
        glfwSwapBuffers(window);
        // e) Procesar los eventos pendientes (teclado, mouse, etc.)
        glfwPollEvents();
    }

    // ------------------------------------------------------------------
    // 11. Liberar los recursos de la GPU antes de cerrar
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &vao); // Borrar el VAO
    glDeleteBuffers(1, &vbo);      // Borrar el VBO
    glDeleteProgram(shader_program); // Borrar el programa de shaders

    // Limpiar los recursos de GLFW antes de salir
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

// -----------------------------------------------------------------------------
// compile_shader
// Crea un objeto shader en la GPU, le adjunta el código fuente y lo compila.
//  - source: código GLSL del shader (leído del disco por ResourceManager)
//  - type:   tipo de shader (GL_VERTEX_SHADER o GL_FRAGMENT_SHADER)
// Devuelve el ID del shader compilado, o 0 si hubo un error de compilación.
// -----------------------------------------------------------------------------
unsigned int compile_shader(const std::string& source, unsigned int type){
    // Crear el objeto shader del tipo indicado
    unsigned int shader = glCreateShader(type);
    // Adjuntar el código fuente (1 cadena) al shader
    const char* src_ptr = source.c_str();
    glShaderSource(shader, 1, &src_ptr, nullptr);
    // Compilar el shader
    glCompileShader(shader);

    // Comprobar si la compilación tuvo éxito
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Consultar el tamaño EXACTO del log (incluye el '\0' final) y
        // pedir un buffer de ese tamaño: un buffer fijo truncaría justo
        // los mensajes largos, que son los que más se necesitan.
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(static_cast<size_t>(log_length), '\0');
        glGetShaderInfoLog(shader, log_length, nullptr, info_log.data());

        std::cout << "Shader compilation failed!\n"
                  << info_log << std::endl;
        // Liberar el shader fallido y devolver 0 (error)
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// -----------------------------------------------------------------------------
// create_shader_program
// Compila el vertex y el fragment shader (fuentes leídos del disco por el
// ResourceManager), los adjunta a un programa y lo enlaza. Un programa
// enlazado es lo que se activa al dibujar.
// Devuelve el ID del programa, o 0 si hubo un error.
// -----------------------------------------------------------------------------
unsigned int create_shader_program(const ShaderSource& source){
    // Compilar ambos shaders
    unsigned int vertex_shader = compile_shader(source.vs, GL_VERTEX_SHADER);
    unsigned int fragment_shader = compile_shader(source.fs,
                                                  GL_FRAGMENT_SHADER);
    // Si alguno falló, no tiene sentido continuar
    if (vertex_shader == 0 || fragment_shader == 0) {
        return 0;
    }

    // Crear el programa (contenedor de shaders) y adjuntar los dos shaders
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    // Enlazar el programa: une las salidas del vertex shader con las
    // entradas del fragment shader y lo deja listo para usar
    glLinkProgram(program);

    // Comprobar si el enlace tuvo éxito
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        // Mismo criterio que con los shaders: log de tamaño dinámico
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(static_cast<size_t>(log_length), '\0');
        glGetProgramInfoLog(program, log_length, nullptr, info_log.data());

        std::cout << "Program linking failed!\n"
                  << info_log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    // Los shaders ya están dentro del programa: ya no se necesitan
    // como objetos individuales, se pueden eliminar
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}
