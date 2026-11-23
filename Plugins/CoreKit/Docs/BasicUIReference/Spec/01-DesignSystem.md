# 01 — Design system: Rooftop Works (I)

Artboard: `DS_I_Rooftop`. Estructura de Hazard Works (placas, stencil, cintas) con colores muestreados de la captura real del juego. Regla de color: **el magenta es del jugador, el naranja de los drones**; el cian y el rosa neón quedan en el mundo, nunca en el HUD.

## Colores
### Marca
| Token | Hex | Uso |
|---|---|---|
| `Brand.Magenta` (Weapon Magenta) | `#FF3FD2` | Marca, borde de foco, katana, Dash, todo lo del jugador |
| `Brand.White` (Signal White) | `#F2F3F5` | Texto del HUD, rifle, relleno de foco |
| `Brand.MagentaDark` (perno) | `#A01488` | Pernos sobre placas magenta |

### Neutros
| Token | Hex | Uso |
|---|---|---|
| `Neutral.Soot` | `#0A0A0C` | Fondo base, texto sobre claros |
| `Neutral.Hull` | `#141418` | Paneles, botón por defecto |
| `Neutral.Plate` | `#222328` | Placas, botón hover, fila enfocada |
| `Neutral.Seam` | `#3A3D44` | Separadores, pistas vacías |
| `Neutral.Steel` | `#9A9DA5` | Etiquetas secundarias |
| `Neutral.KeyLight` | `#E6E8EC` | Etiquetas del HUD |
| Texto secundario | `#B9BCC3`, `#C9CCD2` | Cuerpo atenuado, "/ máx" |
| `Neutral.Muted` | `#8E9199` | Sublabels de menú, encabezados de sección, pies |
| `Neutral.Shade` | `#5A5D66` | Anillo de pernos, texto de la etiqueta derecha en botones con foco, obstáculos claros del minimapa |
| `Neutral.Divider` | `#2A2C31` | Divisores de sección |

### Señales de gameplay
| Token | Hex | Uso | Prohibido |
|---|---|---|---|
| `Signal.DroneOrange` | `#FF7A1A` | Enemigos, disparos enemigos, anillos de slam, jefe, amenazas | Nada del jugador |
| `Signal.XPLime` | `#B8FF3A` | Orbes XP, barra y chip de nivel, cola de draws | — |
| `Signal.RepairMint` | `#4DFFC3` | Vida, kits de reparación, palo PLATE, estados de éxito (DRAWN, RUN COMPLETE) | — |
| `Signal.DangerRed` | `#FF3B4E` | Vida baja y confirmaciones destructivas; pantalla de Derrota | **Nunca en enemigos** |

### Cintas (hazard tape)
- **Magenta/negra** (`-45°`): sistema y jugador — cartas especiales, diálogos de confirmación, logo. Ancho de franja por uso: 16 px en el DS, 14 px en el logo del menú y en el diálogo de confirmación (magenta sobre `#141418`), 8 px en cartas y minicartas.
- **Naranja/negra**: amenaza de drones — zonas de slam, lock-on, banner CHAOS, jefe. Franja de 8 px en banners del HUD y 10 px en el banner del jefe.
- La cinta dice de quién es la señal: nunca se mezclan.

### Palos de carta
| Palo | Dominio | Color |
|---|---|---|
| SIGHT | Rifle | `#F2F3F5` |
| EDGE | Katana | `#FF3FD2` |
| VECTOR | Dash | `#FF3FD2` |
| PLATE | Armadura / vida | `#4DFFC3` |

**Acento de las especiales**: toda carta especial usa magenta como acento (etiqueta, contorno de insignia, palabra clave, tinte y banda), aunque su palo sea SIGHT. El glifo del palo conserva su color.

## Tipografía
| Rol | Familia | Pesos | Uso |
|---|---|---|---|
| Display | Saira Condensed | 500–800 | Títulos, números del HUD, nombres de carta, botones |
| Logo | Saira Stencil One | 400 | Logo, monograma |
| Cuerpo | Barlow Semi Condensed | 400–700 | Párrafos, descripciones |
| Etiquetas y teclas | JetBrains Mono (estática) | 400/500/700 | Etiquetas en mayúsculas, keycaps, datos |

- Números del HUD con **dígitos tabulares** (en UE: celdas de ancho fijo, `UDDTabularNumber`).
- Etiquetas en mayúsculas, tracking amplio (`letter-spacing` 0,1–0,32 em según el tamaño → `LetterSpacing` de `FSlateFontInfo`).
- Sombra de legibilidad del HUD (`.sh`): `0 1px 2px rgba(0,0,0,.9), 0 0 10px rgba(0,0,0,.6)`.

## Formas
- **Chaflán superior izquierdo a 45°** en todo panel, botón y carta: 7–28 px según tamaño (botones de menú 14–20, botones de modal 16, segmentados y pestañas 8–14, paneles 10–18, diálogo de confirmación 22, carta 22–24, minicarta 7–8).
- **Pernos**: círculos de 10 px, `Soot` con anillo interior de 2 px `#5A5D66`, o `MagentaDark` sobre magenta.
- Placas sin bordes redondeados; los únicos círculos son anillos (Dash, carga, telegraph) y pernos.
- Viñetas radiales oscuras (`rgba(10,10,12,.45–.6)`) detrás de cada bloque del HUD.

## Botones
| Estado | Fondo | Texto | Detalle |
|---|---|---|---|
| Default | `Hull` | `White` | — |
| Hover | `Plate` | `White` | inserción magenta de 6 px a la izquierda. En las listas de Main Menu y Pausa el hover es `Plate` con **texto magenta y sin inserción** |
| Focus | `White` | `Soot` | inserción magenta de 8 px (10 px en el PLAY del menú) |
| Confirm / destructivo | `Magenta` (sistema) o `DangerRed` (destructivo) | `Soot` | — |
| Bloqueado / inactivo (**añadido**: no hay estado disabled en el DS) | opacidad 0,55, como la fila Pause de Settings y el slot de arma inactivo | — | — |

Altura 60 px en DS, 66–78 px en Main Menu. Etiqueta derecha opcional en JetBrains Mono 12–13 px (`NEW RUN`, `CONTROLS · LOOP`…). Hover y foco son el mismo "seleccionado" (un solo elemento seleccionado a la vez).

## Chips
| Chip | Estilo |
|---|---|
| `STAT · STACKS` | fondo `Plate`, texto `#B9BCC3` |
| `SPECIAL · UNIQUE` | fondo magenta, texto `Soot`, bold |
| `INCOMING` | contorno naranja 1 px, texto naranja |
| Estado (`TIME FROZEN`, `RUN COMPLETE`, `SIGNAL LOST`, `HULL CRITICAL`, `04:00 SURVIVED`) | contorno o relleno del color de la señal |

## Keycaps
- **Menú**: JetBrains Mono bold, borde 1,5 px `Steel` con borde inferior de 3–4 px, fondo `Hull`, 34–40 px de alto.
- **HUD** (`.kc`): borde 1 px `#C9CCD2` con borde inferior de 3 px, fondo `rgba(10,10,12,.6)`, 20 px de alto, texto 10 px.
- Con mando, el keycap se reemplaza por el glifo del botón ([09-Input.md](09-Input.md)).

## Logo
- **Placa principal**: "DEAD" en Saira Stencil One sobre placa magenta (chaflán superior izquierdo), "DRAW" sobre placa `Plate` (chaflán inferior derecho), pernos, y debajo cinta magenta + "ARENA UNIT DD-10 // SURVIVE 04:00" + cinta.
- **Monograma**: "DD" sobre magenta con franja de cinta inferior, 180 px y 64 px.
- **Horizontal sobre blanco**: monograma 70 px + "DEAD DRAW" en Saira Condensed 800.
