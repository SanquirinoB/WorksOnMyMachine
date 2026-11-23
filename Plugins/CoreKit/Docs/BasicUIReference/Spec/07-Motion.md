# 07 — Motion

Artboards: `Motion_System` (**fuente de verdad** para tiempos, curvas y audio), `Motion_HUDEvents`, `Motion_LevelUp`, `Motion_Screens`. Implementación con herramientas estándar de UE (Widget Animations, materiales de UI, post-process, time dilation, Camera Shakes, interpolación en `NativeTick`); detalle en §5 del plan.

## Dos relojes
- **Tiempo de combate**: el feedback llega en el frame 0 y solo anima la salida. Nada de más de 150 ms delante de la mira.
- **Tiempo en pausa** (cartas, pausa, finales): 300–700 ms, escalonado, con overshoot.

## Tokens
| Token | Duración | Curva (cubic-bezier) | Uso |
|---|---|---|---|
| SNAP | 90 ms | (.2, .9, .3, 1) | Hit marker, flash de daño, tick de kill |
| SETTLE | 200 ms | (.16, 1, .3, 1) | Banners, líneas de feed, paneles, interpolación de barras |
| SLAM | 380 ms | (.34, 1.56, .64, 1) | LEVEL UP, DEAD., MECH DOWN, etiqueta NEW |
| DEAL | 520 ms + 70 ms de escalonado | (.22, 1, .36, 1) | Cartas, entradas de menú, filas de stats |
| EXIT | 140 ms | (.4, 0, 1, 1) | Todo lo que sale (≈ 0,7× su entrada) |
| PULSE | 1000 ms en loop | sine in-out | Vida baja, anillos de carga, cuenta del jefe |

En UE: `UDDMotionTokens` (duraciones) + `UCurveFloat` por curva; cada valor es editable.

## Reglas
1. **El feedback es instantáneo**: fuerza completa en el frame 0 y luego decae; nunca ease-in.
2. **Entrar desde el ancla**: lo de arriba cae 12 px, lo de abajo sube 12 px, lo lateral entra 24 px hacia dentro; las salidas invierten el movimiento y son más rápidas.
3. **Los números ruedan, las barras se retrasan**: contadores interpolan en 250 ms; el daño deja un chunk blanco que se drena tras 300 ms.
4. **Un héroe a la vez**: los banners hacen cola; un banner de etapa espera mientras se reproduce LEVEL UP.
5. **El glitch está racionado**: separación RGB y jitter solo en daño recibido, entrada del jefe y muerte; máximo 150 ms.
6. **Reduced motion**: un ajuste apaga sacudida, glitch, golpe de FOV, líneas de velocidad y parallax de cartas; los fundidos y cambios de color se mantienen.

## Tabla de tiempos
| Evento | Disparo | Qué se mueve | Duración | Curva | Audio |
|---|---|---|---|---|---|
| Hit marker | Cualquier impacto con daño | 4 trazos escalan 1,5 → 1 y se desvanecen; tinte = arma | 160 ms | SNAP | Tick |
| Número de daño | Impacto | Pop 1,3 → 1, sube 44 px, se desvanece; críticos/especiales con etiqueta | 650 ms | SETTLE | — |
| Kill confirm | Golpe letal | Trazos en X + anillo; el contador hace pop 1,25 → 1 | 260 ms | SNAP | Thunk |
| Daño recibido | Golpe al jugador | Flash rojo en bordes, sacudida 6 px, arco naranja hacia la fuente (1,2 s), la vida baja + drenaje del chunk blanco | 420 ms | SNAP | Impact |
| Vida baja | < 30 % | Pulso de viñeta, bloque de vida en rojo, chip parpadea | loop | PULSE | Heartbeat |
| Pickup XP / curación | Proximidad | La línea de feed entra 24 px; la barra interpola con destello; líneas del mismo tipo se fusionan | 200 ms · 1,8 s | SETTLE | Chime / hum |
| Recarga | R o vacío | El anillo se llena en la mira; la munición rueda; la barra se rellena | tiempo de recarga | LINEAR | Clack |
| Cambio de arma | 1 / 2 / rueda | El nombre entra con slash; la mira cambia cruz ↔ arco; el subrayado del slot se desliza | 160 ms · 1 s | SETTLE | Draw / sheath |
| Dash | Shift | Líneas de velocidad, golpe de FOV 4 %, el anillo se vacía y rellena, ping de listo | 260 ms + CD | SNAP | Whoosh / chime |
| Cambio de etapa | 2:00 · 5:00 · 8:00 | La cinta barre de izquierda a derecha, el banner cae 12 px, se mantiene 2,5 s, sale hacia arriba | 2,6 s | SETTLE / EXIT | Stinger |
| Jefe en camino | 3:50 | Scroll de cinta, cuenta parpadea; a las 04:00 el timer se funde en la barra del jefe, que crece desde 0 | 10 s + 700 ms | DEAL | Alarm |
| Level up | XP llena | Destello de la barra, el título golpea sobre el slash, el mundo se congela y desenfoca, se reparten 3 cartas desde abajo | ≈ 1,4 s | SLAM / DEAL | Riser + hit |
| Carta elegida | Confirmar | Las perdedoras caen y se desvanecen; la ganadora vuela a la mano, aterriza, NEW hace pop, el glow dura 4 s | 900 ms | DEAL | Card snap |
| Pausa | Esc | El blur sube; PAUSED cae; entradas escalonadas 50 ms | 200 / 140 ms | SETTLE / EXIT | Low-pass |
| Muerte | Vida 0 | Glitch 150 ms, 0,4 s de cámara lenta + desaturación, sello DEAD., stats con count-up, botones suben | ≈ 2,2 s | SLAM | Flatline |
| Victoria | Mech 0 % | La barra del jefe estalla, flash blanco, sello MECH DOWN, stats con count-up | ≈ 2,4 s | SLAM | Explosión + fanfarria |

Los detalles de los scripts que la tabla no fija (escalas, desplazamientos, retardos) están en [03-HUDEvents.md](03-HUDEvents.md) y [04-Overlays.md](04-Overlays.md). Donde un script contradice la tabla (por ejemplo, hit marker 180 ms, glitch de muerte 200 ms, glow 3 s), manda la tabla.

## Pantallas
Regla: todo **cambio de pantalla** usa un **barrido con slash magenta de 320 ms DEAL**: línea magenta de 8 px girada 12°, glow `0 0 18px`, que cruza de izquierda a derecha. Los overlays (pausa, level up) nunca barren: desenfocan el mundo. Entre mapas, el barrido es la pantalla de carga de CoreKitLoading.

### Intro del menú
Duración ≈ 1,5 s.
| t (ms) | Paso |
|---|---|
| 0 | El fondo se funde en 400 ms |
| 200 | "DEAD" entra desde la izquierda 80 px, 380 ms SLAM |
| 280 | "DRAW" entra desde la derecha, 380 ms SLAM |
| 600 | Los pernos hacen pop, 300 ms SLAM |
| 650 | La cinta barre, 300 ms SETTLE |
| 800 | El tagline aparece en 300 ms |
| 850 | Las entradas entran desde la izquierda 24 px, 520 ms DEAL + 60 ms de escalonado |
| 1300 | El pie aparece |

### Pausa
- Entrada (200 ms SETTLE): blur 0 → 14 px, escala del mundo 1,03, brillo 0,4, saturación 0,6 (valores finales de `PauseMenu`; el artboard de Motion usa 10 px / 0,45); el título cae 12 px; entradas desde −24 px a los 120 ms con 50 ms de escalonado.
- Salida: 140 ms EXIT, todo junto, sin escalonado.

### Muerte
Duración ≈ 2,2 s.
| t (ms) | Paso |
|---|---|
| 0–150 | Glitch: separación RGB ±12 px y sacudida ±10/8 px (máx. 150 ms, regla 5; el artboard dura ≈ 200 ms) |
| 150–550 | Cámara lenta (time dilation) + escala de grises (brillo 0,35, escala 1,05), 0,4 s; después la pausa terminal |
| 0 → 150 + 700 | La viñeta roja está completa desde el frame 0 y se desvanece en 700 ms tras el glitch |
| 550 | Sello "DEAD." 1,7 → 1, 380 ms SLAM; fantasmas magenta/blanco a ±14 px, 420 ms, que asientan en blanco/rojo a ±8 px ([04-Overlays.md](04-Overlays.md#derrota)) |
| 900 | Subtítulo |
| 1100 | Stats suben 16 px, 520 ms DEAL + 70 ms; el reloj sobrevivido cuenta en 600 ms |
| 1700 | Botones |

### Victoria
Duración ≈ 2,4 s.
| t (ms) | Paso |
|---|---|
| 0 | La barra del jefe estalla en 8 fragmentos, 500 ms SNAP, 15 ms de escalonado |
| 250 | Flash blanco de 300 ms (pico 0,9) |
| 450 | Slash a −5° en 260 ms + sello "MECH DOWN" 1,7 → 1, 380 ms SLAM |
| 800 | Pop del chip, 300 ms |
| 1000 | Stats suben 16 px, 520 ms DEAL + 70 ms; las kills cuentan en 800 ms |
| 1700 | Botones (suben 16 px, 520 ms DEAL) |

### Level up
Secuencia completa en [04-Overlays.md § Secuencia](04-Overlays.md#secuencia-motion--level-up).
