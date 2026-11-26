// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// fondo.fs — fragment shader del FONDO (Práctico 08, Partes 4 y 5 + opcional; Práctico 09)
//
// Pinta cada píxel del fondo por CAPAS, según su dirección d:
//   1. CIELO (por encima del horizonte): el cubemap (Parte 4) o el degradé 1D
//      por elevación (opcional), según uModoCielo (tecla K).
//   2. SUELO CONTINUADO (por debajo del horizonte, Parte 5): el rayo que sale
//      del ojo en la dirección d choca contra un PLANO HORIZONTAL INFINITO a la
//      altura media del terreno; ahí se lee la textura del pasto con la MISMA
//      escala y el MISMO origen que la malla. Como la malla se dibuja antes y
//      más cerca, este suelo solo queda a la vista donde la malla no llega:
//      tapa la "banda" del problema de la altitud.
//
// ESCENA Z-ARRIBA: la guía usa y como altura; acá la altura es z. Entonces:
//   - "debajo del horizonte" es d.z < 0 (la guía: d.y < 0);
//   - el plano del suelo es z = altura del terreno (la guía: y = ys);
//   - el punto del suelo es (x, y) (la guía: (x, z)).
// El cubemap de OpenGL, en cambio, tiene "arriba" en +Y: para leerlo se
// reacomoda la dirección con a_cubemap().
//
// Práctico 09 (Parte 1): el programa se arma con
//   #version + iluminacion.glsl + fondo.fs
// La niebla que estaba repetida acá se borró: el suelo continuado usa la del
// código común, la MISMA que la malla (si no, se vería el borde). Para eso hace
// falta el punto del suelo en MUNDO, P = uOjo + t·d (uOjo viene del código
// común: por eso este archivo NO lo declara, sería una redefinición).
//
// Práctico 09 (Parte 2): el suelo continuado se ILUMINA con la misma función y
// el mismo material que la malla (iluminar_suelo, código común). Su normal es
// la de un plano horizontal: (0, 0, 1) en esta escena Z-arriba (la guía dice
// (0, 1, 0) porque usa Y arriba). El CIELO no se ilumina: el cubemap ya es la
// foto de un cielo iluminado.
//
// Práctico 09 (Parte 4): la niebla EXPONENCIAL llega a los dos colores del fondo:
//   - SUELO CONTINUADO: con aplicar_niebla() en su punto P(t), como la malla.
//     Como t crece sin límite hacia el horizonte, f -> 1: el suelo llega al
//     horizonte con el color de la niebla.
//   - CIELO, solo CERCA DEL HORIZONTE: las direcciones del cubemap no tienen
//     una distancia, así que no se usa aplicar_niebla(): la mezcla depende de
//     la ELEVACIÓN. En el horizonte (elevación 0) f = 1 y el cielo vale lo
//     mismo que el suelo lejano: desaparece la línea recta que se veía en el
//     Práctico 08 (Practico08-Consultas.md, pregunta 22).
//
// Práctico 09 (Parte 5): CORRECCIÓN GAMMA. Los dos colores del fondo entran en
// LINEAL (a_lineal) y el resultado sale en sRGB (a_srgb):
//   - el CIELO no se ilumina, pero pasa igual por la conversión final: si no se
//     lo pasara antes a lineal, saldría más claro y desaturado ("lavado"). Con
//     las dos conversiones, a_srgb(a_lineal(c)) = c: el cielo se ve IGUAL con
//     y sin la corrección (es una verificación de la guía);
//   - el SUELO CONTINUADO: la textura del pasto a lineal, igual que la malla.
//
// Práctico 09, opcional "HALO DEL SOL" (decisiones 20 a 24, tecla Y):
//   - halo_sol(d): un DISCO de 2° (el real mide 0,53°: se veía muy chico) y un RESPLANDOR
//     alrededor de uSolDir, la dirección de la que viene la luz. Se SUMA al
//     cielo en lineal (es luz). Se ve sobre el degradé (que no tiene sol) y
//     cuando O/P alejan la luz del sol pintado; sobre el sol pintado casi no
//     se ve, porque esa zona de la foto ya es blanco saturado.
//   - cerca del horizonte el halo se funde con la niebla como el resto del
//     cielo, y la mezcla usa color_niebla(d) (código común), la misma que el
//     suelo lejano: con la tecla Y en "completo", la bruma se aclara hacia el
//     sol, en el cielo y en el suelo por igual (sin costura).
// -----------------------------------------------------------------------------

// ENTRADA (in): la dirección de este píxel, interpolada desde fondo.vs (en el
// MUNDO, Z arriba). No es unitaria: se normaliza en main.
in vec3 vDireccion;

// SALIDA (out): el color final de este píxel (cielo o suelo continuado).
out vec4 FragColor;

layout (binding = 0) uniform samplerCube uCielo;          // unidad 0: el cubemap
layout (binding = 1) uniform sampler2D   uTexturaSuelo;   // unidad 1: el pasto (el mismo de la malla)
layout (binding = 2) uniform sampler1D   uDegrade;        // unidad 2: degradé (opcional)

uniform int   uModoCielo;        // 0 = cubemap, 1 = degradé 1D (tecla K)
uniform int   uSueloContinuado;  // 1 = suelo continuado, 0 = no (tecla G: se ve la banda)
uniform vec2  uOjoUV;            // posición (x, y) del ojo en el mundo, MÓDULO el período de
                                 // las (u, v) del terreno (Terrain::kPeriodoUV), unidades de escena
uniform float uAlturaOjo;        // altura del ojo SOBRE el plano del suelo (unidades de escena)
uniform float uTamTesela;        // tamaño de una tesela (el mismo uTamTesela del terreno)
uniform int   uNieblaCielo;      // P09, Parte 4: 1 = el cielo se funde con la niebla cerca del
                                 // horizonte; 0 = no, como el P08 (tecla J, para comparar)

// La luz (uSolDir, uSolColor, uAmbiente, uModoSuelo, iluminar_suelo), la niebla
// (uNieblaColor, uVisibilidad, aplicar_niebla, color_niebla), la posición del
// ojo en mundo (uOjo), la corrección gamma (uGamma, a_lineal, a_srgb; Parte 5)
// y el modo del halo (uHalo, tecla Y) vienen de iluminacion.glsl.

// Dirección de NUESTRA escena (Z arriba) -> convención del cubemap (Y arriba).
// (x, y, z) -> (x, z, -y) es una ROTACIÓN de -90° alrededor del eje X: lleva
// nuestro "arriba" (+Z) al "arriba" del cubemap (+Y). Al ser una rotación (y no
// un simple intercambio de ejes) no espeja la imagen.
// Lo que se rota es el VECTOR DIRECCIÓN d (hacia dónde mira este píxel), no la
// imagen ni la geometría: se reescribe la MISMA dirección en los ejes del
// cubemap, para que texture() lea el texel correcto. Ejemplos:
//   arriba    (0, 0, 1) -> (0, 1, 0)  = cara +Y (top)
//   hacia +Y  (0, 1, 0) -> (0, 0, -1) = cara -Z (back)
//   hacia +X  (1, 0, 0) -> (1, 0, 0)  = cara +X (right; el eje de giro no cambia)
// (Rotación de -90° alrededor de X: y' = z, z' = -y. El cambio "a secas"
// (x, z, y) sería un ESPEJO, con determinante -1: el cielo se vería reflejado.)
vec3 a_cubemap(vec3 d)
{
    return vec3(d.x, d.z, -d.y);
}

// --- Opcional: el HALO DEL SOL (decisiones 20, 21 y 22) ----------------------
// El sol "de la luz": un disco y un resplandor alrededor de uSolDir, la
// dirección de la que viene la luz que ilumina la escena (la misma L de
// iluminar()). El cubemap trae un sol PINTADO, pero es parte de la foto: no se
// mueve con O/P ni con la hora, y el degradé (tecla K) no tiene ninguno. El
// halo marca siempre de dónde viene la luz de verdad.
//
// Todo se mide con el ÁNGULO entre la dirección del píxel (d) y la del sol:
//   coseno = d · uSolDir   (las dos unitarias)   ->   ángulo = acos(coseno)
//
// 1. DISCO (decisión 21): 2° de diámetro, radio 1°. Con la cámara de 45° de
//    campo vertical en 600 px, 1° son ~12,6 px: el disco mide ~25 px.
//    Visto desde la Tierra el sol mide 0,53° (radio 0,265°), y se probó
//    primero así: ~7 px, que se veían MUY CHICOS. El sol que percibimos parece
//    más grande que su tamaño geométrico porque encandila: su luz se
//    desparrama en el ojo (o en la lente) y lo agranda. La pantalla no puede
//    reproducir ese brillo, así que, como en los simuladores y los juegos, el
//    disco se dibuja más grande (unas 4 veces).
//    El borde se suaviza con smoothstep en ~0,16° (~2 px): sin eso quedaría
//    escalonado, porque el MSAA solo suaviza los bordes de los triángulos, no
//    lo que pinta el shader adentro de uno.
//    Su brillo (K_DISCO) es MUCHO mayor que el del cielo: el sol real es
//    miles de veces más brillante. Pasado 1, la pantalla lo recorta: el disco
//    queda blanco aunque esté sobre una nube clara.
// 2. RESPLANDOR (decisión 20): la luz del sol que el aire dispersa apenas
//    hacia los costados (la misma física que la bruma de color_niebla, pero
//    de cerca del sol). Dos términos con la forma max(coseno, 0)^n, que vale
//    1 en el sol y cae con el ángulo; cuanto más grande n, más angosto:
//      corona  : n = 500 -> cae a la mitad a ~3°: el brillo pegado al disco
//                (con el disco de 0,53° era n = 2000, mitad a ~1,5°; con el
//                disco de 2° quedaba casi toda adentro y se ensanchó);
//      aureola : n = 40  -> cae a la mitad a ~11°: el cielo más claro alrededor.
//    Sus brillos son BAJOS a propósito: junto al borde del disco suman ~0,25
//    (de 1). El cielo ya es claro (el azul del degradé alrededor del sol vale
//    ~0,87 en pantalla, ~0,7 en lineal: le falta poco para el tope); con un
//    resplandor fuerte, todo alrededor del sol se recorta a blanco y el disco
//    se pierde adentro de una mancha. Probado: con la corona en 1,0 la mancha
//    blanca medía 34 px (~2,7°) y el disco de 7 px no se distinguía. Con
//    ~0,25 alrededor queda un celeste muy claro y el disco, blanco, se ve.
//
// Todo es proporcional a uSolColor (decisión 22): si el sol cambia de color o
// se apaga, el halo lo sigue. Es LUZ que se suma al cielo, en LINEAL (después
// de a_lineal(cielo) y antes de a_srgb), como toda la luz desde la Parte 5.
const float SOL_RADIO = radians(1.0);      // radio angular del disco: diámetro 2° (el real: 0,53°)
const float SOL_BORDE = radians(0.08);     // medio ancho del borde suavizado (~2 px en total)
const float K_DISCO   = 20.0;              // brillo del disco (satura a blanco)
const float K_CORONA  = 0.15;              // brillo de la corona, junto al disco
const float N_CORONA  = 500.0;             // ancho de la corona: mitad a ~3°
const float K_AUREOLA = 0.12;              // brillo de la aureola
const float N_AUREOLA = 40.0;              // ancho de la aureola: mitad a ~11°

vec3 halo_sol(vec3 d)
{
    float coseno = dot(d, uSolDir);                      // 1 = mirando justo al sol
    float angulo = acos(clamp(coseno, -1.0, 1.0));       // ángulo con el sol, radianes
    // Disco: 1 adentro, 0 afuera, con el borde suavizado.
    float disco  = 1.0 - smoothstep(SOL_RADIO - SOL_BORDE, SOL_RADIO + SOL_BORDE, angulo);
    float c      = max(coseno, 0.0);                     // del lado opuesto al sol, nada
    float brillo = K_DISCO * disco
                 + K_CORONA * pow(c, N_CORONA)
                 + K_AUREOLA * pow(c, N_AUREOLA);
    return uSolColor * brillo;
}

void main()
{
    // La dirección interpolada no es unitaria: para la elevación y la
    // intersección hace falta normalizarla (el cubemap no lo necesita).
    vec3 d = normalize(vDireccion);

    // --- 1. Cielo: cubemap o degradé ---------------------------------------
    vec3 cielo_cubo = texture(uCielo, a_cubemap(d)).rgb;   // color del cubemap en esa dirección
    // Degradé por ELEVACIÓN: asin(d.z) va de -90° a +90°; s = 0 en el horizonte
    // y 1 en el cenit. Debajo del horizonte el clamp deja s = 0 (color del
    // horizonte), que es lo que se ve si el suelo continuado está apagado.
    float elevacion  = asin(clamp(d.z, -1.0, 1.0));
    float s          = clamp(elevacion / radians(90.0), 0.0, 1.0);
    vec3 cielo_degr  = texture(uDegrade, s).rgb;                        // color del degradé
    vec3 cielo       = (uModoCielo == 1) ? cielo_degr : cielo_cubo;     // el cielo elegido (tecla K)
    // P09 (Parte 5): el cielo elegido, a LINEAL (el cubemap es una foto y el
    // degradé se eligió mirando la pantalla: los dos están en sRGB). Así la
    // mezcla con la niebla de abajo se hace en lineal, como en el resto.
    cielo = a_lineal(cielo);

    // Opcional (tecla Y): el HALO DEL SOL, sumado en lineal (es luz). En los
    // modos "completo" y "solo el cielo"; en "sin halo", no. Se suma ANTES de
    // la mezcla con la niebla de abajo: cerca del horizonte se funde con la
    // niebla como el resto del cielo (decisión 23), y no queda cortado en seco
    // contra el suelo cuando el sol está bajo.
    if (uHalo != 2) {
        cielo += halo_sol(d);
    }

    // P09 (Parte 4): NIEBLA EN EL CIELO, solo en los primeros grados sobre el
    // horizonte. Con d unitaria, d.z es el SENO de la elevación (la guía dice
    // d.y porque usa Y arriba). smoothstep(0, 0.10, d.z) sube de 0 a 1, con
    // forma de S (sin quiebres), mientras d.z va de 0 a 0,10, o sea entre 0° y
    // asin(0,10) = 5,7° de elevación. Entonces:
    //   f_cielo = 1 - smoothstep(...):  1 en el horizonte (y debajo), 0 desde 5,7°.
    // En el horizonte el cielo vale EXACTAMENTE el color de la niebla, igual
    // que el suelo lejano (f -> 1): no queda ninguna línea entre los dos. Más
    // arriba de 5,7° el cielo queda como está. Con la niebla apagada (N,
    // uVisibilidad <= 0) o con J en "cielo sin niebla", no se mezcla.
    // Opcional "halo del sol": el color de la niebla es color_niebla(d) (código
    // común), el MISMO que usa aplicar_niebla() para el suelo lejano en esta
    // dirección. Con la tecla Y en "completo" se aclara hacia el sol; como el
    // cielo y el suelo usan la misma función, en el horizonte siguen valiendo
    // lo mismo.
    if (uVisibilidad > 0.0 && uNieblaCielo == 1) {
        float f_cielo = 1.0 - smoothstep(0.0, 0.10, d.z);   // factor de niebla del cielo
        cielo = mix(cielo, color_niebla(d), f_cielo);
    }

    // --- 2. Suelo continuado: rayo P(t) = ojo + t d contra el plano ----------
    //   ojo.z + t d.z = z_suelo   ->   t = (ojo.z - z_suelo) / (-d.z) = uAlturaOjo / (-d.z)
    // t > 0 solo si d.z < 0 (el rayo baja). max(-d.z, 1e-4) evita dividir por 0
    // en el horizonte (ahí t crece sin límite: el suelo queda infinitamente lejos).
    float t   = uAlturaOjo / max(-d.z, 1.0e-4);    // distancia del ojo al suelo por este rayo
    vec2  xy  = uOjoUV + t * d.xy;                 // punto del suelo visto en este píxel
    // MISMA regla que la malla: posición en el mundo / tamaño de la tesela. Hacia
    // el horizonte las (u, v) cambian muy rápido entre píxeles vecinos y el
    // mipmap usa niveles cada vez más chicos: queda el color promedio del pasto.
    // P09 (Parte 5): a LINEAL, igual que en la malla (terreno.fs).
    vec3 kd_suelo = a_lineal(texture(uTexturaSuelo, xy / uTamTesela).rgb);   // color del pasto (= kd), lineal
    // El mismo punto del suelo, pero en MUNDO (origen flotante), para el código
    // común: P = ojo + t·d. Su distancia al ojo es t (d es unitaria), la misma
    // que se usaba en el Práctico 08. Su altura es uOjo.z - uAlturaOjo (el plano).
    vec3 P_suelo = uOjo + t * d;
    // P09 (Parte 2): la LUZ, con la MISMA función y el MISMO material que la
    // malla (kd = textura, ks = 0) y la normal del plano horizontal. Como la
    // luz es direccional, en todo el plano da el MISMO resultado que en una
    // parte plana de la malla: no se ve el borde de la ventana de terreno (salvo
    // donde la malla tiene relieve justo en el borde; ahí la orientación de la
    // malla y la del plano difieren, y lo disimula la niebla).
    const vec3 N_PLANO = vec3(0.0, 0.0, 1.0);      // normal del plano horizontal (Z arriba)
    vec3 suelo = iluminar_suelo(P_suelo, N_PLANO, kd_suelo);
    suelo = aplicar_niebla(suelo, P_suelo);        // la MISMA niebla que la malla, DESPUÉS de la luz
                                                   // (P09, Parte 4: exponencial; hacia el horizonte t
                                                   // crece sin límite y f -> 1: el color de la niebla)

    // Las TRES texturas se leyeron siempre, ANTES de elegir: el filtrado con
    // mipmaps compara coordenadas de píxeles vecinos y dentro de un if eso no
    // está definido. Recién ahora se elige el color.
    // debajo = el rayo baja (mira por debajo del horizonte) y el ojo está por
    // encima del suelo (si no, el plano nunca se cruzaría hacia abajo).
    bool debajo = (d.z < 0.0) && (uAlturaOjo > 0.0);
    vec3 color  = (uSueloContinuado == 1 && debajo) ? suelo : cielo;   // color final (lineal)

    // P09 (Parte 5): AL FINAL, de vuelta a sRGB para la pantalla.
    FragColor = vec4(a_srgb(color), 1.0);
}
