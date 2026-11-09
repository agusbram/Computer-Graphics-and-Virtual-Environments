# Texturas: origen y licencias

Fuente y licencia de cada imagen que usa el proyecto. Antes de agregar una imagen nueva, anotala acá.

| Archivo | Qué es | Origen | Licencia |
|---|---|---|---|
| `suelo/grass.png` | pasto, 256×256 RGB, *tileable* | aula virtual del curso (Práctico 08) | material de la cátedra |
| `cielo/right.png`, `left.png`, `top.png`, `bottom.png`, `front.png`, `back.png` | cubemap "cielo claro y nubes", 6 × 900×900 | aula virtual del curso (Práctico 08) | material de la cátedra |
| `avion/chapa_color.jpg` | chapa metálica gris con rayones y remaches, 1024×1024 (*Color*) | [ambientCG — Metal Plates 014](https://ambientcg.com/view?id=MetalPlates014) | **CC0** (dominio público) |
| `avion/chapa_normal_gl.jpg` | *normal map* (convención OpenGL) de la misma chapa; **sin usar todavía**: queda para el opcional de *normal mapping* del Práctico 09 | [ambientCG — Metal Plates 014](https://ambientcg.com/view?id=MetalPlates014) | **CC0** |
| `avion/camuflaje.png` | camuflaje de 4 tonos, 1024×1024, *tileable*, con el detalle de la chapa incorporado | generado con `tools/generar_camuflaje.py` a partir de `chapa_color.jpg` | propia del proyecto (deriva de una textura CC0) |
| `prueba/prueba_uv.png` | imagen de prueba asimétrica para verificar coordenadas de textura | generada con `tools/generar_imagen_prueba_uv.py` | propia del proyecto |

La arena del aula (`Practico 8/texturas-suelo/suelo/sand.png`) no se copió: se usa solo pasto. La mezcla de pasto y arena por regiones ("biomas") quedó como extra para más adelante.
