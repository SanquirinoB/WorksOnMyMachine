# 09 — Input, glifos y navegación

Implementación: CoreKitNavigation extendido (N1–N6), sin CommonUI. Detalle técnico en `Plugins/CoreKit/Docs/CoreKitNavigation-Expansion.md` y §7 del plan.

## Acciones del jugador
| Acción | Teclado/mouse por defecto | Mando (por defecto propuesto) | Rebindeable |
|---|---|---|---|
| Move | W A S D | stick izquierdo | no |
| Look | mouse | stick derecho | no (sensibilidad e invertir Y sí) |
| Jump | Space | A / Cross | sí |
| Dash | Shift (+ dirección) | B / Circle | sí |
| Attack | LMB | RT / R2 | sí (D7) |
| Aim | RMB (verificar contra `IMC_Combat`) | LT / L2 | sí (D7) |
| Reload | R | X / Square | sí |
| Equip rifle / katana | 1 / 2 | D-pad | sí |
| Swap weapon | rueda | Y / Triangle | sí |
| Interact | E (verificar contra `IMC_Combat`) | — | sí (D7) |
| Inspect build | mantener Tab | mantener View / Touchpad | sí (D7) |
| Pause | Esc | Menu / Options | **bloqueada** |
| Elegir carta 1/2/3 | 1 / 2 / 3 | foco + A | — |

Los mapeos de mando se definen en UI-1 contra `IMC_Combat`; esta tabla es la propuesta de partida.

## Navegación de menús
| Acción de menú | Teclado | Mando |
|---|---|---|
| Mover el foco | W/S, flechas (y A/D en rejillas) | D-pad / stick izquierdo |
| Seleccionar / activar la fila (en Settings: empezar a escuchar en rebinding) | Enter / clic | A / Cross |
| Volver | Esc | B / Circle |
| Pestaña anterior / siguiente | Q / E | LB / RB |
| Cambiar valor de la fila | A/D, flechas | D-pad izquierda/derecha |
| Reset de Settings | T | Y / Triangle |
| Aplicar Settings | F | X / Square |

- Hover y foco son el mismo "seleccionado": un solo elemento resaltado a la vez.
- Al abrir un menú, el foco va a su elemento inicial (PLAY, RESUME, la primera carta, la primera fila); al cerrar, vuelve a donde estaba.
- Volver sale de la pantalla superior y regresa a quien la abrió (Pausa → Settings → Pausa).
- Bloqueo de input al abrir: 400 ms en Card Draw; un cooldown breve en el resto de menús para evitar dobles pulsaciones.
- Con mando, el cursor se oculta; vuelve al mover el mouse.
- Input mode: juego durante la run; menú (con cursor) en front end, pausa, cartas y finales.

## Glifos
- Cada pie de tecla y keycap muestra el binding **actual** del jugador para el **dispositivo activo**: teclado y mouse, Xbox o PlayStation.
- Sets de íconos: Kenney Input Prompts 1.5 (CC0), variantes "Double" de 128 px, teñidos en UMG con el estilo del keycap.
- Detección del tipo de mando por hardware. En Windows sin GameInput todo mando aparece como XInput, así que se usa Xbox o el ajuste "Gamepad glyphs" (AUTO / XBOX / PLAYSTATION).
- Si una tecla no tiene ícono, se muestra su nombre dentro del keycap.

## Rebinding (Settings › Controls)
- La fila enfocada entra en escucha con "PRESS A KEY…".
- Cualquier tecla o botón del mouse sirve. Si ya está en uso, se intercambia con la otra acción. Escape cancela.
- Restauración global con T; restauración por fila (**añadido**, la ofrece `UCoreKitKeyBindings`). Los cambios persisten en los ajustes de Enhanced Input del jugador.
