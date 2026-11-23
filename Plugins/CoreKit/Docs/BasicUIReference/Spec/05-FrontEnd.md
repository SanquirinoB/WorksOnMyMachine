# 05 — Front end

Mapa `L_MainMenu` como arranque, con la arena vigente de fondo por una cámara cinemática. Cambios de pantalla con el barrido magenta de 320 ms ([07-Motion.md](07-Motion.md#pantallas)). Música del menú: la pista de combate con filtro paso bajo y volumen 0,4 (D4).

## Main Menu
Artboard: `MainMenu`. Intro ≈ 1,5 s ([07-Motion.md](07-Motion.md#intro-del-menú)).
- **Fondo**: la arena con brillo 0,8, degradado oscuro desde la izquierda (1300 px) y desde abajo (300 px).
- **Abanico decorativo** a la derecha (x 1160, y 170, 620 px): tres cartas grandes (SIGHT blanca −16°, PLATE menta +16°, EDGE con cinta magenta al frente) y una barra de brillo magenta a −12°. Con el arte aprobado, las cartas muestran su escena ([06-Cards.md](06-Cards.md)).
- **Bloque de título** (x 120, y 110):
  - "FPS · ARENA SURVIVAL · ROGUELITE" (JetBrains Mono 15 px).
  - Placa del logo (DEAD/DRAW a 150 px) con cinta y "ARENA UNIT DD-10".
  - "SURVIVE 04:00 // DRAW YOUR BUILD // BREAK THE MECH" con los `//` en magenta.
- **Menú** (x 120, y 560, ancho 520, gap 12):

| Entrada | Sublabel | Acción |
|---|---|---|
| PLAY (78 px, estilo foco, triángulo) | NEW RUN | New Game de CoreKitFrontend |
| HOW TO PLAY | CONTROLS · LOOP | How to Play |
| SETTINGS | INPUT · AUDIO · VIDEO | Settings |
| CREDITS | JAM TEAM | Credits (D2) |
| QUIT | DESKTOP | Confirmación, después sale |

  Entradas de 66 px, fondo `Hull`, chaflán 18; hover: `Plate` y texto magenta.
- **Mission brief** (abajo derecha, 420 px): "MISSION BRIEF" + "Hold the arena for 04:00. Collect what the machines drop. Every level, draw one of three cards. When the clock hits zero, Heavy Mech arrives." (04:00 en magenta, Heavy Mech en naranja).
- **Pie** (bottom 44, sobre una línea superior `Plate` ≈ 40 px más arriba): glifos de navegación "NAVIGATE" y "SELECT" según el dispositivo; a la derecha la versión del build ("JAM BUILD 0.1 // UNREAL ENGINE 5", desde la config del proyecto).

## How to Play
Artboard: `HowToPlay`. "FIELD MANUAL" / "HOW TO PLAY"; "ESC BACK" vuelve a quien lo abrió (Main Menu o Pausa). Tres columnas:
1. **Controls**: filas con padding vertical de 11 px y separador (etiqueta Barlow 600 22 px, keycaps de 40 px), cada una con sus keycaps, que reflejan los **bindings actuales** del jugador y el glifo del mando si es el dispositivo activo.
   - Move: W A S D. Look: MOUSE. Jump: SPACE. Attack: LMB. Reload: R. Pause: ESC.
   - Dash: SHIFT + DIR, en magenta.
   - Rifle / Katana: 1 (borde blanco) / 2 (borde magenta).
   - Swap weapon: WHEEL.
   - Inspect build: TAB (**añadido**; D7 hace rebindeables también Aim e Interact, que no se listan aquí porque el loop no los enseña).
   - Nota: "Dash with no direction pressed goes forward. You can attack while dashing." (**añadido**: si el ajuste es BACKWARD, el texto dice "backward").
2. **The loop**: cuatro placas numeradas:
   - 01 FIGHT (blanco): "Rifle at range, katana up close. Switch any time."
   - 02 COLLECT (lima): "XP orbs and heals stay on the floor. Go in and take them."
   - 03 DRAW (magenta): "Fill the XP bar and time stops. Pick one of three cards."
   - 04 SURVIVE (naranja): "At 04:00 Heavy Mech drops in. Destroy it to win the run."

   Debajo, las armas: ASSAULT RIFLE ("Medium range. Full auto. Watch your magazine.") y ENERGY KATANA ("Close range. Heavy damage. Never reloads.").
3. **Read the glow**:
   - LIME · XP ORB: "Dropped by every kill. Fills your next draw."
   - MINT · REPAIR KIT: "Sometimes dropped. Restores 25 HP." El valor sale de los datos.
   - ORANGE · INCOMING: "Charging shots, slam zones. Move or dash."
   - MAGENTA · YOU: "Your blade, your dash, your next card."

   Después, **Card suits**: RIFLE, BLADE, DASH y ARMOR, con su glifo y su color (son SIGHT, EDGE, VECTOR y PLATE).

## Settings
Artboard: `Settings`. "SYSTEM CONFIG" / "SETTINGS".
- **Pestañas** GAMEPLAY | CONTROLS | VIDEO | AUDIO con keycaps Q y E a los lados (LB y RB con mando). La pestaña activa va en blanco con un borde inferior magenta de 4 px.
- **Fila**: 70 px de alto; etiqueta, control de 440 px y valor de 90 px. Pista del slider de 8 px `Seam`; muestras de color de mira de 40 × 40 con `outline` de 3 px a 3 px; la fila Pause muestra "LOCKED".
  - Fila enfocada: fondo `Plate`, inserción magenta de 6 px, relleno del slider y valor en magenta.
  - Segmentado seleccionado: blanco con borde inferior magenta de 3 px.
  - Thumb del slider: magenta, 16 × 30.
- **Panel lateral "SELECTED"** (560 px): título y descripción de la fila enfocada. En Gameplay incluye una vista previa de la mira en vivo.
- **Pie**: `ESC` BACK, `T` RESET TO DEFAULTS (con confirmación) y APPLY. El mockup asigna APPLY a `ENTER`, pero Enter / A activan la fila enfocada (por ejemplo, empezar a escuchar en una fila de rebinding); APPLY pasa a `F` / X-Square (decisión en [11-Decisions.md](11-Decisions.md)).
  - Los cambios se previsualizan en vivo y Apply los confirma.
  - Back con cambios sin aplicar pregunta antes de salir.
  - Back vuelve a quien abrió Settings.
- Con mando, izquierda/derecha cambian el valor de la fila enfocada.

| Pestaña | Fila | Control | Valores (por defecto) |
|---|---|---|---|
| Gameplay | Mouse sensitivity | slider | 0–100 (45; se muestra "0.45") |
| | Invert look Y | segmentado | OFF / ON (OFF) |
| | Field of view | slider | 80–120 (100°) |
| | Dash with no input | segmentado | FORWARD / BACKWARD (FORWARD) |
| | Damage numbers | segmentado | OFF / ON (ON) |
| | Screen shake | slider | 0–100 (60 %) |
| | HUD scale | slider | 75–125 (100 %) |
| | Crosshair colour | 4 muestras | White / Magenta / Lime / Mint (White) |
| | Reduced motion | segmentado | OFF / ON (OFF) — **fila por diseñar en el canvas (D2)** |
| | Gamepad glyphs | segmentado | AUTO / XBOX / PLAYSTATION (AUTO) — añadida |
| Controls | Dash, Jump, Reload, Equip rifle, Equip katana, Swap weapon, Attack, Aim, Interact, Inspect build | rebinding | "PRESS A KEY…"; una tecla en uso se intercambia; Escape cancela (D7) |
| | Pause | bloqueada | ESC, a 0,55 de opacidad |
| Video | Display mode | segmentado | FULLSCREEN / BORDERLESS / WINDOWED (FULLSCREEN) (D8) |
| | Resolution | selector ◀ ▶ | resoluciones del monitor (1920 × 1080) (D8) |
| | Frame rate limit | selector ◀ ▶ | 30 / 60 / 120 / 144 / 240 / Unlimited (144) (D8) |
| | V-Sync | segmentado | OFF / ON (OFF) |
| | Motion blur | segmentado | OFF / ON (OFF) |
| Audio | Master volume / Music / Effects | slider | 0–100 (80 / 60 / 90) |
| | Low-health heartbeat | segmentado | OFF / ON (ON) |

- **Controls**: panel "REBINDING" con el texto "Press any key or mouse button to assign. A key already in use swaps with this action. Escape cancels." Move y Look quedan fijos.
- **Descripciones** (panel SELECTED). Las tres que trae el mockup:
  - Mouse sensitivity: "How fast the camera turns. Lower values make long rifle tracking steadier; higher values make it easier to turn on melee robots behind you." ("melee robots" debe pasar a "drones" con los enemigos vigentes.)
  - Display mode: "Fullscreen gives the lowest input latency. Motion blur ships off: projectiles stay readable during dashes."
  - Master volume: "Scales every channel. Enemy charge-up and boss telegraph sounds are never ducked by music."
  - El resto está pendiente ([11-Decisions.md](11-Decisions.md#pendientes)).

## Credits
**Pendiente de diseño (D2).** Contenido mínimo:
- Equipo de la jam.
- Atribuciones de terceros: Kenney (CC0, se acredita aunque no sea obligatorio) y las fuentes OFL con su licencia empaquetada.
- Herramientas (**añadido**).

Usa el design system y el barrido de pantalla; Back vuelve al menú.
