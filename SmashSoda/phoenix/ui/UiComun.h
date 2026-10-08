#pragma once

#include <algorithm>
#include "imgui.h"

// =============================================================================
//  Phoenix Link · utilidades visuales compartidas por las pantallas del shell
// =============================================================================

namespace phoenix::vis {

	inline ImU32 col(const ImVec4& c, float alfa = 1.0f) {
		return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alfa));
	}

	/// Acerca `actual` a `objetivo` de forma suave e independiente de los FPS.
	inline float acercar(float actual, float objetivo, float velocidad) {
		const float k = (std::min)(1.0f, ImGui::GetIO().DeltaTime * velocidad);
		return actual + (objetivo - actual) * k;
	}

	/// Mezcla dos colores (t = 0 → a, t = 1 → b).
	inline ImVec4 mezclar(const ImVec4& a, const ImVec4& b, float t) {
		return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}

	/// Color de semáforo para un ping en ms (-1 = sin dato).
	inline ImVec4 colorPing(int ms, const ImVec4& bueno, const ImVec4& malo, const ImVec4& apagado) {
		if (ms < 0) return apagado;
		if (ms < 60) return bueno;
		if (ms < 120) return ImVec4(0.96f, 0.71f, 0.27f, 1.0f); // ámbar
		return malo;
	}


	/// Botón chip dibujado a mano (hover suave + pulso al clic). Devuelve true al clic.
	inline bool chip(const char* id, const char* texto, const ImVec4& color, bool lleno, float s, float ancho = 0.0f) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 t = ImGui::CalcTextSize(texto);
		const ImVec2 tam(ancho > 0.0f ? ancho : t.x + 26.0f * s, 32.0f * s);
		const ImVec2 p0 = ImGui::GetCursorScreenPos();
		const ImGuiID k = ImGui::GetID(id);
		const bool clic = ImGui::InvisibleButton(id, tam);
		const bool encima = ImGui::IsItemHovered();
		if (encima) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGuiStorage* st = ImGui::GetStateStorage();
		float h = acercar(st->GetFloat(k, 0.0f), encima ? 1.0f : 0.0f, 18.0f);
		st->SetFloat(k, h);
		const bool apretado = ImGui::IsItemActive();
		const float enc = apretado ? 1.5f * s : 0.0f;
		const ImVec2 a(p0.x + enc, p0.y + enc), b(p0.x + tam.x - enc, p0.y + tam.y - enc);
		dl->AddRectFilled(a, b, col(color, lleno ? 0.85f + 0.15f * h : 0.10f + 0.14f * h), tam.y * 0.5f);
		if (!lleno) dl->AddRect(a, b, col(color, 0.35f + 0.4f * h), tam.y * 0.5f, 0, 1.0f * s);
		const ImVec4 txt = lleno ? ImVec4(0.04f, 0.05f, 0.10f, 1.0f) : color;
		dl->AddText(ImVec2(p0.x + (tam.x - t.x) * 0.5f, p0.y + (tam.y - t.y) * 0.5f), col(txt), texto);
		return clic;
	}

	/// Fondo de tarjeta con borde que brilla al pasar el mouse (h = 0..1).
	inline void tarjeta(ImDrawList* dl, ImVec2 a, ImVec2 b, const ImVec4& fondo, const ImVec4& acento, float h, float s, bool activa = false) {
		const float r = 14.0f * s;
		dl->AddRectFilled(a, b, col(fondo), r);
		if (activa) dl->AddRectFilled(a, b, col(acento, 0.10f), r);
		dl->AddRect(a, b, col(acento, activa ? 0.9f : 0.12f + 0.45f * h), r, 0, (activa ? 2.0f : 1.0f) * s);
	}

	/// Interruptor animado (pista + perilla). Devuelve true al clic; quien llama cambia el valor.
	inline bool interruptor(const char* id, bool activo, const ImVec4& color, float s) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 tam(42.0f * s, 22.0f * s);
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const ImGuiID k = ImGui::GetID(id);
		const bool clic = ImGui::InvisibleButton(id, tam);
		if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGuiStorage* st = ImGui::GetStateStorage();
		const float meta = activo ? 1.0f : 0.0f;
		const float v = acercar(st->GetFloat(k, meta), meta, 16.0f);
		st->SetFloat(k, v);
		const ImVec4 pista = mezclar(ImVec4(1, 1, 1, 0.12f), ImVec4(color.x, color.y, color.z, 0.85f), v);
		dl->AddRectFilled(p, ImVec2(p.x + tam.x, p.y + tam.y), col(pista), tam.y * 0.5f);
		const float r = tam.y * 0.5f;
		dl->AddCircleFilled(ImVec2(p.x + r + (tam.x - tam.y) * v, p.y + r), r - 3.0f * s, col(ImVec4(1, 1, 1, 1)));
		return clic;
	}

	/// Color estable a partir de un texto (avatares).
	inline ImVec4 colorDe(const char* t) {
		unsigned h = 2166136261u;
		for (const char* c = t; *c; ++c) h = (h ^ static_cast<unsigned char>(*c)) * 16777619u;
		static const ImVec4 paleta[] = {
			{0.00f,0.90f,1.00f,1}, {0.55f,0.36f,0.96f,1}, {0.98f,0.45f,0.60f,1},
			{0.30f,0.85f,0.55f,1}, {0.98f,0.70f,0.25f,1}, {0.40f,0.60f,1.00f,1} };
		return paleta[h % 6];
	}

}
