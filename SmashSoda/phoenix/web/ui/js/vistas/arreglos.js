// =============================================================================
//  ARREGLOS: lo que hace cada botón «Arreglar» (Semáforo Phoenix, Diagnóstico,
//  avisos). Un solo sitio, así un mismo problema se arregla igual en todas partes.
// =============================================================================
import { t } from "../i18n.js";
import { accion, avisar } from "../tienda.js";

// El mismo instalador oficial que baja PhoenixLink-instalar.bat (paso 4/8).
export const URL_VIGEM = "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe";

/** Baja el instalador oficial de ViGEmBus en el navegador y explica en una frase cómo seguir. */
export async function instalarVigem() {
  await accion("ui.abrir", { url: URL_VIGEM });
  avisar(t("Se está bajando el instalador oficial de ViGEmBus. Ábrelo, pulsa Instalar y, al terminar, cierra y abre Phoenix Link.",
    "The official ViGEmBus installer is downloading. Open it, press Install and, when done, close and reopen Phoenix Link."), "info", 12000);
}
