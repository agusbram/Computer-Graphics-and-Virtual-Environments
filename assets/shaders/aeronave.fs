#version 460 core

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
// (En el Práctico 09 este color pasa a ser el kd del material.)
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de aeronave.vs, INTERPOLADAS para este fragmento.
in vec3  vNormal;      // normal interpolada (sin usar hasta la luz del P09)
in vec2  vTexCoord;    // coordenadas de textura (ya escaladas por la pieza)
in float vDistancia;   // distancia a la cámara (niebla)

// SALIDA (out): el color final de este píxel.
out vec4 FragColor;

// Textura de la pieza (camuflaje, chapa o imagen de prueba). El sampler lee la
// UNIDAD 0: del lado de C++, textura->bind(0) antes de dibujar la pieza.
layout (binding = 0) uniform sampler2D uTextura;   // unidad 0

uniform vec3 uColor;         // color de la pieza (o blanco, o el gris compensado)
uniform int  uUsarTextura;   // 1 = color * textura, 0 = color plano

// Niebla lineal (igual que terreno.fs; pasa a código común en el Práctico 09).
uniform vec3  uNieblaColor;  // color de la niebla (= horizonte del cielo)
uniform float uNieblaInicio; // distancia donde empieza
uniform float uNieblaFin;    // distancia donde es total; <= 0 -> apagada (tecla N)

vec3 aplicar_niebla(vec3 color, float distancia)
{
    if (uNieblaFin <= 0.0) {
        return color;
    }
    // f = cuánta niebla (0 cerca, 1 lejos); mix mezcla el color con la niebla.
    float f = clamp((distancia - uNieblaInicio) / (uNieblaFin - uNieblaInicio), 0.0, 1.0);
    return mix(color, uNieblaColor, f);
}

void main()
{
    vec3 texel = texture(uTextura, vTexCoord).rgb;          // se lee siempre (mipmaps)
    vec3 color = (uUsarTextura == 1) ? uColor * texel : uColor;   // color sin niebla
    FragColor = vec4(aplicar_niebla(color, vDistancia), 1.0);

    // Depuración de normales (Práctico 03; se usará en el Práctico 09):
    //   FragColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}
