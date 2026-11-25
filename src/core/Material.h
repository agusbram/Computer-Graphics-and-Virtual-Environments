// -----------------------------------------------------------------------------
// Material.h — coeficientes del modelo de Phong de UN objeto (Práctico 09, Parte 3)
//
// Es la cara CPU del struct Material de assets/shaders/iluminacion.glsl: los
// MISMOS tres campos, con los mismos significados. Reemplaza al color del
// RenderItem del Práctico 04 (decisión 6), como propone la guía.
//
// Qué es cada campo (la fórmula está en iluminar(), iluminacion.glsl):
//   kd : coeficiente DIFUSO, "de qué color es" la superficie. Multiplica a la
//        luz ambiente y a la del sol (ka = kd, lo habitual). Es el color que
//        antes llevaba el RenderItem. Con textura, la textura lo MULTIPLICA en
//        el shader (decisión 6): kd_final = kd · texel.
//   ks : coeficiente ESPECULAR, cuánta luz del sol se refleja como BRILLO (el
//        reflejo tipo espejo). 0 = mate, sin brillo. No lo tiñe la textura:
//        el brillo de una pintura o de un plástico es del color de la luz.
//   ns : exponente especular (brillo, "shininess"). Más alto = brillo más
//        CHICO y concentrado (metal pulido); más bajo = brillo ancho y suave.
//        Con ks = 0 no tiene efecto.
//
// Es un tipo de SOLO DATOS (como FlightData o CameraData): no sabe de OpenGL.
// Lo envía main al dibujar cada pieza (dibujar_items), igual que la matriz de
// modelo. Los valores por omisión son los de la guía: blanco, sin brillo.
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>   // glm::vec3: los coeficientes kd y ks (uno por canal R, G, B)

struct Material {
    glm::vec3 kd = glm::vec3(1.0f);   // difuso: el color que antes llevaba el RenderItem
    glm::vec3 ks = glm::vec3(0.0f);   // especular: 0 = sin brillo
    float     ns = 1.0f;              // exponente especular (sin efecto con ks = 0)
};
