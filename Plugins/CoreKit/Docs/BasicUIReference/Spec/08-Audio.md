# 08 — Audio de UI

Fuente de los eventos: columna Audio de la tabla de `Motion_System`, ampliada con cues **añadidos** (navegación `UI.*`, `Cards.Deal/Hover/Pick`, `HUD.LevelUpHit`, `End.Glitch`, `End.DefeatJingle`). Origen y licencia de cada archivo: `ArtSource/ThirdParty/Manifest.json` (Kenney CC0) y `ArtSource/Audio/UI/Generated/` (sintetizados por `Tools/Audio/SynthesizeUICues.py`). En UE: `UDDUISounds` mapea cue → `USoundBase`; clase de sonido `UI`.

**Regla**: la UI no duplica sonidos que el gameplay ya reproduce (`/Game/DeadDraw/Audio/SFX`, audio de armas de CombatKit, música de `UDDRunMusicSubsystem`). Esos cues figuran como `superseded-by-gameplay`.

Las filas marcadas "gameplay" tienen un archivo propuesto en el manifiesto (`superseded-by-gameplay`) que **no se importa**.

**Estado**: todos los cues propios están **propuestos**; requieren la aprobación del usuario por escucha (D3) antes de cerrar UI-5.

## Navegación
| Cue | Evento | Archivo |
|---|---|---|
| `UI.Hover` | Hover/foco de botón | UI Audio `rollover1.ogg` |
| `UI.Confirm` | Clic / confirmar | Interface Sounds `confirmation_001.ogg` |
| `UI.Select` | Cambio de pestaña u opción | Interface Sounds `select_001.ogg` |
| `UI.Back` | Volver / cancelar | Interface Sounds `back_001.ogg` |
| `UI.Error` | Acción inválida: activar la fila Pause bloqueada, elegir carta durante el bloqueo de 400 ms, tecla rechazada en el rebinding | Interface Sounds `error_001.ogg` |
| `UI.Toggle` | Segmentado de Settings | Interface Sounds `toggle_001.ogg` |
| `UI.Open` / `UI.Close` | Abrir/cerrar pausa o modal (+ snapshot de mezcla paso bajo) | Interface Sounds `open_001.ogg` / `close_001.ogg` |

## HUD
| Cue | Evento | Archivo | Estado |
|---|---|---|---|
| `HUD.Hit` | Hit marker (Tick) | Interface Sounds `tick_001/002.ogg` (aleatorio) | propuesto |
| `HUD.Kill` | Kill confirm (Thunk) | Impact Sounds `impactPunch_heavy_000/001.ogg` | propuesto |
| `HUD.DamageTaken` | Daño recibido (Impact) | Impact Sounds `impactPlate_heavy_000/001.ogg` | propuesto |
| `HUD.LowHealthHeartbeat` | Vida baja (loop) | `UI_LowHealth_Heartbeat_Loop.wav` (sintetizado) | propuesto; se apaga con el ajuste Low-health heartbeat |
| `HUD.PickupHeal` | Curación (hum) | Sci-fi Sounds `forceField_000.ogg` | propuesto |
| `HUD.PickupXP` | XP (chime) | — | gameplay (`SC_Pickups_XP`) |
| `HUD.ReloadUI` | Recarga (clack) | — | gameplay (CombatKit) |
| `HUD.WeaponSwap` | Draw / sheath | — | gameplay (`SC_Weapons_PistolDraw`) |
| `HUD.Dash` | Whoosh | — | gameplay (`SC_Player_Dash`) |
| `HUD.DashReady` | Dash listo (chime) | Digital Audio `twoTone1.ogg` | propuesto |
| `HUD.StageChange` | Stinger de etapa | Music Jingles `jingles_STEEL00.ogg` | propuesto |
| `HUD.LevelUp` | Riser de level up | Digital Audio `powerUp1.ogg` | propuesto |
| `HUD.LevelUpHit` | Golpe del título | Music Jingles `jingles_HIT00.ogg` | propuesto |

## Cartas
| Cue | Evento | Archivo |
|---|---|---|
| `Cards.Deal` | Reparto (uno por carta, escalonado) | Casino Audio `card-slide-1/2/3.ogg` |
| `Cards.Hover` | Hover de carta | Casino Audio `card-fan-1.ogg` |
| `Cards.Pick` | Carta elegida | Casino Audio `card-place-1.ogg` |
| `Cards.Land` | La carta llega a la mano | Casino Audio `card-shove-1.ogg` |

## Jefe y finales
| Cue | Evento | Archivo | Estado |
|---|---|---|---|
| `Boss.InboundAlarm` | Jefe en camino (03:50, loop hasta 04:00) | `UI_Boss_Inbound_Alarm_Loop.wav` (sintetizado) | propuesto |
| `Boss.Telegraph` | Aviso de ataque | — | gameplay (`SC_Boss_Telegraph`) |
| `End.VictoryBoom` | La barra del jefe estalla | — | gameplay (`SC_Boss_Death`) |
| `End.VictoryJingle` | Fanfarria de MECH DOWN | Music Jingles `jingles_STEEL01.ogg` | propuesto |
| `End.Glitch` | Glitch de muerte | Interface Sounds `glitch_001.ogg` | propuesto |
| `End.DefeatFlatline` | Flatline de muerte | `UI_Death_Flatline.wav` (sintetizado) | propuesto |
| `End.DefeatJingle` | Golpe de DEAD. | Music Jingles `jingles_PIZZI00.ogg` | propuesto; **revisar** contra la música de derrota que ya entra al morir |

## Mezcla
- Volúmenes de usuario Master / Music / Effects ([05-FrontEnd.md](05-FrontEnd.md#settings)) sobre las clases de CoreKitFrontend; la UI va en Effects.
- Los sonidos de carga de enemigos y de telegraph del jefe **nunca** se atenúan por la música.
- Pausa: snapshot paso bajo sobre el mundo mientras está abierta.
- Música del menú: pista de combate con paso bajo y volumen 0,4 (D4).
