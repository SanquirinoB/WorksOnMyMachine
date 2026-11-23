# 06 — Cartas

Artboards: `CardArt_Card` (componente), `CardArt_Catalog` (12 cartas, tamaños pequeños, producción), `CardArt_Draw` (en contexto). **Aprobado 2026-11-15**: la imagen principal de cada carta es una **escena renderizada que muestra su efecto**; el ícono pasa a una insignia.

## Anatomía (360 × 540)
| Capa | Spec |
|---|---|
| Marco | 4 px alrededor, chaflán 24. Stat: color del palo con alfa (blanco 0,55, magenta 0,6, menta 0,55); elegida: magenta sólido. Especial: cinta magenta/negra de 8 px que se desplaza con el tilt |
| Acento | Color del palo; **en las especiales, magenta siempre** (Piercing Rounds y Ricochet son SIGHT pero su etiqueta, insignia, palabra clave, tinte y banda son magenta; el glifo del palo sigue blanco) |
| Cuerpo | `Hull`, chaflán 22, recorta todo lo interior |
| **Escena** | 352 × 250 arriba, a sangre. Dos capas: escena (fondo, con sobreescala fija de 1,06 para el parallax) y efecto (frente). Encima: tinte radial del color del palo en el punto focal (alfa 0,16 en magenta y menta, 0,10 en blanco), degradado vertical (`Soot` 0,72 → 0 en el 24 %, 0 hasta el 58 %, `Hull` 0,86 en el 88 %, `Hull` sólido al 100 %) y scanlines sutiles |
| Fila superior (sobre la escena) | Palo + dominio ("SIGHT · RIFLE") en JetBrains Mono 13 px bold, color del palo, con sombra; a la derecha la tecla (1/2/3 o el glifo) solo en Card Draw |
| Insignia | 58 × 58 en (22, 214), `Soot`, chaflán 11, contorno interior de 2 px del color del palo, glifo del palo a 32 px. Monta sobre el borde inferior de la escena |
| Texto (desde y 286, márgenes 26) | Chip de tipo (`STAT · STACKS` o `SPECIAL · UNIQUE`) · nombre (Saira Condensed 700, 40 px, mayúsculas) · efecto (Barlow 22 px, `#B9BCC3`, con la cifra o palabra clave en Saira 700 del color del palo) · pie (JetBrains Mono 12 px, `Steel`, separador `Seam` arriba, al fondo) |
| Brillo | Radial blanco 0,16 que sigue al cursor, modo screen, solo con tilt |

Glifos de palo: SIGHT = rombo con punto; EDGE = hoja diagonal con guarda; VECTOR = doble chevron; PLATE = hexágono con hexágono interior.

## Catálogo (18 cartas reales)
Los valores entre corchetes salen de los datos de la carta. Los demás valores de stats son los del panel de build del mockup y deben confirmarse contra los datos ([10-Data.md](10-Data.md)).

| Carta | Palo | Tipo | Efecto (texto) | Escena |
|---|---|---|---|---|
| Firepower | SIGHT | stat | "+15% rifle damage." | Trazadoras blancas que impactan el ojo de un dron esfera; destello y chispas naranjas |
| Rapid Fire | SIGHT | stat | "+12% rifle fire rate." | Primer plano del blaster con fogonazo y una ráfaga de trazadoras hacia un dron; vainas eyectadas |
| Extended Magazine | SIGHT | stat | "+[N] rounds per magazine." | Blaster con un cargador holográfico extendido lleno de proyectiles y un "+" |
| Piercing Rounds | SIGHT | especial | "Bullets pass through every enemy in line." | Un rayo blanco y magenta atraviesa una esfera y un cubo alineados |
| Ricochet | SIGHT | especial | "Bullets bounce to a nearby enemy." | Trayectoria que rebota de una esfera a una pirámide y a otra pirámide, con chispas en cada rebote |
| Sharp Edge | EDGE | stat | "+20% katana damage." | Arco de katana magenta que corta un dron cubo |
| Swift Blade | EDGE | stat | "+[N]% katana swing speed." | Tres arcos rápidos sobre una pirámide con estelas |
| Electric Blade | EDGE | especial | "Katana hits deal bonus electric damage." | Corte sobre una esfera y un rayo que encadena una pirámide y otra esfera |
| Shockwave | EDGE | especial | "Every katana swing sends out a shockwave." | Anillos magenta que se expanden en el suelo desde un corte; drones alrededor |
| Quick Dash | VECTOR | stat | "−15% dash cooldown." | Arena con desenfoque de movimiento, líneas radiales y doble chevron magenta |
| Static Dash | VECTOR | especial | "Your dash shocks every enemy along its path." | Estela de Dash magenta por el suelo con descargas que suben a los drones |
| Reinforced Armor | PLATE | stat | "+25 max health." | Escudo hexagonal menta frente a la vista en primera persona que detiene un disparo naranja |
| Reload Speed | SIGHT | stat | "−[N]% rifle reload time." | **Añadida (origin, lote 10–19)**; escena por diseñar en UI-A |
| Precision | SIGHT | stat | "−[N]% rifle spread. The first shot stays exact." | Añadida; escena por diseñar |
| Magnet | VECTOR | stat | "+[N]% XP pickup radius." | Añadida; escena por diseñar |
| Move Speed | VECTOR | stat | "+[N]% walk and sprint speed." | Añadida; escena por diseñar |
| Damage Reduction | PLATE | stat | "−[N]% damage taken." (máximo 12 %) | Añadida; escena por diseñar |
| Healing Efficiency | PLATE | stat | "+[N]% healing from repair kits." (máximo +45 %) | Añadida; escena por diseñar |

Pies:
- La palabra clave del efecto va en el color de acento.
- Por defecto, "STACKS · EVERY COPY ADDS" (stat) y "UNIQUE · LEAVES THE POOL" (especial).
- En Card Draw, el pie es la proyección: "OWNED ×N → <total>" o "NEW → <antes> → <después>".
- Abreviaturas para listas compactas: FP, RF, EM, PIERCE, RICO, SE, QD, SD, RA (del canvas) y SB, EB, SW (**añadidas** para Swift Blade, Electric Blade y Shockwave).
- Abreviaturas de las seis cartas añadidas con origin (lotes 10–19): RS (Reload Speed), PR (Precision), MG (Magnet), MS (Move Speed), DR (Damage Reduction), HE (Healing Efficiency). Todas sus cifras salen de los datos (`Magnitude` × rango, con los límites del gameplay: recarga y dispersión ×0,05–4, movimiento ×0,1–3, reducción de daño ≤ 12 %, curación ≤ +45 %); `[N]` en la tabla es la cifra de un rango. Proyecciones de pie: RELOAD, SPREAD, XP RADIUS, MOVE SPEED, DMG TAKEN, KIT HEALING.

## Tamaños pequeños
| Uso | Tamaño | Contenido |
|---|---|---|
| Mano del HUD y pie de Card Draw | 52 × 72 | Recorte de la escena centrado en el `ArtFocus` (escala ≈ 0,36) con el efecto simplificado (trazos más gruesos); marco `Plate` de 2 px (especial: cinta de 8 px); banda superior de 3 px del color de acento; degradado inferior de 24 px y "×N" (o "★" en magenta para especiales) |
| Victoria | 76 × 100 | Mismo recorte + abreviatura + ×N |
| Derrota | 62 × 82 | Mismo recorte + ×N; las de acento magenta en rojo |
| Panel TAB | 22 px | El mismo recorte, cuadrado |
| Lista de build de Pausa | 34 px (columna de 44) | El mismo recorte, cuadrado |

La escena tiene que leerse a 52 × 72: un sujeto, silueta clara y efecto de alto contraste.

## Dirección de arte
1. Un momento por carta, en la arena real, con los drones, el blaster y la katana reales.
2. El efecto vive en el 60 % superior; la parte inferior se funde con el cuerpo de la carta y no lleva nada importante.
3. El mundo va graduado hacia abajo (en la maqueta: `saturate(.62) brightness(.78) contrast(1.12)`; Dash además `blur(1.4px)`); el efecto, en su color de acento, es lo más brillante del cuadro. Los drones conservan su naranja.
4. Un sujeto claro por carta.
5. Dos capas (escena y efecto) para la profundidad del tilt.
6. Las cartas de Dash llevan desenfoque de movimiento en la escena (regla de la maqueta).
7. Sin texto horneado en las imágenes (plan §2.2).

Las reglas 1–5 son las del artboard `CardArt_Catalog`.

## Producción (etapa UI-A del plan)
1. Mapa `L_CardBooth` (`/Game/DeadDraw/Tests/Maps`) con assets de la arena; una CineCamera y un montaje de efecto por carta.
2. Efectos reales del juego: disparos, estela de katana y los pulsos de Electric Blade, Shockwave y Static Dash.
3. Render por script de las dos capas a 3× (1056 × 750) y material de grading común.
4. Texturas `T_CardArt_<Carta>_Scene` y `T_CardArt_<Carta>_Effect` (24); `UDDCardPresentation` guarda `ArtScene`, `ArtEffect` y `ArtFocus` (UV del sujeto, para los recortes pequeños).
5. Se registran en el manifiesto como obra propia del proyecto.

Las escenas del canvas son una maqueta: recortes de la captura del juego con efectos dibujados. No son el arte final.
