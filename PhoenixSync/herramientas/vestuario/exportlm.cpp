#include "core/LigaMaster.h"
#include "core/BlobLM.h"
#include <nlohmann/json.hpp>
#include <cstdio>
using namespace mercado; using namespace mercado::lm;
static uint32_t le32(const std::vector<uint8_t>& d,size_t o){return d[o]|(d[o+1]<<8)|(d[o+2]<<16)|(uint32_t(d[o+3])<<24);}
int main(int c,char**v){
  auto g=GuardadoLM::abrir(v[1]); if(!g.ok()){std::printf("ERR %s\n",g.error.detalle.c_str());return 1;}
  int kU=-1; for(int k=0;k<528&&kU<0;k++) if(g.valor->esEquipoUsuario(k)) kU=k;
  auto e=g.valor->equipo(kU); auto id=g.valor->idOptionDe(kU); auto al=g.valor->alineacionDe(kU); auto fi=g.valor->finanzas(kU); auto fe=g.valor->fechaActual();
  auto b=BlobLM::leer(g.valor->datos());
  nlohmann::json j; j["indice"]=kU; j["nombre"]=e.valor->nombre; j["id_option"]=id.ok()?*id.valor:0;
  if(fe.ok()) j["fecha"]=std::to_string(fe.valor->anio)+"-"+std::to_string(fe.valor->mes)+"-"+std::to_string(fe.valor->dia);
  if(fi.ok()) j["finanzas"]={{"presupuesto_fichajes",fi.valor->presupuestoFichajes},{"tope_salarial",fi.valor->topeSalarial},{"sueldos",fi.valor->sueldosActuales},{"presupuesto_salarial",fi.valor->presupuestoSalarial()}};
  if(al.ok()){ j["orden"]=al.valor->orden; j["roles"]=std::vector<int>(al.valor->roles.begin(),al.valor->roles.end()); }
  for(auto& p: e.valor->plantilla){
    nlohmann::json x={{"reg",p.reg},{"pid",p.pid},{"dorsal",p.dorsal}};
    if(b.ok()){ long long o=b.valor->fichaDe(p.reg,p.pid); auto& d=b.valor->contenido();
      if(o>=0){ x["sueldo"]=uint64_t(le32(d,o+0x56))*100; x["valor"]=uint64_t(le32(d,o+0x5A))*100; auto fin=Fecha::desde(le32(d,o+0x66)); x["fin_contrato"]=std::to_string(fin.anio)+"-"+std::to_string(fin.mes)+"-"+std::to_string(fin.dia); } }
    j["plantilla"].push_back(x);
  }
  std::printf("%s\n", j.dump(1).c_str());
}
