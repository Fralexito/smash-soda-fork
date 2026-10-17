// =============================================================================
//  Menú «por momento de uso» (pedido de Fralex, 0.3.4): ANTES · JUGANDO · DESPUÉS ·
//  OTRAS SALAS · AJUSTES. Cada pestaña del menú apunta a una pantalla de siempre
//  (sección + pestaña de vistas/*.js), así que irA("mandos", "mapeo"), la paleta,
//  el modo cedido y lo que se avisa al motor (ui.seccion) no cambian.
// =============================================================================

// p: [sección, pestaña]; es/en: nombre de la pestaña en el menú nuevo.
// mas: true = detrás del botón MÁS (máximo 4 pestañas a la vista).
export const GRUPOS = [
  { id: "antes", es: "ANTES", en: "BEFORE", icono: "antes",
    d: ["Preparar la sala: abrirla, la transmisión, las opciones y tu PC.", "Get the room ready: open it, the stream, the options and your PC."],
    pestanas: [
      { p: ["sala", "resumen"], es: "SALA", en: "ROOM" },
      { p: ["sala", "transmision"], es: "TRANSMISIÓN", en: "STREAM" },
      { p: ["sala", "opciones"], es: "OPCIONES", en: "OPTIONS" },
      { p: ["sync", "mipc"], es: "MI PC", en: "MY PC" },
      { p: ["sala", "red"], es: "CONEXIÓN", en: "CONNECTION", mas: true },
      { p: ["sala", "juegos"], es: "JUEGOS", en: "GAMES", mas: true },
    ] },
  { id: "jugando", es: "JUGANDO", en: "PLAYING", icono: "mandos",
    d: ["Durante el partido: puestos, marcador, gente en la sala y mandos.", "During the match: seats, score, people in the room and pads."],
    pestanas: [
      { p: ["mandos", "puestos"], es: "PUESTOS", en: "SEATS" },
      { p: ["partido", "vivo"], es: "MARCADOR", en: "SCORE" },
      { p: ["gente", "sala"], es: "GENTE", en: "PEOPLE" },
      { p: ["mandos", "marionetas"], es: "TU MANDO", en: "YOUR PAD" },
      { p: ["partido", "campo"], es: "CAMPO", en: "PITCH", mas: true },
      { p: ["mandos", "mapeo"], es: "MAPEO", en: "MAPPING", mas: true },
      { p: ["mandos", "bloqueo"], es: "BLOQUEO", en: "LOCK", mas: true },
      { p: ["mandos", "turnos"], es: "TURNOS", en: "HOTSEAT", mas: true },
      { p: ["mandos", "teclado"], es: "TECLADO", en: "KEYBOARD", mas: true },
      { p: ["sala", "puesto"], es: "DÓNDE JUEGO YO", en: "WHERE I PLAY", mas: true },
    ] },
  { id: "despues", es: "DESPUÉS", en: "AFTER", icono: "trofeo",
    d: ["Después del partido: resultados, traspasos y lo que pasó en la sala.", "After the match: results, transfers and what happened in the room."],
    pestanas: [
      { p: ["partido", "historial"], es: "HISTORIAL", en: "HISTORY" },
      { p: ["sync", "traspasos"], es: "TRASPASOS", en: "TRANSFERS" },
      { p: ["sala", "actividad"], es: "ACTIVIDAD", en: "ACTIVITY" },
    ] },
  { id: "otras", es: "OTRAS SALAS", en: "OTHER ROOMS", icono: "jugar",
    d: ["Para jugar en salas de otros: salas abiertas, tus amigos y tu perfil de PES.", "To play in other people's rooms: open rooms, your friends and your PES profile."],
    pestanas: [
      { p: ["jugar", "salas"], es: "SALAS ABIERTAS", en: "OPEN ROOMS" },
      { p: ["jugar", "amigos"], es: "AMIGOS", en: "FRIENDS" },
      { p: ["jugar", "perfiles"], es: "PERFIL PES", en: "PES PROFILE" },
    ] },
  { id: "ajustes", es: "AJUSTES", en: "SETTINGS", icono: "ajustes",
    d: ["Todo lo demás: general, overlay, moderación y opciones de Sync.", "Everything else: general, overlay, moderation and Sync options."],
    pestanas: [
      { p: ["ajustes", "general"], es: "GENERAL", en: "GENERAL" },
      { p: ["ajustes", "overlay"], es: "OVERLAY", en: "OVERLAY" },
      { p: ["gente", "moderacion"], es: "MODERACIÓN", en: "MODERATION" },
      { p: ["sync", "avanzado"], es: "SYNC", en: "SYNC" },
      { p: ["ajustes", "sonidos"], es: "SONIDOS CHAT", en: "CHAT SOUNDS", mas: true },
      { p: ["ajustes", "interfaz"], es: "INTERFAZ", en: "INTERFACE", mas: true },
      { p: ["gente", "permisos"], es: "PERMISOS", en: "PERMISSIONS", mas: true },
      { p: ["ajustes", "avanzado"], es: "AVANZADO", en: "ADVANCED", mas: true },
    ] },
];

/** Grupo al que pertenece una pantalla (sección + pestaña). */
export function grupoDe(seccion, pestana) {
  return GRUPOS.find((g) => g.pestanas.some((x) => x.p[0] === seccion && x.p[1] === pestana)) || GRUPOS[0];
}

/** «Jugando › Bloqueo»: dónde está una pantalla en el menú nuevo (paleta Ctrl+K). */
export function rutaDe(seccion, pestana, en = false) {
  const g = grupoDe(seccion, pestana);
  const x = g.pestanas.find((y) => y.p[0] === seccion && y.p[1] === pestana);
  const bonito = (w) => w.charAt(0) + w.slice(1).toLowerCase();
  return x ? `${bonito(en ? g.en : g.es)} › ${bonito(en ? x.en : x.es)}` : "";
}
