// =============================================================================
//  Menú de Phoenix Link (0.3.11): INICIO · SALA · PARTIDO · MANDOS · JUGAR · SYNC · AJUSTES (plan UX 0.3.30: «Otras salas» pasa a «Jugar»).
//  PARTIDO: marcador, campo e historial. MANDOS: puestos, mandos en vivo, árbitro y bloqueo, tu mando
//  (Fralex, 0.3.11: «esto podria en mandos, pero mejor organizado»).
//  Nombres de cosas, no de momentos (Fralex: «no me gusta que diga antes y después»),
//  y SYNC siempre con su sección propia (regla fija: nunca dentro de Ajustes ni de otra).
//  Cada pestaña del menú apunta a una pantalla de siempre (sección + pestaña de
//  vistas/*.js), así que irA("mandos", "mapeo"), la paleta, el modo cedido y lo
//  que se avisa al motor (ui.seccion) no cambian.
// =============================================================================

// p: [sección, pestaña]; es/en: nombre de la pestaña en el menú.
// mas: true = detrás del botón MÁS. Lo que se usa al jugar va siempre a la vista (plan UX 0.3.22: Conexión y Juegos).
export const GRUPOS = [
  { id: "inicio", es: "INICIO", en: "HOME", icono: "inicio",
    d: ["Jugar en dos clics: con un amigo o contra quien esté libre.", "Play in two clicks: with a friend or against whoever is free."],
    pestanas: [
      { p: ["inicio", "inicio"], es: "INICIO", en: "HOME" },
    ] },
  { id: "sala", es: "SALA", en: "ROOM", icono: "sala",
    d: ["Tu sala: abrirla, el enlace, la transmisión, la gente (con moderación y permisos) y las opciones.", "Your room: open it, the link, the stream, the people (with moderation and permissions) and the options."],
    pestanas: [
      { p: ["sala", "resumen"], es: "RESUMEN", en: "OVERVIEW" },
      { p: ["sala", "transmision"], es: "TRANSMISIÓN", en: "STREAM" },
      { p: ["gente", "sala"], es: "GENTE", en: "PEOPLE" },
      { p: ["gente", "moderacion"], es: "MODERACIÓN", en: "MODERATION", mas: true },   // plan UX: junto a la gente
      { p: ["gente", "permisos"], es: "PERMISOS", en: "PERMISSIONS", mas: true },
      { p: ["sala", "opciones"], es: "OPCIONES", en: "OPTIONS" },
      { p: ["sala", "red"], es: "CONEXIÓN", en: "CONNECTION" },   // plan UX: lo que se usa al jugar no va tras «MÁS»
      { p: ["sala", "juegos"], es: "JUEGOS", en: "GAMES" },
      { p: ["sala", "actividad"], es: "ACTIVIDAD", en: "ACTIVITY", mas: true },
    ] },
  { id: "partido", es: "PARTIDO", en: "MATCH", icono: "partido",
    d: ["El partido: marcador, equipos y lados, campo en vivo e historial.", "The match: score, teams and sides, live pitch and history."],
    pestanas: [
      { p: ["partido", "vivo"], es: "MARCADOR", en: "SCORE" },
      { p: ["partido", "campo"], es: "CAMPO", en: "PITCH" },
      { p: ["partido", "historial"], es: "HISTORIAL", en: "HISTORY" },
    ] },
  { id: "mandos", es: "MANDOS", en: "PADS", icono: "mandos",
    d: ["Los mandos: quién juega en cada puesto, qué pulsa cada uno en vivo, árbitro y bloqueo, y tu mando.", "The pads: who plays in each seat, what each one presses live, referee and lock, and your pad."],
    pestanas: [
      { p: ["mandos", "puestos"], es: "PUESTOS", en: "SEATS" },
      { p: ["mandos", "vivo"], es: "EN VIVO", en: "LIVE" },
      { p: ["mandos", "bloqueo"], es: "ÁRBITRO Y BLOQUEO", en: "REFEREE & LOCK" },
      { p: ["mandos", "marionetas"], es: "TU MANDO", en: "YOUR PAD" },
      { p: ["mandos", "mapeo"], es: "MAPEO", en: "MAPPING", mas: true },
      { p: ["mandos", "teclado"], es: "TECLADO", en: "KEYBOARD", mas: true },
      { p: ["mandos", "turnos"], es: "TURNOS", en: "HOTSEAT", mas: true },
      { p: ["sala", "puesto"], es: "DÓNDE JUEGO YO", en: "WHERE I PLAY", mas: true },
    ] },
  { id: "otras", es: "JUGAR", en: "PLAY", icono: "jugar",
    d: ["Para jugar en salas de otros: salas abiertas, tus amigos y tu perfil de PES.", "To play in other people's rooms: open rooms, your friends and your PES profile."],
    pestanas: [
      { p: ["jugar", "salas"], es: "SALAS ABIERTAS", en: "OPEN ROOMS" },
      { p: ["jugar", "amigos"], es: "AMIGOS", en: "FRIENDS" },
      { p: ["jugar", "perfiles"], es: "PERFIL PES", en: "PES PROFILE" },
    ] },
  { id: "sync", es: "SYNC", en: "SYNC", icono: "sync",
    d: ["Conectar tu cuenta de la liga y el juego con la web: tu PC, traspasos y opciones avanzadas.", "Connect your league account and the game with the website: your PC, transfers and advanced options."],
    pestanas: [
      { p: ["sync", "mipc"], es: "MI JUEGO", en: "MY GAME" },
      { p: ["sync", "traspasos"], es: "TRASPASOS", en: "TRANSFERS" },
      { p: ["sync", "avanzado"], es: "AVANZADO", en: "ADVANCED" },
    ] },
  { id: "ajustes", es: "AJUSTES", en: "SETTINGS", icono: "ajustes",
    d: ["Todo lo demás: lo fundamental (idioma, tema, sentado automático), overlay, sonidos del chat e interfaz.", "Everything else: the basics (language, theme, auto-seat), overlay, chat sounds and interface."],
    pestanas: [
      { p: ["ajustes", "general"], es: "GENERAL", en: "GENERAL" },
      { p: ["ajustes", "overlay"], es: "OVERLAY", en: "OVERLAY" },
      { p: ["ajustes", "sonidos"], es: "SONIDOS CHAT", en: "CHAT SOUNDS" },
      { p: ["ajustes", "interfaz"], es: "INTERFAZ", en: "INTERFACE" },
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
