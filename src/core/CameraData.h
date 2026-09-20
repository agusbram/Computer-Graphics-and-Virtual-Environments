// -----------------------------------------------------------------------------
// CameraData.h
//
// Lo único que sale del módulo de cámara: el par de matrices que completan
// el proceso de proyección (vista + proyección). El resto del programa las
// usa para construir gl_Position = uProjection * uView * uModel * v.
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>

struct CameraData {
    glm::mat4 view       = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
};
