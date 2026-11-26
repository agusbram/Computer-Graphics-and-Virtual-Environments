// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// aeronave.fs — fragment shader de las PIEZAS DEL AVIÓN (Prácticos 08 y 09)
//
// Práctico 09 (Parte 1): el programa se arma con
//   #version + iluminacion.glsl + aeronave.fs
// La niebla que estaba repetida acá se borró: es la del código común.
//
// Práctico 09 (Parte 3): el avión se ILUMINA con la misma función que el suelo,
// iluminar() del código común, pero con el MATERIAL DE CADA PIEZA (uMaterial).
// Qué kd manda Aircraft según la apariencia (el color del Práctico 08):
//   - camuflaje (tecla T, la inicial): blanco -> kd = la textura tal cual;
//   - metal gris (tecla T): el gris de la pieza dividido por el brillo medio
//     de la chapa -> se conserva el gris y la chapa suma rayones y remaches;
//   - colores de las piezas (tecla T): sin textura (uUsarTextura = 0);
//   - imagen de prueba (tecla U): blanco -> kd = la imagen tal cual.
// ks y ns dependen de la apariencia (mate, metal, plástico; ver Aircraft.cpp).
//
// Práctico 09 (Parte 5): CORRECCIÓN GAMMA. El kd de la pieza y la textura se
// pasan a LINEAL (a_lineal) antes de la luz, y el resultado final, a sRGB
// (a_srgb). El kd se eligió en el Práctico 04 MIRANDO la pantalla sin
// corrección: es un color en sRGB. Si no se lo convirtiera, con la corrección
// activa la pieza se vería más clara y desaturada que en el Práctico 04.
// ks y ns NO se convierten: ks es una proporción de la luz, no un color.
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de aeronave.vs, INTERPOLADAS para este fragmento.
in vec3  vNormalMundo; // P09: normal en MUNDO (con la matriz normal), sin normalizar
in vec2  vTexCoord;    // coordenadas de textura (ya escaladas por la pieza)
in vec3  vPosMundo;    // posición en mundo (para la luz y la niebla)

// SALIDA (out): el color final de este píxel.
out vec4 FragColor;

// Textura de la pieza (camuflaje, chapa o imagen de prueba). El sampler lee la
// UNIDAD 0: del lado de C++, textura->bind(0) antes de dibujar la pieza.
layout (binding = 0) uniform sampler2D uTextura;   // unidad 0

// P09: el MATERIAL de la pieza. Es un uniform de tipo struct (el struct
// Material está declarado en iluminacion.glsl, que va antes de este archivo).
// Cada campo es un uniform aparte: la CPU busca su ubicación con el nombre
// completo, con punto ("uMaterial.kd", "uMaterial.ks", "uMaterial.ns").
uniform Material uMaterial;
uniform int  uUsarTextura;   // 1 = kd * textura, 0 = kd plano

// P09 (Parte 3, tecla V): 1 = pintar la NORMAL como color, para verificarla
// antes de mirar la luz (es lo primero que pide la guía: las normales de las
// primitivas se usan por primera vez en este práctico).
uniform int  uVerNormales;

// La luz (uSolDir, uSolColor, uAmbiente, uOjo, uBlinnPhong, iluminar) y la
// niebla (aplicar_niebla) vienen de iluminacion.glsl: las MISMAS del terreno.

void main()
{
    // La normal llega interpolada y MÁS CORTA que 1 (promedio de vectores
    // unitarios): hay que normalizarla antes de usarla (N·L supone largo 1).
    vec3 N = normalize(vNormalMundo);

    // VERIFICACIÓN DE LAS NORMALES (tecla V). Cada componente va de [-1, 1] a
    // [0, 1]: N·0,5 + 0,5. Una cara que mira hacia +X se ve rosada
    // (1; 0,5; 0,5), hacia +Y verde claro y hacia +Z (ARRIBA en esta escena)
    // lila. Sin luz ni niebla: es un DATO (una dirección codificada como
    // color), no un color de la escena. Por eso tampoco se le aplicará la
    // corrección gamma de la Parte 5.
    if (uVerNormales == 1) {
        FragColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    }

    // La textura se lee SIEMPRE (también sin textura) y recién después se
    // elige: el filtrado con mipmaps necesita las derivadas de las (u, v)
    // entre fragmentos vecinos, que no están definidas dentro de un if.
    vec3 texel = texture(uTextura, vTexCoord).rgb;   // sRGB (es una imagen)

    // El material de ESTE fragmento: el de la pieza, con la textura
    // MULTIPLICANDO al kd (decisión 6; la textura aporta el detalle y el kd
    // tiñe). ks y ns no los toca la textura: el brillo de una pintura o de un
    // metal es del color de la luz, no del de la superficie.
    // Parte 5: kd y textura, los DOS a lineal. (Como (a·b)^2,2 = a^2,2·b^2,2,
    // da lo mismo convertir cada uno o el producto.) ks queda como está.
    Material m = uMaterial;
    m.kd = a_lineal(uMaterial.kd);
    if (uUsarTextura == 1) {
        m.kd *= a_lineal(texel);
    }

    // LUZ: la MISMA función que el suelo (código común), con el material de
    // la pieza. A diferencia del suelo, acá ks > 0: aparece el BRILLO
    // especular, la única parte de la luz que cambia al mover la cámara.
    // Las cuentas, en LINEAL (Parte 5).
    vec3 color = iluminar(vPosMundo, N, m);

    // NIEBLA, después de la luz (la niebla no se ilumina). En lineal.
    color = aplicar_niebla(color, vPosMundo);

    // Parte 5: AL FINAL, de vuelta a sRGB para la pantalla.
    FragColor = vec4(a_srgb(color), 1.0);
}
