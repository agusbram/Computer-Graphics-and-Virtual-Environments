#version 460 core

// -----------------------------------------------------------------------------
// solid.vs — vertex shader del cubo (Práctico 02)
//
// Los atributos llegan desde el VAO que arma Mesh::load():
//   location 0 -> aPos   (3 floats, posición)
//   location 1 -> aColor (3 floats, color)
// El color se pasa al fragment shader para que lo interpole el rasterizador.
// -----------------------------------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

// Caja negra de hoy (vence 04-sep cuando se vean las transformaciones):
// gira el cubo para que no quede "de frente" (sin esto se vería como un
// cuadrado, porque la cámara mira en -z y el cubo está en el plano z = 0).
// OJO: GLSL recibe la matriz por COLUMNAS: cada vec3 de esta lista es una
// COLUMNA, no una fila. Lo escrito se copió de las filminas del práctico.
const mat3 kRotacionFija = mat3(
    vec3( 0.3686, -0.1454, -0.3119),   // columna 0
    vec3( 0.0000,  0.5438, -0.2536),   // columna 1
    vec3(-0.2581, -0.2077, -0.4454));  // columna 2

void main() {
    vec3 pos_girada = kRotacionFija * aPos;
    gl_Position = vec4(pos_girada, 1.0);
    vColor = aColor;
}