// -----------------------------------------------------------------------------
// iluminacion.glsl — código GLSL COMÚN a los fragment shaders de la escena
// (Práctico 09: Parte 1, el archivo común; Parte 2, la luz del suelo; Parte 3,
// el avión y Blinn-Phong; Parte 4, la niebla exponencial; Parte 5, la
// corrección gamma)
//
// El terreno, el avión y el fondo (que pinta el suelo continuado) tienen que
// usar EL MISMO sol y LA MISMA niebla: si uno usara otro modelo u otros
// valores, se notaría una costura (por ejemplo, entre la malla del terreno y
// el suelo continuado). Por eso las funciones y sus parámetros se escriben UNA
// sola vez, acá, y cada fragment shader las usa en lugar de repetirlas. (En el
// Práctico 08 la niebla estaba repetida en terreno.fs, aeronave.fs y fondo.fs.)
//
// CÓMO SE USA. GLSL no tiene #include: este archivo se PEGA delante del código
// propio de cada fragment shader. main lo pide así al cargar cada programa:
//   load_shader_source("terreno", {"shaders/terreno.vs"},
//                      {"shaders/iluminacion.glsl", "shaders/terreno.fs"});
// y Shader compila las cadenas una detrás de la otra:
//   "#version 460 core" + iluminacion.glsl + terreno.fs
// Como este archivo va ANTES, todo lo que declara (uniforms, el struct
// Material y las funciones) ya existe cuando empieza el código propio.
//
// SIN #version: esa línea tiene que ser la primera del texto completo y la
// agrega Shader a todas las etapas. Si este archivo (o uno propio) la tuviera,
// quedaría en el medio del texto y no compilaría.
//
// OJO: el código propio NO puede volver a declarar un uniform de acá (por
// ejemplo uOjo o uNieblaColor): el compilador da un error de redefinición.
//
// COORDENADAS: "mundo" es el sistema con ORIGEN FLOTANTE (el avión en el
// origen; ver main.cpp). P, N, uSolDir y uOjo tienen que estar TODOS en ese
// mismo sistema. Mezclar sistemas no da error: el síntoma es que la luz se
// mueve con la cámara. La escena es Z-ARRIBA (la guía usa Y-arriba), pero
// estas funciones no dependen de cuál eje es "arriba".
// -----------------------------------------------------------------------------

// --- Parámetros de la ESCENA ---------------------------------------------------
// Los manda C++ en cada cuadro desde el struct Iluminacion (Iluminacion.h), el
// MISMO para los tres programas. Un programa que no use alguno no lo tiene (el
// compilador lo elimina) y su ubicación es -1: está previsto.

uniform vec3  uSolDir;        // dirección HACIA el sol (de la superficie a la luz), mundo, unitaria
uniform vec3  uSolColor;      // color e intensidad del sol
uniform vec3  uAmbiente;      // luz ambiente (la que llega a todos lados, aun en sombra)
uniform vec3  uOjo;           // posición de la cámara, en mundo (relativa al avión: origen flotante)
uniform int   uBlinnPhong;    // P09, Parte 3 (tecla R): 0 = Phong (vector R), 1 = Blinn-Phong (vector H)

// Niebla. Parte 1: la lineal del Práctico 08 (con un inicio y un fin), mudada
// a este archivo. Parte 4: EXPONENCIAL, con un solo parámetro, la VISIBILIDAD V.
uniform vec3  uNieblaColor;   // color de la niebla = color del horizonte del cielo (tecla J: gris).
                              // P09, Parte 5: llega YA EN LINEAL si la gamma está activa
                              // (lo convierte main, una vez por cuadro, en la CPU)
uniform float uVisibilidad;   // V: distancia a la que la niebla llega al 63 %, unidades de escena;
                              // <= 0 -> niebla apagada (tecla N)

// Corrección gamma (Parte 5): ver a_lineal() / a_srgb() al final del archivo.
uniform int   uGamma;         // 1 = corrección activa (lo correcto); 0 = sin corrección (tecla C)

// --- Material (Partes 2 y 3) ----------------------------------------------------
// Coeficientes del modelo de Phong de UN objeto. Es un dato del OBJETO, no de
// la escena: el suelo lo arma con su textura (iluminar_suelo, más abajo) y el
// avión lo recibe por pieza (Parte 3: `uniform Material uMaterial;` en
// aeronave.fs; del lado de C++, el struct Material de Material.h, con los
// mismos tres campos). Como este archivo va ANTES del código propio, el struct
// ya está definido cuando aeronave.fs declara ese uniform.
struct Material
{
    vec3  kd;    // coeficiente difuso (y ambiente: ka = kd, lo habitual)
    vec3  ks;    // coeficiente especular (cuánta luz se refleja como brillo)
    float ns;    // exponente especular: más alto = brillo más chico y concentrado
};

// Modelo de iluminación de PHONG con UNA luz direccional (el sol).
//   P: posición del punto, en mundo.   N: normal en mundo, YA normalizada.
// Devuelve el color iluminado: ambiente + difusa + especular.
// Desde la Parte 2 la usa el SUELO (malla del terreno y suelo continuado, a
// través de iluminar_suelo, más abajo); desde la Parte 3, también el avión.
//
// Las tres componentes (valores de la escena: sol 0,9 y ambiente 0,1 desde la
// Parte 5, elegidos con la corrección gamma activa; antes, 0,7 y 0,3):
//   ambiente : uAmbiente·kd. La luz que "rebota por todos lados": llega igual a
//              todos los puntos, aun a los que no ven el sol. Sin ella, las
//              laderas en sombra quedarían NEGRAS.
//   difusa   : uSolColor·kd·max(N·L, 0). Ley de Lambert: una superficie
//              recibe más luz cuanto más de FRENTE mira al sol (N·L = coseno
//              del ángulo entre la normal y la dirección al sol). Es la que
//              hace VISIBLE EL RELIEVE: las laderas hacia el sol quedan claras
//              y las de atrás, oscuras. No depende de dónde esté el ojo.
//   especular: uSolColor·ks·max(R·V, 0)^ns. El BRILLO: el reflejo del sol
//              como en un espejo (R) visto desde el ojo (V). Es la única que
//              cambia al mover la cámara. En el suelo ks = 0: no aparece.
//
// PHONG o BLINN-PHONG (Parte 3, decisión 5; tecla R). Solo cambia la
// especular, es decir, CÓMO se mide "qué tan cerca está el ojo del reflejo":
//   Phong      : R = reflect(-L, N), el rayo reflejado; brillo = (R·V)^ns.
//   Blinn-Phong: H = normalize(L + V), el vector a MITAD de camino entre la
//                luz y el ojo; brillo = (N·H)^ns'. Si el ojo está justo en
//                el reflejo, H coincide con N.
// El ángulo entre N y H es más o menos la MITAD del ángulo entre R y V, así
// que con el mismo exponente el brillo de Blinn-Phong sale bastante más
// ANCHO. Para comparar reflejos de tamaño parecido se usa ns' = 4·ns (la
// regla práctica habitual). Blinn-Phong es el modelo de la tubería fija de
// OpenGL: con luz direccional y ojo lejano, H es casi constante y era más
// barato; además no se corta de golpe cuando R·V pasa por 0 a ángulos rasantes.
vec3 iluminar(vec3 P, vec3 N, Material m)
{
    vec3  L     = uSolDir;                  // direccional: la MISMA en todos los puntos
    vec3  V     = normalize(uOjo - P);      // hacia el ojo
    float NdotL = max(dot(N, L), 0.0);      // coseno entre la normal y la luz (0 si la cara no la ve)

    vec3 ambiente  = uAmbiente * m.kd;
    vec3 difusa    = uSolColor * m.kd * NdotL;
    vec3 especular = vec3(0.0);
    if (NdotL > 0.0) {                      // sin brillo en la cara que no recibe luz
        float brillo;                       // en [0, 1]: 1 = el ojo justo en el reflejo
        if (uBlinnPhong == 1) {
            vec3 H = normalize(L + V);      // vector medio (halfway) entre la luz y el ojo
            brillo = pow(max(dot(N, H), 0.0), 4.0 * m.ns);
        } else {
            vec3 R = reflect(-L, N);        // reflect espera la dirección QUE LLEGA: -L
            brillo = pow(max(dot(R, V), 0.0), m.ns);
        }
        especular = uSolColor * m.ks * brillo;
    }
    return ambiente + difusa + especular;
}

// --- El SUELO: su material y la verificación de la tecla B (Parte 2) ----------
// El suelo se dibuja con DOS programas: la MALLA del terreno (terreno.fs) y el
// SUELO CONTINUADO más allá de la malla (fondo.fs). La guía pide que el suelo
// continuado se ilumine "con la misma función y con el mismo material que la
// malla"; si no, se vería el borde de la ventana de terreno. Por eso el
// material del suelo se escribe UNA sola vez, acá, y los dos programas llaman a
// iluminar_suelo() en lugar de armar cada uno su Material.

// Modo de iluminación del suelo (tecla B), para la verificación de la guía:
// comparar la MISMA escena, desde la MISMA cámara, en tres modos.
//   0 = con luz y las normales del terreno (Práctico 07, con las correcciones
//       del Práctico 09): el relieve se ve.
//   1 = sin luz: el color es directamente kd (la textura), como en el
//       Práctico 08: el relieve casi no se ve.
//   2 = con luz, pero con TODAS las normales reemplazadas por la vertical
//       (0, 0, 1): el relieve desaparece (salvo su contorno contra el cielo)
//       aunque la geometría es la misma. Muestra que lo que se percibe del
//       relieve viene de la NORMAL y no de la posición de los vértices.
// (La guía dice (0, 1, 0) porque usa Y arriba; esta escena es Z-ARRIBA.)
uniform int uModoSuelo;

// Ilumina un punto del suelo.
//   P : posición en mundo.  N : normal en mundo, normalizada.  kd : el color
//   del pasto en ese punto (la textura; o el color plano de depuración, tecla L).
// MATERIAL DEL SUELO (decisión 7): kd = la textura y SIN brillo especular,
// ks = 0. Con ks > 0 toda la superficie brilla igual y, mirando hacia el sol,
// el pasto se ve mojado o de plástico. Con ks = 0 el valor de ns no importa
// (multiplica a cero); se deja en 1.
vec3 iluminar_suelo(vec3 P, vec3 N, vec3 kd)
{
    if (uModoSuelo == 1) {                  // B: sin luz -> el color es kd (como el P08)
        return kd;
    }
    if (uModoSuelo == 2) {                  // B: normal constante (vertical, Z arriba)
        N = vec3(0.0, 0.0, 1.0);
    }
    Material m;
    m.kd = kd;                              // la textura ES el coeficiente difuso
    m.ks = vec3(0.0);                       // el pasto no brilla
    m.ns = 1.0;                             // sin efecto con ks = 0
    return iluminar(P, N, m);
}

// NIEBLA: mezcla el color con el de la niebla según la distancia del punto P
// (en mundo) al ojo:
//   C = (1 - f)·C_luz + f·C_niebla  =  mix(C_luz, C_niebla, f)
// f = 0: el color iluminado tal cual; f = 1: solo el color de la niebla.
// Se aplica DESPUÉS de la iluminación: la niebla es el AIRE entre el punto y
// el ojo, no depende de la orientación de la superficie ni del sol (la niebla
// no se ilumina).
//
// Parte 4 — EXPONENCIAL:  f = 1 - e^(-d/V).
//   Cada tramo de aire de largo V deja pasar la MISMA fracción de la luz del
//   objeto (e^-1 = 37 %): por eso es una exponencial. Con la visibilidad V:
//     d = V   -> f = 63 %        d = 2V   -> f = 86 %
//     d = 2,5V -> f = 92 %       d = 3V   -> f = 95 %
//   A diferencia de la LINEAL del Práctico 08 (cero hasta un inicio d0 y total
//   desde un fin d1), empieza a actuar desde d = 0 y nunca llega a 1 del todo:
//   no hay una distancia donde "arranca" ni un corte. Un solo parámetro (V) y
//   una forma que imita la atmósfera (perspectiva atmosférica).
//   (La exponencial CUADRÁTICA, 1 - e^(-(d/V)^2), deja más transparente lo
//   cercano y cierra más lo lejano; la guía propone la simple.)
//
// La distancia d es la distancia en el ESPACIO, |P - uOjo|, en mundo (origen
// flotante: P y uOjo relativos al avión; la resta da lo mismo). Por eso, a una
// altura h sobre el terreno, el punto más cercano del suelo ya está a d >= h
// (ver Practico09-Decisiones.md, Parte 4: la altitud).
vec3 aplicar_niebla(vec3 color, vec3 P)
{
    if (uVisibilidad <= 0.0) {              // tecla N: niebla apagada
        return color;
    }
    float d = length(P - uOjo);             // distancia del punto al ojo (unidades de escena)
    float f = 1.0 - exp(-d / uVisibilidad); // factor de niebla, en [0, 1)
    return mix(color, uNieblaColor, f);
}

// --- CORRECCIÓN GAMMA (Parte 5) --------------------------------------------------
// EL PROBLEMA. Las cuentas de la luz (kd·luz·cos θ, sumar ambiente + difusa +
// especular) y la mezcla de la niebla suman y multiplican INTENSIDADES DE LUZ:
// solo valen si los números son PROPORCIONALES a la luz (escala LINEAL). Pero
// la pantalla no es lineal: un valor v se ve con una intensidad ~ v^2,2. Por
// eso las imágenes y los colores elegidos "a ojo" vienen en sRGB, comprimidos
// con la curva inversa (v^(1/2,2)): así se ven bien en la pantalla sin hacer
// nada. Si se hacen las cuentas con esos valores y el resultado se escribe tal
// cual, la luz queda deformada. Ejemplo de la guía: una cara con N·L = 0,5
// recibe la mitad de la luz; se escribe 0,5 y la pantalla muestra
// 0,5^2,2 = 0,22 (¡menos de la cuarta parte!): la sombra llega demasiado rápido.
//
// LA SOLUCIÓN, en dos pasos en cada fragment shader:
//   1. ENTRAR en lineal: pasar con a_lineal() cada COLOR que entra en las
//      cuentas (texturas, cubemap, degradé, kd de las piezas, colores de
//      depuración). El color de la niebla lo pasa main en la CPU.
//   2. SALIR en sRGB: al FINAL de todo (después de la niebla), a_srgb() sobre
//      el resultado. En el ejemplo se escribe 0,5^(1/2,2) = 0,73 y la pantalla
//      muestra 0,73^2,2 = 0,5: justo la mitad.
//
// QUÉ SE CONVIERTE: lo que representa un COLOR elegido por cómo se VE en la
// pantalla (no importa si sale de una imagen o se escribió a mano).
// QUÉ NO: lo que ya es una PROPORCIÓN de luz (uSolColor = 0,9 y uAmbiente =
// 0,1 suman la luz total; ks es la fracción reflejada como brillo) y los DATOS
// que se pintan como color (las normales de la tecla V: una dirección, no un
// color).
//
// Se usa la aproximación de la curva sRGB con γ = 2,2 (la de la Unidad III y la
// guía). Decisión 11: EN EL SHADER, con pow, y no por hardware (texturas
// GL_SRGB8_ALPHA8 + GL_FRAMEBUFFER_SRGB): así se puede APAGAR con una tecla
// para comparar (C), y no depende de que la ventana admita un framebuffer sRGB.
// OJO: entonces las texturas tienen que seguir en GL_RGBA8. Si además se
// cargaran como sRGB, la conversión a lineal se haría DOS veces (todo oscuro).
//
// uGamma = 0 (tecla C): las dos funciones devuelven el color TAL CUAL, y la
// escena sale idéntica a la de la Parte 4. Así los shaders se escriben una vez
// (con las conversiones, como propone la guía) y la tecla no los cambia.
const float GAMMA = 2.2;

// Color en sRGB -> lineal (para las CUENTAS). Ej.: 0,5 -> 0,22.
vec3 a_lineal(vec3 c)
{
    return (uGamma == 1) ? pow(c, vec3(GAMMA)) : c;
}

// Resultado lineal -> sRGB (para la PANTALLA). Ej.: 0,22 -> 0,5; 0,5 -> 0,73.
// Va al FINAL, después de la niebla: es lo último antes de escribir FragColor.
vec3 a_srgb(vec3 c)
{
    return (uGamma == 1) ? pow(c, vec3(1.0 / GAMMA)) : c;
}
