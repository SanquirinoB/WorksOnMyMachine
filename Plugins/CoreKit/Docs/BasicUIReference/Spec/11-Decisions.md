# 11 — Decisiones, inconsistencias resueltas y pendientes

## Decisiones (aprobadas por el usuario, 2026-11-15)
| # | Tema | Decisión |
|---|---|---|
| — | Design system | I — Rooftop Works |
| — | HUD | v2 A — Compact Corners, con safe zone 96/54 |
| — | Navegación | CoreKitNavigation extendido reemplaza a CommonUI |
| — | Cartas | Imagen principal = escena renderizada del efecto; ícono en insignia (fila 06) |
| D1 | Color de XP | Lima. Se recolorea la emisión del cristal de XP |
| D2 | Credits y Reduced motion | Se diseñan en el canvas con el DS I antes de la etapa de front end |
| D3 | Sonidos | Página de escucha; el usuario aprueba o pide cambio por cue antes de cerrar la etapa de eventos |
| D4 | Música del menú | Pista de combate con paso bajo y volumen 0,4 |
| D5 | Herramientas de autoría | Instalar BlueprintSemanticExtractor y registrar el MCP de Unreal para este proyecto; si no, commandlet generador |
| D6 | Elegir carta | Clic o 1/2/3 eligen al instante tras 400 ms de bloqueo; con mando, foco + A. Sin botón "SPACE · DRAW": la placa de estado solo informa |
| D7 | Rebinding | Añadir Attack, Aim, Interact e Inspect build; Move y Look fijos; Pause bloqueada |
| D8 | Video | Display mode FULLSCREEN / BORDERLESS / WINDOWED; selectores de resolución y límite de FPS (30/60/120/144/240/Unlimited) |

## Inconsistencias del canvas y su resolución
Regla general: la tabla de `Motion_System` manda en tiempos, curvas y audio.

**Aceptadas por el usuario el 2026-11-15**: todas las resoluciones de esta tabla, incluidas las que cambian el mockup (APPLY de Settings en F / X-Square, arquetipo por dominio, tamaños de minicarta por pantalla, una cadena de efecto por carta y el prompt "Discard changes?").

| Tema | Variantes en el canvas | Resolución |
|---|---|---|
| Hit marker | 80 / 160 / 180 ms | 160 ms SNAP |
| Kill confirm | 200 / 260 / 280 + 340 ms; pop 1,35 SLAM vs 1,25 SNAP | 260 ms SNAP, pop 1,25 |
| Número de daño | 600 ms 40 px / 650 ms 44 px pop 1,3 / 650 ms 48 px pop 1,35 | 650 ms, 44 px, pop 1,3 |
| Feed de pickups | 1,5 / 1,8 s | 1,8 s |
| Cambio de arma | 800 ms / 160 ms · 1 s / 1100 ms + 500 ms | 160 ms la mira, 1 s el nombre |
| Ping de Dash, FOV, sacudida | 150 / 420 ms; 4 / 4,5 %; 6 / 8 px | 420 ms el ping (detalle de script), FOV 4 %, sacudida 6 px |
| Level up | 400 ms → cartas / ≈ 1,4 s / 1660 ms | ≈ 1,4 s en la tabla; la secuencia detallada de `Motion_LevelUp` (hasta 1780 ms) es la referencia de pasos |
| Banner de etapa | 2,5 / 2,6 s | 2,6 s total (2,5 s en pantalla) |
| Glow de carta nueva | 4 / 3 s | 4 s |
| Glitch y cámara lenta de muerte | 150 ms / 0–200 ms; 0,4 s / 700 ms | 150 ms de glitch y 0,4 s de cámara lenta; los pasos posteriores mantienen sus tiempos del artboard |
| Flujo de elección de carta | Seleccionar + confirmar vs un clic | D6 |
| Tamaño de carta | 360 × 540 / 300 × 440 | 360 × 540 |
| Título de Card Draw | "DRAW ONE" / "LEVEL UP · DRAW" | "LEVEL UP · DRAW" en la intro, "DRAW ONE" al elegir |
| Banner del jefe | 520 px vs límite de 440 | Excepción: 520 px |
| Cuenta del jefe | 00:08 / 00:03 | Valor real de la cuenta |
| Entradas de Pausa | 5 / 4 | 5 (incluye HOW TO PLAY) |
| Nombres de palo | SIGHT/EDGE/VECTOR/PLATE vs RIFLE/BLADE/DASH/ARMOR | Los primeros en cartas; How to Play muestra el dominio |
| Cinta de especiales | Texto "red tape" vs dibujo magenta | Magenta (el texto es errata) |
| Rojo en Derrota | El DS limita el rojo | Se permite: la Derrota es el estado de peligro terminal |
| Back de Settings / How to Play | Siempre al Main Menu | Vuelve a quien abrió la pantalla |
| QUIT y RESET sin confirmación | — | Ambos con prompt de confirmación |
| Colores de mira vs señales | Magenta, lima y menta chocan | Se ofrecen; el hit marker sigue al arma |
| Katana en el bloque de munición | Solo en un script | "EDGE" en magenta, barra llena, sin máximo |
| Valores de mockup (30/36/46 balas, Dash 3 s, vida 150) | Distintos entre artboards | Datos reales; "EM" = Extended Magazine |
| Reloj | Regresivo en HUD, transcurrido en el resto | Ambos, según la pantalla |
| Placeholder del jefe | Silueta | FinalBoss real |
| Blur de Pausa | 14 px / 0,4 (`PauseMenu`) vs 10 px / 0,45 (`Motion_Screens`) | 14 px, brillo 0,4, saturación 0,6 |
| Munición baja | "LOW" (`HUDCombat`) vs "LOW AMMO" (`HUDEvents`) | "LOW" en el bloque compacto; "LOW AMMO" en el aviso grande junto a la mira |
| Arquetipo | "RIFLE-HEAVY" vs "SIGHT-HEAVY" | Por dominio: RIFLE / BLADE / DASH / ARMOR-HEAVY |
| Minicartas | 52 × 72, 62 × 82, 76 × 100, 40 × 52; marco del palo vs `Plate`; cinta de 6 vs 8 px | Tamaño por pantalla (mano 52 × 72, Derrota 62 × 82, Victoria 76 × 100); marco `Plate` de 2 px y cinta de 8 px (`CardArt_Card`) |
| Carta nueva en la mano | 900 ms con NEW a 700 (`Motion_HUDEvents`) vs vuelo 500 + aterrizaje 420 + NEW a +300 (`Motion_LevelUp`) | Vuelo 500 ms DEAL + aterrizaje 420 ms SLAM; NEW con pop a los 700 ms; glow 4 s desde 900 ms |
| Carta elegida | ×1,04 (estado de `CardArt_Draw`) vs ×1,08 (`Motion_LevelUp`) | Con D6 no hay estado "elegida" estático: la ganadora sube 24 px ×1,08 en 250 ms |
| Hover de carta | −12 px ×1,03 perspectiva 1000 (`Motion_LevelUp`) vs −10 px ×1,02 perspectiva 1100 (`CardArt_Draw`) | `CardArt_Draw` (aprobado) |
| Congelado del mundo | brillo 0,5 / 0,55; saturación 0,85 en `CardDraw` | blur 10 px, brillo 0,55, saturación 0,6 |
| Título de Card Draw durante la elección | "LEVEL UP · DRAW" a 0,6 vs "DRAW ONE" | El título se funde en "DRAW ONE" al repartir |
| Anillo de Dash en cooldown | Cónico gris con segundos vs trazo magenta que se rellena | Cónico gris con segundos, vuelve a magenta al estar listo |
| Carta más nueva en la mano | Sube 2 / 6 / 8 / 14 px | 8 px |
| Fantasmas de DEAD. | Blanco/rojo ±8 px vs magenta/blanco ±14 px | Magenta/blanco en la entrada; asientan en blanco/rojo |
| Cadenas de efecto sumado | "Rifle dmg +30%" (TAB) vs "Rifle damage +30%" (Pausa) | Una cadena por carta, forma larga, en ambos |
| Nombres de arma | RIFLE / KATANA / ASSAULT RIFLE / ENERGY KATANA | Corto en el HUD; largo en el banner de cambio y How to Play |
| Flash de level up | etiqueta 200 vs script 280 ms | 280 ms |
| Formatos numéricos | "LV 7" / "STAGE 3" en HUD vs "07" / "03" en el resto | Sin ceros en el HUD; dos dígitos en Pausa, Victoria y Derrota |
| Alerta del minimapa | Cualquier dron que apunta vs desde detrás / fuera de pantalla | Desde detrás o fuera de pantalla (texto de `HUDEvents`) |
| APPLY de Settings | `ENTER` APPLY en el pie, pero Enter también activa filas | APPLY pasa a F / X-Square; Enter / A activan la fila enfocada |
| Glifo de Ricochet | Caret / rombo | Glifo del palo SIGHT en la insignia; la escena distingue la carta |
| Chip TIME FROZEN sobre el contador de kills | — | Se mantiene (el HUD está atenuado) |
| Sonidos y glifos ausentes en el canvas | Navegación, reparto/hover/elección de cartas, golpe de LEVEL UP, glitch y jingle de muerte; glifos de mando | Se añaden según [08-Audio.md](08-Audio.md) y [09-Input.md](09-Input.md) |

## Pendientes
| Pendiente | Responsable | Antes de |
|---|---|---|
| Diseñar la pantalla Credits en el canvas | diseño | etapa de front end |
| Diseñar la fila Reduced motion en el artboard Settings | diseño | etapa de front end |
| Escribir la descripción de cada fila de Settings (hoy hay una por pestaña) | diseño / texto | etapa de front end |
| Cuerpos de los prompts "Quit to desktop?", "Reset to defaults?" y "Discard changes?" | texto | etapa de front end |
| Actualizar la descripción de Mouse sensitivity ("melee robots" → drones) | texto | etapa de front end |
| Confirmar si How to Play lista Aim e Interact | diseño | etapa de front end |
| Validar las abreviaturas añadidas SB, EB, SW | diseño | arte de cartas |
| Aprobar los 26 sonidos propuestos (D3) | usuario | cierre de la etapa de eventos |
| Confirmar los valores de Extended Magazine y Swift Blade en los datos | datos | arte de cartas |
| Mapeos de mando de partida (tabla de [09-Input.md](09-Input.md)) | input | modelo de UI |
| Consejos de Derrota por tipo de enemigo (esfera, cubo, pirámide, melee, ranged, jefe) | texto | etapa de finales |
| Arte final de las 12 cartas (renders en UE) | arte | Card Draw |
