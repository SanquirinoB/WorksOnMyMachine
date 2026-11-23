# 02 — HUD v2 A "Compact Corners"

Artboards: `HUD2_A_CompactCorners` (con safe zone, **elegido**), `HUD` (idéntico sin overlay), `HUDCombat` (Under Pressure), `BossHUD` (Heavy Mech).

## Layout y safe zone
- **Title safe 90 %**: rectángulo de 1728 × 972 con margen **96 px** horizontal y **54 px** vertical. Todo el HUD vive dentro.
- **Fila superior** alineada en y = 54. **Bloques inferiores** comparten una pila de **88 px** (48 número / 8 barra / 16 detalle) que termina en y = 1026.
- Validar en 16:9, 16:10 y 21:9: las esquinas siguen a la safe zone; el centro queda centrado.
- Detrás de cada bloque hay una viñeta radial oscura (`rgba(10,10,12,.45–.6)`), sin paneles opacos.
- Texto del HUD con la sombra `.sh` ([01-DesignSystem.md](01-DesignSystem.md#tipografía)).

| Bloque | Ancla | Posición | Tamaño |
|---|---|---|---|
| Contador de kills | arriba izquierda | left 96, top 54 | alto 40 |
| Timer + etapas | arriba centro | left 760, top 54 | ancho 400 |
| Minimapa | arriba derecha | right 96, top 54 | ancho 200 |
| Vida + nivel | abajo izquierda | left 96, bottom 54 | 300 × 88 |
| Anillo de Dash | abajo izquierda | left 420, bottom 74 | 48 × 48 |
| Mano de build | abajo centro | left 660, bottom 54 | 600 × 88 |
| Panel de build (TAB) | abajo centro | left 700, bottom 150 | ancho 520 |
| Munición + armas | abajo derecha | right 96, bottom 54 | 260 × 88 |
| Mira | centro | 960, 540 | — |

## Bloques
### Contador de kills
Ícono de calavera (cabeza con dos ojos) + número en Saira Condensed 700, 26 px + "KILLS" en JetBrains Mono 10 px (`KeyLight`, tracking 0,18 em). El número sube con un pop al matar ([03-HUDEvents.md](03-HUDEvents.md) #2).

### Timer y etapas
- Fila de 40 px: "STAGE n" (JetBrains Mono 11 px bold, magenta, ancho 72, alineado a la derecha) · reloj "mm:ss" (Saira Condensed 700, 40 px, dígitos tabulares) · "TO MECH" (11 px, `KeyLight`, ancho 72).
- **Cuenta regresiva** hasta el jefe (04:00 − transcurrido).
- Pista de 248 × 14 px, 4 segmentos de 4 px con gap 3 y proporción **2:3:3:2** = etapas 0–0:48, 0:48–2:00, 2:00–3:12, 3:12–4:00. Completos en magenta; el actual parcialmente lleno; pendientes `rgba(242,243,245,.3)`. Ícono naranja del mech al final.
- La etapa 4 (CHAOS) pinta su etiqueta y su segmento en naranja.
- Accesible: rol timer con "Stage n, m minutes s seconds until Heavy Mech".

### Minimapa (200 px)
- Arena completa, **norte arriba**, "N" marcado; obstáculos en gris.
- Jugador: flecha en blanco con cono de visión.
- Drones en naranja con su **forma**: círculo = esfera, cuadrado = cubo, triángulo = pirámide. Un dron que apunta al jugador lleva un **anillo de alerta** pulsante.
- Orbes XP: rombos lima. Kits de reparación: cruz menta.
- Pie: nombre de la arena (JetBrains Mono 10 px, `KeyLight`) y "n DRONES" (naranja).
- Placa `rgba(10,10,12,.62)` con chaflán 14; marco de la arena de 168 px con trazo de 2 px; obstáculos `#5A5D66` / `#3A3D44`; anillo de alerta de radio 8–9 px y trazo 1,5 px.
- Planta estática generada de la arena vigente; íconos actualizados a 10–15 Hz.

### Vida y nivel (300 × 88)
- Fila de 48: ícono de cruz menta + valor (Saira Condensed 700, **52 px**, tabular) + "/ máx" (20 px, `#C9CCD2`).
- Barra de **10 segmentos** de 8 px (gap 3): llenos en menta, el segmento parcial al 0,45 de opacidad (también en vida baja), vacíos `rgba(242,243,245,.25)`.
- Fila de 16: chip "LV n" (fondo lima, texto `Soot`, 800, 13 px) + barra de XP de 3 px en lima.
- Vida baja (< 30 %): número, ícono y segmentos en `DangerRed` + chip "HULL CRITICAL" (ver evento #5).

### Anillo de Dash (48 px)
Anillo magenta con glow (`0 0 12px rgba(255,63,210,.5)`), centro `rgba(20,20,24,.9)` de 40 px con chevron doble. En cooldown: anillo cónico gris que se rellena hasta volver al magenta, con los segundos restantes en el centro ("1.8") en lugar del chevron. Al estar listo: ping ([03-HUDEvents.md](03-HUDEvents.md) #9).

### Munición y armas (260 × 88, anclado a la derecha)
- Valor de munición (Saira Condensed 700, **56 px**, tabular) + "/ cargador" (20 px).
- Barra de 220 × 4 px anclada a la derecha, blanca, con la fracción del cargador.
- Fila de 16: nombre corto del arma ("RIFLE", "KATANA"; JetBrains Mono 10 px) + slots numerados. Slot activo: fondo `rgba(20,20,24,.7)` con subrayado magenta de 2 px; inactivo a 0,55.
- Munición baja (< 25 %): número y barra en magenta + keycap "R" magenta + "LOW" (el bloque compacto usa "LOW"; el aviso grande del evento #7 usa "LOW AMMO").
- **Katana**: muestra "EDGE" en magenta en lugar del número, barra llena y sin máximo.
- Modelo de **N slots** (hoy 2: rifle y katana).

### Mano de build (600 × 88)
- Keycap "TAB" (20 px) a la izquierda, 24 px por encima de la base.
- Abanico de minicartas de **52 × 72** con margen de −3 px por lado (6 px de solape entre vecinas), rotadas en pasos de ≈ 5° alrededor de 0° (−10…+10° con 5 cartas, −12…+13° con 6, −15…+15° con 7; origen 50 % 130 %), con sombra. Ancho del bloque: 600 px con 5 cartas, 640 con 6, 680 con 7. Anatomía de la minicarta: [06-Cards.md § Tamaños pequeños](06-Cards.md#tamaños-pequeños).
- La carta más nueva sube 8 px, lleva glow del color de su palo y la etiqueta "NEW" (texto `Soot` sobre el color del palo, 9 px) durante 4 s.
- La mano muestra cartas distintas; "n CARDS" cuenta copias totales.

### Panel de build (mantener TAB)
- 520 px, fondo `rgba(10,10,12,.82)`, chaflán 12, borde superior magenta de 3 px. El juego **sigue corriendo**.
- Encabezado: "BUILD · n CARDS" (magenta; n = copias totales) y el arquetipo ("RIFLE-HEAVY", por dominio) a la derecha.
- Filas de 30 px en rejilla `22px 1fr auto`, gap 12: miniatura del arte de 22 px · nombre (Saira Condensed 700, 20 px) + "×N" (JetBrains Mono 11 px) o chip "SPECIAL" · efecto sumado (Saira Condensed 600, 18 px).
- Color del efecto sumado: blanco para stats SIGHT, magenta para stats EDGE y VECTOR y para todas las especiales, menta para PLATE. El texto es el mismo que en Pausa (una cadena por carta, [10-Data.md](10-Data.md)).

### Mira
Punto de 4 px + 4 trazos de 2 × 10 px a 12 px del centro, blancos con contorno de 1 px negro al 50 %. Con katana se transforma en **punto magenta + arco** ([03-HUDEvents.md](03-HUDEvents.md) #8). El color de la mira es configurable ([05-FrontEnd.md § Settings](05-FrontEnd.md#settings)); el tinte del hit marker sigue al arma, no al color de la mira.

## Estado Under Pressure (`HUDCombat`)
Mismo layout; muestra simultáneamente: viñeta roja de vida baja, vida 23 en rojo con "HULL CRITICAL", arco de dirección de daño naranja (anillo de 400 px) a la izquierda, indicador de lock-on fuera de pantalla en el borde derecho, banner de etapa "04 CHAOS · MAX DRONE DENSITY" con cinta naranja, feed de pickups ("+25 HP" menta, "+24 XP ×3" lima), munición baja con "R LOW", Dash en cooldown con "1.8", nivel con glow, minimapa con un anillo de alerta. En el mundo: anillo "CHARGING" alrededor de un dron, barra de vida del dron dañado y números de daño ("18", "PIERCE"). Detalle de cada uno en [03-HUDEvents.md](03-HUDEvents.md).

## Estado Heavy Mech (`BossHUD`)
- **Barra del jefe** de 720 px (x 600) **reemplaza al timer**: cuadro de cinta naranja de 28 px, "HEAVY MECH" (Saira Condensed 800, 30 px) + "SIEGE UNIT · FINAL TARGET" (JetBrains Mono 10 px), porcentaje en naranja (Saira Condensed 700, 24 px); barra de 10 px con relleno naranja, **chunk de retardo blanco** al 0,85 y marcas de 2 px `rgba(10,10,12,.9)` cada 72 px.
- Tinte cálido del mundo mientras el jefe está activo (`rgba(40,12,0,.42)` sobre la escena; post-process en UE). El nombre y el subtítulo vienen de la presentación del jefe ([10-Data.md](10-Data.md)).
- Arriba izquierda: kills + chip menta "04:00 SURVIVED" + sobretiempo "+mm:ss".
- **Advertencia de ataque** centrada (y 620): cinta naranja a ambos lados, nombre del ataque ("GROUND SLAM"), instrucción ("GET OUT OF THE RING") y barra de 260 px con el progreso del telegraph. También para Barrage.
- **Minimapa**: el jefe como cuadrado naranja grande con contorno y la elipse del área de slam; pie "MECH + n" (adds vivos, naranja) a la izquierda y "OPTIONAL" (`KeyLight`) a la derecha: matar los adds es opcional.
- En el mundo: anillo de slam naranja en el suelo (anillo sólido + anillo interior discontinuo) — señal de mundo de UI-W.
- La silueta del mockup es un placeholder: el jefe real es el FinalBoss sobre `ADDHeavyMech`.

## Accesibilidad
- Timer: rol timer con la etapa y el tiempo restante en texto.
- Barra del jefe: rol meter, "Heavy Mech health n percent".
- Mano: rol list; cada carta, item con nombre y ×N.
- Banners de etapa: rol status. Advertencias del jefe: rol alert.
- Minimapa: etiqueta con el nombre de la arena y el número de drones.
En UE se traduce a texto accesible de los widgets (`AccessibleText`) donde la plataforma lo soporte.
