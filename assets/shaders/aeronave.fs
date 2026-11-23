// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// aeronave.fs — fragment shader de las PIEZAS DEL AVIÓN (Práctico 08)
//
// Sin textura: el color plano de la pieza (como hasta el Práctico 07).
// Con textura: color * textura. Qué color manda Aircraft según la apariencia:
//   - camuflaje (tecla T, la inicial): blanco -> se ve la textura tal cual;
//   - metal gris (tecla T): el gris de la pieza dividido por el brillo medio
//     de la chapa -> se conserva el gris y la chapa suma rayones y remaches;
//   - colores de las piezas (tecla T): sin textura (uUsarTextura = 0);
//   - imagen de prueba (tecla U): blanco -> se ve la imagen tal cual, para
//     verificar las coordenadas de textura de cada primitiva.
// (En el Práctico 09, Parte 3, este color pasa a ser el kd del material.)
//
// Práctico 09 (Parte 1): el programa se arma con
//   #version + iluminacion.glsl + aeronave.fs
// La niebla que estaba repetida acá se borró: es la del código común.
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de aeronave.vs, INTERPOLADAS para este fragmento.
in vec3  vNormal;      // normal interpolada (sin usar hasta la luz del P09, Parte 3)
in vec2  vTexCoord;    // coordenadas de textura (ya escaladas por la pieza)
in vec3  vPosMundo;    // posición en mundo (niebla; desde la Parte 3, también la luz)

// SALIDA (out): el color final de este píxel.
out vec4 FragColor;

// Textura de la pieza (camuflaje, chapa o imagen de prueba). El sampler lee la
// UNIDAD 0: del lado de C++, textura->bind(0) antes de dibujar la pieza.
layout (binding = 0) uniform sampler2D uTextura;   // unidad 0

uniform vec3 uColor;         // color de la pieza (o blanco, o el gris compensado)
uniform int  uUsarTextura;   // 1 = color * textura, 0 = color plano

// La niebla (uNieblaColor, uNieblaInicio, uNieblaFin y aplicar_niebla) viene de
// iluminacion.glsl: la misma del terreno y del suelo continuado.

void main()
{
    vec3 texel = texture(uTextura, vTexCoord).rgb;          // se lee siempre (mipmaps)
    vec3 color = (uUsarTextura == 1) ? uColor * texel : uColor;   // color sin niebla
    FragColor = vec4(aplicar_niebla(color, vPosMundo), 1.0);      // código común

    // Depuración de normales (Práctico 03; se usará en el Práctico 09):
    //   FragColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}
