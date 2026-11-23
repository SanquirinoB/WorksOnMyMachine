# DeadDraw — Spec de UI (documentación de referencia)

Estado: **aprobada** (2026-11-15). Esta carpeta es la versión escrita y normativa del diseño de UI. Si el código, una WBP o un asset contradice este texto, manda este texto. Si este texto contradice el canvas, se corrige uno de los dos y se registra en [11-Decisions.md](11-Decisions.md).

- **Fuente visual**: canvas "Dead Draw — Art Direction & UI", `https://claude.ai/artifact/3yAc3SFYjPXu2HWwKAJhfq` (privado del usuario).
- **Plan de implementación**: [`Docs/Architecture/DeadDraw-UI-ImplementationPlan.md`](../../Architecture/DeadDraw-UI-ImplementationPlan.md). Este README describe *qué* se construye; el plan describe *cómo* y en qué orden.
- **Idioma de la UI**: inglés. Todo texto visible es `FText`; los textos de esta spec entre comillas son literales de pantalla.
- **Resolución de referencia**: 1920 × 1080. Todas las medidas en px de esa resolución; la UI escala con la DPI del proyecto y con el ajuste HUD scale.

## Índice
| Doc | Contenido | Artboards del canvas |
|---|---|---|
| [01-DesignSystem.md](01-DesignSystem.md) | Colores, tipografía, formas, cintas, botones, chips, logo | `DS_I_Rooftop` |
| [02-HUD.md](02-HUD.md) | HUD v2 A: layout, safe zone, cada bloque, estados Under Pressure y Heavy Mech | `HUD`, `HUD2_A_CompactCorners`, `HUDCombat`, `BossHUD` |
| [03-HUDEvents.md](03-HUDEvents.md) | Los 15 eventos del HUD y las señales de mundo | `HUDEvents`, `Motion_HUDEvents`, `HUDCombat` |
| [04-Overlays.md](04-Overlays.md) | Level Up / Card Draw, Pausa, confirmación, Victoria, Derrota | `CardArt_Draw`, `CardDraw` (fondo y encabezado), `Motion_LevelUp`, `PauseMenu`, `Victory`, `Defeat` |
| [05-FrontEnd.md](05-FrontEnd.md) | Main Menu, How to Play, Settings, Credits | `MainMenu`, `HowToPlay`, `Settings`, `Motion_Screens` (intro) |
| [06-Cards.md](06-Cards.md) | Anatomía de carta con arte de escena, catálogo de 12, tamaños pequeños, dirección de arte | `CardArt_Card`, `CardArt_Catalog` |
| [07-Motion.md](07-Motion.md) | Tokens, reglas, tabla de tiempos, secuencias de pantallas | `Motion_System`, `Motion_Screens`, `Motion_LevelUp`, `Motion_HUDEvents` |
| [08-Audio.md](08-Audio.md) | Cues de UI, origen y relación con el audio de gameplay | tabla de `Motion_System` |
| [09-Input.md](09-Input.md) | Acciones, glifos por dispositivo, navegación de menús, rebinding | todos los pies de tecla |
| [10-Data.md](10-Data.md) | Datos de juego que la UI consume | todos |
| [11-Decisions.md](11-Decisions.md) | Decisiones D1–D8, resolución de inconsistencias, pendientes | — |

## Reglas de precedencia
1. Las **decisiones registradas** en [11-Decisions.md](11-Decisions.md) mandan sobre cualquier artboard.
2. Para **duraciones, curvas y audio** manda la tabla de `Motion_System`; los scripts de los artboards de Motion aportan el detalle fino (desplazamientos, retardos, escalas) cuando la tabla no lo da.
3. Para **layout y medidas** manda `HUD2_A_CompactCorners` (HUD) y el artboard propio de cada pantalla.
4. Para **cartas** manda la fila 06 (`CardArt_*`); `CardDraw` (fila 03) queda reemplazado en la anatomía de la carta pero sigue valiendo para el fondo y el encabezado.
5. Los **valores de juego** de los mockups (vida 87/100, munición 24/36, kills, nivel, Dash 3 s) son ilustrativos: la UI muestra los datos reales ([10-Data.md](10-Data.md)).

## Convenciones
- **Añadido**: algo que no está en el canvas y entra por decisión o por necesidad técnica; se marca así en el texto y figura en [11-Decisions.md](11-Decisions.md).
- Los conflictos entre artboards se resuelven en [11-Decisions.md](11-Decisions.md#inconsistencias-del-canvas-y-su-resolución); el resto de documentos ya usan el valor resuelto.

## Artboards descartados (no son spec)
`Main` (00 · A), `DS_B` … `DS_H` (variantes de design system), `HUD2_B_EdgeLine`, `HUD2_C_CrosshairAnchored`. La carta de `CardDraw` (ícono + texto) está reemplazada por la fila 06.

## Estado de pendientes
Ver [11-Decisions.md § Pendientes](11-Decisions.md#pendientes): pantalla Credits y fila Reduced motion por diseñar en el canvas, aprobación de sonidos, descripciones de cada fila de Settings.
