// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// terreno.fs — fragment shader del TERRENO (Prácticos 08 y 09)
//
// Con una TEXTURA el color deja de ser uno por objeto (o uno por vértice
// interpolado) y pasa a decidirse PUNTO POR PUNTO: cada fragmento lee la imagen
// en sus coordenadas (u, v) interpoladas. En el Práctico 08 la textura
// REEMPLAZA al color; en el Práctico 09 (Parte 2) pasa a ser el coeficiente kd
// de la luz.
//
// Práctico 09 (Parte 1): este archivo ya NO es el texto completo. El programa
// se arma con   #version + iluminacion.glsl + terreno.fs   (en ese orden), así
// que acá se pueden usar los uniforms, el struct Material y las funciones
// iluminar() y aplicar_niebla() del código común sin declararlos. La niebla
// que antes estaba repetida acá se borró: es la de iluminacion.glsl.
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de terreno.vs, INTERPOLADAS por la GPU para este
// fragmento (mismo nombre y tipo que los out del vertex shader).
in vec3  vNormal;      // normal interpolada (sin usar hasta la luz del P09, Parte 2)
in vec2  vTexCoord;    // coordenadas de textura, en teselas
in vec3  vPosMundo;    // posición en mundo (niebla; desde la Parte 2, también la luz)

// SALIDA (out): el color final de este píxel (R, G, B, A en [0, 1]).
out vec4 FragColor;

// El sampler lee la UNIDAD de textura 0 (layout binding): del lado de C++
// alcanza con textura.bind(0) antes de dibujar, sin setear este uniform. Su
// valor es el NÚMERO DE UNIDAD, no el identificador de la textura.
layout (binding = 0) uniform sampler2D uTextura;

uniform vec3 uColor;         // color plano (depuración: colores por nivel, tecla L)
uniform int  uUsarTextura;   // 1 = textura (pasto), 0 = color plano

// La niebla (uNieblaColor, uNieblaInicio, uNieblaFin y aplicar_niebla) viene de
// iluminacion.glsl: es la MISMA que usan el avión y el suelo continuado.

void main()
{
    // La textura se lee SIEMPRE (también con color plano) y recién después se
    // elige: el filtrado con mipmaps necesita comparar las coordenadas de
    // fragmentos vecinos, y eso no está definido si la lectura queda adentro de
    // un if que algunos fragmentos toman y otros no.
    vec3 texel = texture(uTextura, vTexCoord).rgb;              // color del pasto en este punto
    vec3 color = (uUsarTextura == 1) ? texel : uColor;          // el color elegido, sin niebla

    FragColor = vec4(aplicar_niebla(color, vPosMundo), 1.0);    // código común
}
