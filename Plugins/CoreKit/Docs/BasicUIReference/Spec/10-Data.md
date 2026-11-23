# 10 — Datos que consume la UI

La UI muestra siempre datos reales. Los números de los mockups son ilustrativos. Cada dato tiene un dueño en gameplay; el ViewModel del dominio lo expone ([plan §3](../../Architecture/DeadDraw-UI-ImplementationPlan.md)). "Nuevo" = el gancho no existe hoy.

## Run
| Dato | Uso | Fuente |
|---|---|---|
| Tiempo transcurrido y restante hasta el jefe | Timer, Pausa, finales | `UDDRunComponent` |
| Etapa: índice, nombre, subtítulo, fracción | Timer, banner, Pausa, líneas de tiempo | `FDDSpawnStage` + `DisplayName`/`Subtitle` (nuevo) |
| Etapas: 01 INTRO (0–2:00) · 02 PRESSURE "RANGED DRONES INBOUND" (2–5) · 03 SURVIVAL (5–8) · 04 CHAOS "MAX DRONE DENSITY" (8–10) | Textos | datos del schedule |
| Fase (supervivencia, jefe, victoria, derrota) y pausa | Router de pantallas | `UDDRunComponent` |
| Segundos para el jefe (aviso a 03:50) | Evento #12 | Run (nuevo: lead configurable, 10 s) |
| Sobretiempo del jefe | "+mm:ss" | Run |
| Momento y etapa de la muerte, "short by" | Derrota | `UDDRunStatsComponent` (nuevo) |
| Línea de tiempo de la run | Victoria, Derrota | `UDDRunStatsComponent` (nuevo) |

## Jugador
| Dato | Uso | Fuente |
|---|---|---|
| Vida y vida máxima (variable con cartas); umbral de vida baja 30 % | Bloque de vida, Pausa | `UCombatHealthComponent` |
| Dirección y cantidad del daño recibido | Arco de amenaza (#4) | evento `DamageTaken(dir, cantidad)` |
| Posición y orientación del jugador | Minimapa (flecha y cono) | pawn |
| Nivel, XP 0–1, niveles en cola | Barra de XP, Card Draw | `UProgressionComponent` |
| Dash listo, cooldown 0–1 y segundos restantes | Anillo de Dash | Dash (nuevo: `OnDashed`) |
| Munición, cargador (variable con Extended Magazine), reserva ∞, umbral 25 %, recarga y progreso | Bloque de munición | `UCombatEquipmentComponent`, arma |
| Slots de arma (N), nombre, activo | Bloque de armas, cambio | equipo |
| Kills, dashes | Contador, finales | `UDDRunStatsComponent` (nuevo) |
| Pickups (tipo y cantidad: XP por orbe, curación 25) | Feed | `ADDRunPlayer::OnPickup` (nuevo) |

## Cartas
| Dato | Uso | Fuente |
|---|---|---|
| Catálogo (12): nombre, palo, stat/especial, efecto, abreviatura | Cartas, listas | `UProgressionCardDefinition` + `UDDCardPresentation` (nuevo) |
| Arte: `ArtScene`, `ArtEffect`, `ArtFocus` | Cartas y recortes | `UDDCardPresentation` (UI-A) |
| Stacks por carta y **una cadena de efecto sumado por carta** ("Rifle damage +30%"), usada en TAB y Pausa | Mano, TAB, Pausa | consulta en `ADDRunPlayer` (nuevo) + presentación |
| Nombres de arma: corto ("RIFLE", "KATANA") y largo ("ASSAULT RIFLE", "ENERGY KATANA") | HUD (corto), banner de cambio y How to Play (largo) | datos del arma |
| Proyección "OWNED ×N → total" / "NEW → antes → después" | Pie en Card Draw | presentación + stacks |
| Oferta actual (3 cartas) y comando de elección | Card Draw | `UProgressionComponent` |
| Arquetipo de build por dominio ("RIFLE-HEAVY", "BLADE-HEAVY", "DASH-HEAVY", "ARMOR-HEAVY") | TAB, Victoria | regla: el dominio con más copias (nuevo) |

## Enemigos y jefe
| Dato | Uso | Fuente |
|---|---|---|
| Drones vivos por forma (esfera, cubo, pirámide) y posición | Minimapa, "n DRONES" | registro de enemigos (nuevo) |
| Orbes de XP y kits de reparación en el suelo (posición) | Minimapa | registro de pickups (nuevo) |
| Enemigos que apuntan al jugador, progreso de carga y dirección | Anillo CHARGING, lock-on, alerta del minimapa | enemigos (`IsTelegraphing`) |
| Vida 0–1 de cada enemigo dañado | Barra de mundo | salud |
| Tipo de daño (rifle, katana, eléctrico, piercing) | Hit marker, números de daño | combate / FloatingText |
| Jefe: nombre, subtítulo, vida, ataque actual, progreso, centro y radio del telegraph, adds vivos | Barra, advertencia, minimapa | `IDDBossPresentation` (nuevo) |
| Tipo de enemigo que dio el golpe final + consejo | Derrota | `UDDRunStatsComponent` + `UDDDefeatTips` (nuevo) |

## Otros
| Dato | Uso | Fuente |
|---|---|---|
| Nombre de la arena | Pie del minimapa | `UDDArenaMinimapDefinition` (nuevo); el mockup dice "ROOFTOP", pero la arena vigente es otra |
| Planta y límites de la arena | Minimapa | generada por herramienta |
| Versión del build | Pie del menú | config del proyecto |
| Ajustes del usuario | Todo | `UDDUserSettings` + Enhanced Input user settings |
