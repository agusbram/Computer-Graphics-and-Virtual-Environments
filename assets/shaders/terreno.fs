// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// terreno.fs — fragment shader del TERRENO (Prácticos 08 y 09)
//
// Con una TEXTURA el color deja de ser uno por objeto (o uno por vértice
// interpolado) y pasa a decidirse PUNTO POR PUNTO: cada fragmento lee la imagen
// en sus coordenadas (u, v) interpoladas. En el Práctico 08 la textura
// REEMPLAZABA al color; en el Práctico 09 (Parte 2) pasa a ser el coeficiente
// kd de la luz: el color del pasto, iluminado por el sol.
//
// Práctico 09 (Parte 1): este archivo ya NO es el texto completo. El programa
// se arma con   #version + iluminacion.glsl + terreno.fs   (en ese orden), así
// que acá se pueden usar los uniforms, el struct Material y las funciones
// iluminar(), iluminar_suelo() y aplicar_niebla() del código común sin
// declararlos. La niebla que antes estaba repetida acá se borró: es la de
// iluminacion.glsl.
//
// Práctico 09 (Parte 2): el terreno se ILUMINA. La luz se calcula POR
// FRAGMENTO con la normal interpolada (sombreado de Phong; no confundir con el
// MODELO de Phong, que es la fórmula de iluminar()). Con la luz calculada por
// vértice (sombreado de Gouraud) el resultado sería parecido en el terreno,
// porque sus triángulos son chicos, pero la guía propone hacerlo por fragmento.
//
// Práctico 09 (Parte 5): CORRECCIÓN GAMMA. El color del pasto (y el color plano
// de depuración) entra a la luz pasado a LINEAL con a_lineal(), y el resultado
// final (después de la niebla) sale a sRGB con a_srgb(). Las dos funciones son
// del código común; con la tecla C (uGamma = 0) no hacen nada.
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de terreno.vs, INTERPOLADAS por la GPU para este
// fragmento (mismo nombre y tipo que los out del vertex shader).
in vec3  vNormalMundo; // P09: normal en MUNDO (ya mezclada con la del nivel grueso), sin normalizar
in vec2  vTexCoord;    // coordenadas de textura, en teselas
in vec3  vPosMundo;    // posición en mundo (origen flotante): para la luz y la niebla

// SALIDA (out): el color final de este píxel (R, G, B, A en [0, 1]).
out vec4 FragColor;

// El sampler lee la UNIDAD de textura 0 (layout binding): del lado de C++
// alcanza con textura.bind(0) antes de dibujar, sin setear este uniform. Su
// valor es el NÚMERO DE UNIDAD, no el identificador de la textura.
layout (binding = 0) uniform sampler2D uTextura;

uniform vec3 uColor;         // color plano (depuración: colores por nivel, tecla L)
uniform int  uUsarTextura;   // 1 = textura (pasto), 0 = color plano

// La luz (uSolDir, uSolColor, uAmbiente, uOjo, uModoSuelo, iluminar_suelo) y la
// niebla (uNiebla*, aplicar_niebla) vienen de iluminacion.glsl: son las MISMAS
// que usan el avión y el suelo continuado.

void main()
{
    // La NORMAL llega INTERPOLADA entre los tres vértices del triángulo. El
    // promedio de vectores unitarios es MÁS CORTO que 1 (por ejemplo, el
    // promedio de dos normales a 90° mide 0,71), y N·L supone largo 1: hay que
    // volver a normalizarla. Sin esto, el centro de cada triángulo quedaría un
    // poco más oscuro que sus vértices.
    vec3 N = normalize(vNormalMundo);

    // La textura se lee SIEMPRE (también con color plano) y recién después se
    // elige: el filtrado con mipmaps necesita comparar las coordenadas de
    // fragmentos vecinos, y eso no está definido si la lectura queda adentro de
    // un if que algunos fragmentos toman y otros no.
    vec3 texel = texture(uTextura, vTexCoord).rgb;              // color del pasto en este punto (sRGB)
    // P09: el color pasa a ser kd. Parte 5: en LINEAL. La textura es una foto
    // (sRGB) y el color plano de la tecla L se eligió mirando la pantalla
    // (también sRGB): los dos se pasan a lineal antes de entrar en las cuentas.
    vec3 kd    = a_lineal((uUsarTextura == 1) ? texel : uColor);

    // LUZ: la MISMA función y el MISMO material que el suelo continuado
    // (iluminar_suelo, código común: kd = textura, ks = 0). La tecla B cambia
    // el modo (con luz / sin luz / normal constante) en los dos a la vez.
    // Las cuentas se hacen en LINEAL (Parte 5).
    vec3 color = iluminar_suelo(vPosMundo, N, kd);

    // NIEBLA, DESPUÉS de la luz: la niebla es el aire entre el punto y el ojo,
    // no depende del sol (la niebla no se ilumina). También en lineal: el
    // color de la niebla ya llega convertido desde la CPU.
    color = aplicar_niebla(color, vPosMundo);

    // Parte 5: AL FINAL, de vuelta a sRGB para la pantalla.
    FragColor = vec4(a_srgb(color), 1.0);
}
