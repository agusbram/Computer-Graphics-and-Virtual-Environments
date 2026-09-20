// -----------------------------------------------------------------------------
// CameraCommand.h
//
// Comando de cámara producido por el usuario (a través del InputHandler).
// Son DELTAS acumulados en el cuadro, en coordenadas esféricas: quien lee el
// mouse no conoce el estado de la órbita ni sus límites; acumular y acotar es
// responsabilidad de CameraSystem.
// -----------------------------------------------------------------------------

#pragma once

struct CameraCommand {
    float yaw_delta   = 0.0f;   // [rad] cambio de acimut (longitud sobre la órbita)
    float pitch_delta = 0.0f;   // [rad] cambio de latitud (elevación)
    float dist_delta  = 0.0f;   // [unidades de escena] (+) se aleja del objetivo
};
