# 04 — Overlays y estados finales

Los overlays (Level Up y Pausa) **no** usan el barrido de pantalla: desenfocan el mundo. Las pantallas finales y los cambios de pantalla sí ([07-Motion.md](07-Motion.md#pantallas)).

## Level Up · Card Draw
Artboards: `CardArt_Draw` (★ aprobado), `Motion_LevelUp` (secuencia), `CardDraw` (fondo y encabezado; su carta está reemplazada).

### Fondo y encabezado
- Mundo congelado: blur 10 px, brillo 0,55, saturación 0,6 (valores de `Motion_LevelUp`), velo oscuro y scanlines. Slash magenta: a −4° en y 470 durante la intro del título; a −3° en y 186 en el estado de elección.
- HUD atenuado a 0,25 y mano a 0,35.
- Chips: "LEVEL n" (lima), "TIME FROZEN" (contorno), "+n DRAW QUEUED" (fondo lima; solo si hay niveles en cola).
- Título: "LEVEL n" / "LEVEL UP · DRAW" (150 px) golpea en la intro; al repartir sube 300 px escalando a 0,6 y se funde en "DRAW ONE" (Saira Condensed 700, 104 px, "ONE" en magenta), que queda durante la elección.

### Cartas
Tres cartas de **360 × 540** centradas con gap de 56 px, en y 290. Anatomía y catálogo en [06-Cards.md](06-Cards.md). Cada carta muestra su tecla (1/2/3, o el glifo del mando) y un pie de proyección:
- Stat ya poseída: "OWNED ×2 → RIFLE DMG +45%".
- Stat nueva: "NEW → MAX HP 100 → 125".
- Especial: "UNIQUE · LEAVES THE POOL".

| Estado | Visual |
|---|---|
| Reposo | brillo 0,82 |
| Hover / foco | sube 10 px, escala 1,02, brillo 1; foco visible: contorno magenta 3 px a 8 px |
| Elegida | inicia la salida: sube 24 px y escala 1,08 en 250 ms SLAM, marco magenta (stat) y glow magenta |
| Descartadas | brillo 0,6 y caen ([Secuencia](#secuencia-motion--level-up)) |

**Inclinación (parallax)** hacia el cursor o el stick: rotateX ±9°, rotateY ±12°, perspectiva 1100 px. La capa de escena (con sobreescala fija de 1,06) se desplaza −10/−7 px y la de efecto +7/+5 px (sentidos opuestos = profundidad); el nombre ±4/3 px; la cinta del marco de las especiales se desplaza con el tilt; brillo radial que sigue al cursor; transición de transform 120 ms ease-out. Reduced motion: sin inclinación ni parallax.

### Elección (decisión D6)
- **Bloqueo de input de 400 ms** al abrir.
- Un **clic** en una carta o las teclas **1/2/3** la eligen al instante (sin paso de confirmación).
- Con mando, el foco recorre las cartas y **A** confirma.
- Pie: "YOUR BUILD" con la mano en minicartas, la indicación "HOVER A CARD · IT TILTS TOWARD THE CURSOR" y "1 2 3 DRAW". Placa de estado: "CLICK A CARD TO DRAW IT" (fondo blanco) antes de elegir; tras elegir pasa a menta: "<NOMBRE> DRAWN · RESUMING". No hay botón de confirmar (D6).
- Un solo commit por oferta; los niveles en cola encadenan una nueva oferta.

### Secuencia (Motion · Level up)
| t (ms) | Paso | Audio |
|---|---|---|
| 120 | La barra de XP se llena en 450 ms con destello | `HUD.LevelUp` |
| 570 | Flash blanco de 280 ms | — |
| 720 | El título golpea: escala 1,35 → 1, 380 ms SLAM; slash con scaleX en 260 ms. Empieza el congelado: filtro del mundo en 300 ms SETTLE, HUD a 0,25 en 250 ms, chip TIME FROZEN en 200 ms | `HUD.LevelUpHit` |
| 1120 | Reparto: 520 ms DEAL + 70 ms de escalonado; las cartas salen de translateY 640, rotadas −18 / 0 / +18°, escala 0,7; el título sube 300 px, escala a 0,6 y se funde en "DRAW ONE" | `Cards.Deal` ×3 (1120, 1190, 1260) |
| 1780 | Estado de elección; empieza el bloqueo de 400 ms | — |
| Hover de una carta | — | `Cards.Hover` |
| Elección, +0 | La ganadora sube 24 px ×1,08 en 250 ms SLAM; las otras caen 300 ms EXIT (`translate(±40px, 320px) rotate(±14°) scale(.9)`, opacidad 0) | `Cards.Pick` |
| Elección, +250 | La ganadora vuela a su lugar en la mano en 500 ms DEAL (rotación 12°, escala 0,17) y aterriza en 420 ms SLAM; NEW aparece 300 ms después del aterrizaje; glow 4 s | `Cards.Land` al aterrizar |
| Al aterrizar | El mundo se descongela en 250 ms; el nivel avanza y la XP sobrante se conserva | — |

## Pausa
Artboard: `PauseMenu`. Entrada 200 ms SETTLE, salida 140 ms EXIT ([07-Motion.md](07-Motion.md#pausa)).
- Mundo: blur 14 px, brillo 0,4, saturación 0,6 (estado final; la animación de entrada sube hasta estos valores), velo `rgba(10,10,12,.6)` con scanlines. Título "SIMULATION HALTED" / "PAUSED".
- Menú (5 entradas, sublabels en JetBrains Mono):
  - RESUME (`ESC`, estilo foco, 74 px de alto, texto 34 px, triángulo) → cierra la pausa.
  - HOW TO PLAY (`CONTROLS`) → se abre encima y vuelve a Pausa.
  - SETTINGS (`INPUT · AUDIO · VIDEO`) → se abre encima y vuelve a Pausa.
  - RESTART RUN (`START OVER`) → confirmación.
  - QUIT TO MENU (`RUN IS LOST`) → confirmación.
- Encabezado "RUN STATUS" y estado de la run en 4 tiles:
  - TIME: tiempo transcurrido + barra de progreso.
  - STAGE: número + nombre ("SURVIVAL").
  - LEVEL: nivel + "% TO NEXT".
  - KILLS: kills + "HULL vida / máx" en menta.
- Lista "YOUR BUILD · n CARDS" con las mismas filas del panel TAB (ícono de arte de 34 px en columna de 44, misma cadena de efecto y mismos colores).
- Pie: navegación (glifos), "TIMER, SPAWNS AND ENEMIES FROZEN".
- Audio: `UI.Open` / `UI.Close` + snapshot de mezcla paso bajo.

## Confirmación destructiva
Prompt modal de 680 px, chaflán 22, con cinta magenta superior de 12 px y etiqueta "CONFIRM", sobre un velo `rgba(10,10,12,.72)`; botones de 64 px.

| Caso | Título | Cuerpo | Botones |
|---|---|---|---|
| Restart | "Restart run?" | "Your build and mm:ss of progress will be lost. A fresh run starts at 00:00." | CANCEL (`ESC`) · RESTART (`ENTER`, danger red) |
| Quit to menu | "Quit to menu?" | "You will lose this run: mm:ss survived, level nn and n cards." | CANCEL · QUIT |
| Quit (Main Menu) | "Quit to desktop?" | — | CANCEL · QUIT |
| Reset de Settings | "Reset to defaults?" | — | CANCEL · RESET |
| Salir de Settings con cambios sin aplicar | "Discard changes?" | — | CANCEL · DISCARD |

Los tres últimos no tienen mockup (**añadidos**); usan el mismo componente y sus cuerpos están pendientes ([11-Decisions.md](11-Decisions.md#pendientes)). La acción Volver (ESC / B) cancela; Enter / A confirma el botón enfocado.

## Victoria
Artboard: `Victory`. Secuencia ≈ 2,4 s ([07-Motion.md](07-Motion.md#victoria)).
- Fondo: "04:00" enorme en contorno y slash magenta.
- Encabezado: chip menta "RUN COMPLETE", "HEAVY MECH DESTROYED", título "MECH DOWN", línea "You outdrew the machine."
- Línea de tiempo: 00:00 · INTRO · PRESSURE · SURVIVAL · CHAOS · 04:00 · "BOSS +mm:ss"; 4 segmentos magenta, 1 naranja del jefe y marcador final menta.
- Stats con count-up: TOTAL TIME, KILLS, FINAL LEVEL, DASHES.
- "FINAL BUILD · n CARDS · <ARQUETIPO>" (n = copias; arquetipo por dominio, p. ej. "RIFLE-HEAVY") con cartas de 76 × 100: recorte del arte, abreviatura y ×N (FP ×4, RF ×3, EM ×1, PIERCE, RICO…).
- Botones: RUN AGAIN (`ENTER`) · MAIN MENU (`ESC`).
- Audio: `End.VictoryJingle` (la explosión del jefe la pone el gameplay).

## Derrota
Artboard: `Defeat`. Secuencia ≈ 2,2 s ([07-Motion.md](07-Motion.md#muerte)).
- Fondo: tinte rojo oscuro, scanlines rojas, bandas de glitch.
- Encabezado: chip rojo "SIGNAL LOST", "HULL INTEGRITY 0%", "DEAD." con copias fantasma (en reposo: blanca y roja a ±8 px, opacidad 0,55; durante la entrada: magenta y blanca a ±14 px, 420 ms), línea "The machine drew first."
- Línea de tiempo: "DOWN AT mm:ss · STAGE nn", relleno rojo con marcador brillante en el punto de muerte, "04:00 · HEAVY MECH" a la derecha.
- Stats: SURVIVED, SHORT BY (rojo), KILLS, LEVEL.
- "BUILD AT DEATH · n CARDS" con cartas de 62 × 82: recorte del arte y ×N; las de acento magenta se recolorean en rojo.
- Tarjeta naranja "FINAL BLOW · <TIPO DE ENEMIGO>" con un consejo por tipo. Literal del mockup para RANGED DRONE: "An orange ring around a drone means it is about to fire. Dash sideways as the ring closes."
- Botones: RETRY (`ENTER · NEW RUN`) · MAIN MENU (`ESC`).
- Audio: `End.Glitch`, `End.DefeatFlatline`, `End.DefeatJingle` — la música de derrota ya la pone `UDDRunMusicSubsystem`; el jingle no debe chocar con ella ([08-Audio.md](08-Audio.md)).
- El rojo es correcto aquí: la derrota es el estado de peligro terminal.
