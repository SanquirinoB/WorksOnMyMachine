# 03 — Eventos del HUD y señales de mundo

Artboards: `HUDEvents` (15 eventos + 1 tarjeta de regla de layout), `Motion_HUDEvents` (12 disparadores de demo), `HUDCombat`. Duraciones y curvas según la tabla de [07-Motion.md](07-Motion.md); los detalles finos según el script de `Motion_HUDEvents`. Cues de audio en [08-Audio.md](08-Audio.md).

**Reglas comunes** ([07-Motion.md § Reglas](07-Motion.md#reglas)): el feedback de combate llega con fuerza completa en el frame 0 y solo anima la salida; nada de más de 150 ms delante de la mira; un héroe a la vez (los banners esperan a que termine LEVEL UP); el glitch solo en daño recibido, entrada del jefe y muerte.

| # | Evento | Disparo | Qué se ve | Tiempo | Audio |
|---|---|---|---|---|---|
| 1 | **Hit marker** | Cualquier impacto propio que hace daño | 4 trazos diagonales escalan 1,5 → 1 y se desvanecen; tinte por arma: rifle blanco, katana magenta | 160 ms SNAP | `HUD.Hit` |
| 2 | **Kill confirm** | Golpe letal | Trazos en X se expanden (0,6 → 1,5) + anillo (0,3 → 1,7); el contador de kills hace pop 1,25 → 1; el enemigo estalla en orbes XP | 260 ms SNAP | `HUD.Kill` |
| 3 | **Números de daño** | Impacto | En el mundo; Saira Condensed 800, 40 px, sombra `0 2px 0 #0A0A0C`; pop 1,3 → 1, suben 44 px y se desvanecen. Color por fuente: rifle blanco, katana magenta, eléctrico blanco brillante con etiqueta "ZAP"; Piercing añade "PIERCE" (Saira Condensed 20 px, magenta). Se apagan con el ajuste Damage numbers | 650 ms SETTLE | — |
| 4 | **Daño recibido / dirección de amenaza** | El jugador recibe daño | Flash rojo en bordes (`inset 0 0 200px 40px rgba(255,59,78,.7)`, pico al 12 %), sacudida de 6 px durante 180 ms lineal, arco naranja hacia el atacante (1,2 s: se mantiene hasta el 65 % y se desvanece), la vida baja en 80 ms y deja un chunk blanco que se drena 420 ms después de 300 ms; jitter del número 220 ms. Fuera de pantalla: chevron naranja en el borde + anillo de carga | 420 ms SNAP | `HUD.DamageTaken` |
| 5 | **Vida baja** | Vida < 30 % | Viñeta roja pulsante a 1 Hz, bloque de vida en rojo, chip "HULL CRITICAL" parpadea 1 s; se limpia en cuanto una curación la sube por encima de 30 % | PULSE (loop) | `HUD.LowHealthHeartbeat` (si el ajuste está activo) |
| 6 | **Feed de pickups** | Recoger XP o curación | Líneas sobre el bloque de vida; entran deslizando 24 px; viven 1,8 s; las del mismo tipo en menos de 1 s se fusionan y muestran el total y el conteo ("+24 XP ×3"); máximo 3 líneas. La barra afectada interpola 300 ms con un destello de 600 ms; la curación hace count-up en 350 ms | 200 ms entrada · 1,8 s vida, SETTLE | XP: lo pone el gameplay. Curación: `HUD.PickupHeal` |
| 7 | **Recarga** | R o cargador vacío (recarga automática) | Anillo que se llena alrededor de la mira con "RELOADING"; la munición sube con un roll de 250 ms al final; bajo 25 % aparece "R LOW AMMO" en magenta | tiempo real de recarga, LINEAR | Lo pone el arma (CombatKit) |
| 8 | **Cambio de arma** | 1 / 2 / rueda | Barrido con slash del color del arma sobre el módulo de munición: "n · EQUIPPED / NOMBRE"; la mira cambia cruz ↔ punto + arco (escala 0,3 y gira 45°); el subrayado del slot se desliza | 160 ms la mira · 1 s el nombre, SETTLE | Lo pone el gameplay (draw/sheath) |
| 9 | **Dash** | Dash | Líneas de velocidad 300 ms, golpe de FOV del 4 %, el anillo se vacía y se rellena con el cooldown (segundos en el centro), ping al estar listo (anillo 1 → 1,9 y destello). Con Static Dash, anillo blanco de choque al usarlo | 260 ms + cooldown, SNAP | Whoosh: gameplay. Listo: `HUD.DashReady` |
| 10 | **Level up** | Barra de XP llena | Destello de la barra, el título "LEVEL UP · DRAW" golpea sobre un slash, el mundo se congela, se reparten 3 cartas. Los niveles en cola encadenan otro reparto | ≈ 1,4 s SLAM/DEAL (secuencia completa en [04-Overlays.md](04-Overlays.md)) | `HUD.LevelUp`, `HUD.LevelUpHit` |
| 11 | **Cambio de etapa** | 2:00, 5:00, 8:00 | Banner de 40 px en y 132, máximo 440 px: cinta que barre de izquierda a derecha en el primer 10 %, entra cayendo 12 px (0–8 %), se mantiene (8–88 %, ≈ 2,1 s) y sale hacia arriba 8 px (88–100 %); el segmento de la pista se llena en 600 ms. Ej.: "02 PRESSURE · RANGED DRONES INBOUND"; CHAOS en naranja | 2,6 s SETTLE / EXIT | `HUD.StageChange` |
| 12 | **Jefe en camino** | 03:50 | Los spawns se reducen; el banner pasa a cuenta atrás de peligro "WARNING · HEAVY MECH INBOUND 00:0n" (520 px, excepción a 440), cinta en scroll de 0,6 s, cuenta que parpadea cada 0,5 s; banner de 44 px de alto en x 700, cinta de 22 px; a las 04:00 el timer se funde y entra la barra del jefe (cae 12 px en 300 ms SETTLE) y crece desde 0 en 700 ms DEAL | 10 s + 700 ms | `Boss.InboundAlarm` |
| 13 | **Build · carta nueva** | Elegir carta | La carta vuela del centro a su lugar en la mano (500 ms DEAL, escala → minicarta) y aterriza (420 ms SLAM), queda 8 px más alta con glow del palo; etiqueta NEW con pop de 380 ms SLAM a los 700 ms; glow de 4 s desde los 900 ms; si ya existía, su ×N hace pop | ≈ 900 ms, glow 4 s | `Cards.Land` |
| 14 | **Build · inspección TAB** | Mantener `IA_InspectBuild` | El panel de 520 px sube; el juego sigue; al soltar se cierra | SETTLE / EXIT | — |
| 15 | **Alerta en minimapa** | Un dron fija al jugador desde detrás o fuera de pantalla | El ícono mantiene su forma; un anillo naranja pulsa sobre él, en espejo con el chevron del borde (#4) | PULSE mientras apunta | — |

### Geometría de los eventos
| Evento | Geometría |
|---|---|
| Hit marker | 4 trazos de 11 × 3 px a 45° alrededor de la mira |
| Kill confirm | Trazos de 18 × 4 px en X y anillo de 88 px con trazo de 2 px |
| Recarga | Anillo de 64 px con trazo de 3 px alrededor de la mira; "RELOADING" 10 px a +40 px |
| Feed de pickups | En left 96, bottom 158; líneas de 28 px con gap 4, Saira Condensed 700 20 px, fondo `rgba(10,10,12,.7)`, chaflán 6; la línea más vieja a 0,7 |
| Lock-on fuera de pantalla | Placa en right 96, top 476, 40 px de alto |
| Cambio de arma | Banner en right 96, bottom 158; nombre Saira Condensed 800 38 px; slash de 260 × 3 px |
| Vida baja | Viñeta `inset 0 0 240px 30px rgba(255,59,78,.48)` |

Regla de layout (tarjeta 16 de `HUDEvents`): todo dentro del 90 % title-safe; fila superior en y 54; bloques inferiores de 88 px (48/8/16) hasta y 1026.

## Señales de mundo (UI-W)
| Señal | Spec |
|---|---|
| Barra de vida de enemigo dañado | 100 × 6 px, naranja, sobre el enemigo; aparece al recibir daño; baja en 90 ms y deja un chunk blanco que se drena 420 ms tras 300 ms |
| Anillo de carga (CHARGING / LOCKING ON) | Anillo naranja de ~280 px alrededor del dron que carga, con relleno cónico según el progreso y etiqueta "CHARGING"; fuera de pantalla, placa con flecha, anillo cónico y "LOCKING ON" en el borde |
| Indicador de Ground Slam | Anillo naranja sólido en el suelo + anillo interior discontinuo, con el radio real del ataque |
| Orbes de XP | Lima `#B8FF3A` (decisión D1: se recolorea la emisión del cristal) |
| Kits de reparación | Menta `#4DFFC3` |
| Proyectiles enemigos | Naranja `#FF7A1A` |
