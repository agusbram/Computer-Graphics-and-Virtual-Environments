#version 460 core

// -----------------------------------------------------------------------------
// solid.fs — fragment shader del cubo (Práctico 02)
//
// Pinta cada fragmento con el color que vino del vertex shader (interpolado
// entre los tres vértices del triángulo). Como cada cara tiene un color
// plano, la interpolación no varía el color dentro de la cara.
// -----------------------------------------------------------------------------

in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}