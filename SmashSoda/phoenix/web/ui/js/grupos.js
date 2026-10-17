// =============================================================================
//  Menú de Phoenix Link (0.3.7): SALA · PARTIDO · OTRAS SALAS · SYNC · AJUSTES.
//  Nombres de cosas, no de momentos (Fralex: «no me gusta que diga antes y después»),
//  y SYNC siempre con su sección propia (regla fija: nunca dentro de Ajustes ni de otra).
//  Cada pestaña del menú apunta a una pantalla de siempre (sección + pestaña de
//  vistas/*.js), así que irA("mandos", "mapeo"), la paleta, el modo cedido y lo
//  que se avisa al motor (ui.seccion) no cambian.
// =============================================================================

// p: [sección, pestaña]; es/en: nombre de la pestaña en el menú.
// mas: true = detrás del botón MÁS (máximo 4 pestañas a la vista).
export const GRUPOS = [
  { id: "sala", es: "SALA", en: "ROOM", icono: "sala",
    d: ["Tu sala: abrirla, el enlace, la transmisión, la gente y las opciones.", "Your room: open it, the link, the stream, the people and the options."],
    pestanas: [
      { p: ["sala", "resumen"], es: "RESUMEN", en: "OVERVIEW" },
      { p: ["sala", "transmision"], es: "TRANSMISIÓN", en: "STREAM" },
      { p: ["gente", "sala"], es: "GENTE", en: "PEOPLE" },
      { p: ["sala", "opciones"], es: "OPCIONES", en: "OPTIONS" },
      { p: ["sala", "red"], es: "CONEXIÓN", en: "CONNECTION", mas: true },
      { p: ["sala", "juegos"], es: "JUEGOS", en: "GAMES", mas: true },
      { p: ["sala", "actividad"], es: "ACTIVIDAD", en: "ACTIVITY", mas: true },
    ] },
  { id: "partido", es: "PARTIDO", en: "MATCH", icono: "partido",
    d: ["El partido: puestos y mandos, marcador e historial.", "The match: seats and pads, score and history."],
    pestanas: [
      { p: ["mandos", "puestos"], es: "PUESTOS", en: "SEATS" },
      { p: ["partido", "vivo"], es: "MARCADOR", en: "SCORE" },
      { p: ["mandos", "marionetas"], es: "TU MANDO", en: "YOUR PAD" },
      { p: ["partido", "historial"], es: "HISTORIAL", en: "HISTORY" },
      { p: ["partido", "campo"], es: "CAMPO", en: "PITCH", mas: true },
      { p: ["mandos", "mapeo"], es: "MAPEO", en: "MAPPING", mas: true },
      { p: ["mandos", "bloqueo"], es: "BLOQUEO", en: "LOCK", mas: true },
      { p: ["mandos", "turnos"], es: "TURNOS", en: "HOTSEAT", mas: true },
      { p: ["mandos", "teclado"], es: "TECLADO", en: "KEYBOARD", mas: true },
      { p: ["sala", "puesto"], es: "DÓNDE JUEGO YO", en: "WHERE I PLAY", mas: true },
    ] },
  { id: "otras", es: "OTRAS SALAS", en: "OTHER ROOMS", icono: "jugar",
    d: ["Para jugar en salas de otros: salas abiertas, tus amigos y tu perfil de PES.", "To play in other people's rooms: open rooms, your friends and your PES profile."],
    pestanas: [
      { p: ["jugar", "salas"], es: "SALAS ABIERTAS", en: "OPEN ROOMS" },
      { p: ["jugar", "amigos"], es: "AMIGOS", en: "FRIENDS" },
      { p: ["jugar", "perfiles"], es: "PERFIL PES", en: "PES PROFILE" },
    ] },
  { id: "sync", es: "SYNC", en: "SYNC", icono: "sync",
    d: ["Conectar tu cuenta de la liga y el juego con la web: tu PC, traspasos y opciones avanzadas.", "Connect your league account and the game with the website: your PC, transfers and advanced options."],
    pestanas: [
      { p: ["sync", "mipc"], es: "MI PC", en: "MY PC" },
      { p: ["sync", "traspasos"], es: "TRASPASOS", en: "TRANSFERS" },
      { p: ["sync", "avanzado"], es: "AVANZADO", en: "ADVANCED" },
    ] },
  { id: "ajustes", es: "AJUSTES", en: "SETTINGS", icono: "ajustes",
    d: ["Todo lo demás: general, overlay, moderación y sonidos del chat.", "Everything else: general, overlay, moderation and chat sounds."],
    pestanas: [
      { p: ["ajustes", "general"], es: "GENERAL", en: "GENERAL" },
      { p: ["ajustes", "overlay"], es: "OVERLAY", en: "OVERLAY" },
      { p: ["gente", "moderacion"], es: "MODERACIÓN", en: "MODERATION" },
      { p: ["ajustes", "sonidos"], es: "SONIDOS CHAT", en: "CHAT SOUNDS" },
      { p: ["ajustes", "interfaz"], es: "INTERFAZ", en: "INTERFACE", mas: true },
      { p: ["gente", "permisos"], es: "PERMISOS", en: "PERMISSIONS", mas: true },
      { p: ["ajustes", "avanzado"], es: "AVANZADO", en: "ADVANCED", mas: true },
    ] },
];

/** Grupo al que pertenece una pantalla (sección + pestaña). */
export function grupoDe(seccion, pestana) {
  return GRUPOS.find((g) => g.pestanas.some((x) => x.p[0] === seccion && x.p[1] === pestana)) || GRUPOS[0];
}

/** «Partido › Bloqueo»: dónde está una pantalla en el menú (paleta Ctrl+K). */
export function rutaDe(seccion, pestana, en = false) {
  const g = grupoDe(seccion, pestana);
  const x = g.pestanas.find((y) => y.p[0] === seccion && y.p[1] === pestana);
  const bonito = (w) => w.charAt(0) + w.slice(1).toLowerCase();
  return x ? `${bonito(en ? g.en : g.es)} › ${bonito(en ? x.en : x.es)}` : "";
}
