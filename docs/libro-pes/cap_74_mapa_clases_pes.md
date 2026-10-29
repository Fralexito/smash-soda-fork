# cap_74 — Mapa de clases de PES2021.exe (RTTI completo)

Extensión de la Parte 1 del mapa del Fox Engine (`informes/datos/rea/mapa_fox/PARTE1_flujo_partido.md`, 94 clases `match::anime::action`) al **RTTI MSVC completo** del exe (v1.1.0.0, base 0x140000000). Generado el 27-oct-2026 con `mapa_fox/rtti_full.py` (reusa `pe.py`); datos en `cap_74_clases_pes.csv` / `clases_pes.json`.

## Qué contiene
- **11912 vftables** (una por objeto localizador COL; las clases con herencia múltiple tienen varias) de **9006 TypeDescriptors**.
- Columnas del CSV: `clase` (nombre completo con espacios de nombres), `vtable_rva`/`vtable` (dirección de la tabla), `ranuras` (nº de punteros a código consecutivos), `offset_vptr` (desplazamiento del vptr dentro del objeto; ≠0 = base secundaria), `bases` (jerarquía, de CHD/BCD), `tema` (clasificación automática por nombre), `firma_slot` (16 bytes del prólogo de la primera ranura no trivial: sirve como AOB de la clase; compárese con la firma de `PARTE1`).
- Cómo se reproduce: `python rtti_full.py` en `mapa_fox\` (≈1 min).

## Clasificación por tema (automática, por nombre)

| tema | clases | para qué sirve |
|---|---:|---|
| otros | 3806 | sin clasificar |
| juego_menus | 2034 | menús, modos, ligas, equipos |
| fox_motor | 1329 | motor Fox (entidades, recursos, hilos) |
| red_online | 1223 | red y online |
| partido_reglas | 1038 | árbitro, reglas, reinicios, entrenamiento |
| input_comandos | 845 | mandos, CommandPlayer, ThinkUnit* |
| jugadores_anim | 773 | AnimePlayer, acciones, portero |
| estadio_render | 601 | estadio, césped, render, HUD 2D, público |
| balon_fisica | 109 | balón, física, colisión |
| camara | 98 | cámaras y repetición |
| sonido_video | 56 | sonido y vídeo |

## Espacios de nombres principales

| espacio | clases |
|---|---:|
| `commandautomake` | 2820 |
| `menu` | 1806 |
| `process` | 1181 |
| `mode::offline` | 625 |
| `command` | 484 |
| `fox::gr` | 415 |
| `cobra::rd` | 385 |
| `onlinemode` | 239 |
| `menusystem` | 200 |
| `fox` | 182 |
| `fox::ph` | 144 |
| `match` | 143 |
| `multiplaycommandautomake` | 138 |
| `onlinesystem` | 137 |
| `fox::impl` | 124 |
| `match::player` | 119 |
| `match::ai` | 112 |
| `matchPlan` | 112 |
| `cobra::cfg` | 109 |
| `match::anime` | 103 |
| `fox::grx` | 92 |
| `constant` | 87 |
| `fox::geo` | 86 |
| `process::mail` | 86 |
| `fox::fx` | 80 |
| `match::pad` | 79 |
| `kps::domain` | 76 |
| `menuutility` | 72 |
| `match2D::Screen` | 68 |
| `match::registry` | 51 |
| `cobra::And` | 48 |
| `match::record` | 46 |
| `onlinelogin` | 45 |
| `sound::app` | 44 |
| `cobra::rc` | 42 |
| `fox::anim` | 41 |
| `ntl` | 41 |
| `tpp::ef` | 40 |
| `menuutility::OwnerEventListTaskJudge` | 39 |
| `draw::load` | 32 |
| `draw` | 30 |
| `match::camera` | 30 |
| `sys` | 30 |
| `fox::ncl` | 29 |
| `cobra::me` | 26 |

## Clases clave por tema


### Partido: núcleo (`match::` sin `anime`/`ai`/`pad`) (131)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `match::Analyze` | 0x259d5b8 | 2 | match::record::Receiver |
| `match::AnimeCollision` | 0x25ef020 | 1 |  |
| `match::AnimePlayer` | 0x25e34d8 | 5 | match::AnimePlayerRetryWork;draw::anime::Human;draw::anime::HumanRetry |
| `match::AudienceCameraPlugin` | 0x2bd3ec8 | 8 | draw::CameraPluginInterface |
| `match::BallControl` | 0x25988b0 | 18 | match::BallControlRetryWork;match_system::ObjectInterface;fox::SharedO |
| `match::BallInfoUpdateDropPoint` | 0x2608b20 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateDropPointLast` | 0x2608b58 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateFutureDataAll` | 0x2608ab0 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateFutureDataLast` | 0x2608bc8 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateReceiveSpeedPos` | 0x2608b90 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateTopPoint` | 0x2608ae8 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BeginListener` | 0x2598fb8 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::CameraCaptureML` | 0x2bf75a8 | 8 | draw::CameraPluginInterface |
| `match::CameraListener` | 0x2599578 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::CameraLow` | 0x2bd3c80 | 8 | draw::CameraPluginInterface |
| `match::CameraOffside` | 0x2bd3df8 | 10 | match::CameraReplay;draw::CameraPluginInterface |
| `match::CameraPlay` | 0x2bd3d58 | 8 | draw::CameraPluginInterface |
| `match::CameraShoot` | 0x2bd3da0 | 10 | match::CameraReplay;draw::CameraPluginInterface |
| `match::CameraUp` | 0x2bd3cc8 | 8 | draw::CameraPluginInterface |
| `match::CameraWalk` | 0x2bd3d10 | 8 | draw::CameraPluginInterface |
| `match::CollisionEndListener` | 0x259a170 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::CollisionInitListener` | 0x259a150 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::CollisionListener` | 0x259a130 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::Command` | 0x260d060 | 18 | match::CommandRetryWork;match_system::ObjectInterface;fox::SharedObjec |
| `match::CommandListener` | 0x2599d18 | 7 | match::CommandListenerRetryWork;cobra::game::Listener;cobra::game::imp |
| `match::CommandPlayer` | 0x2610df0 | 5 | match::CommandPlayerRetryWork;match::CommandPlayerBase |
| `match::CommandPlayerBase` | 0x2610dc0 | 5 |  |
| `match::CommitRegistryListener` | 0x259b1d8 | 5 | match::ObjectListener;cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::CursorManager` | 0x259bd90 | 2 | match::CursorManagerRetryWork;match::record::Receiver |
| `match::DataManagerInterface` | 0x2653e80 | 11 |  |
| `match::DataManagerReplay` | 0x2654040 | 12 | ?;match::DataManagerInterface |
| `match::DemochatReceiver` | 0x2bf6468 | 8 | sys::TaskUnit |
| `match::DrawPluginInterface` | 0x2bba290 | 9 |  |
| `match::EndListener` | 0x25991e8 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::ExecDrawListener` | 0x2596e30 | 5 | match::ObjectListener;cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::ExecListener` | 0x259b1a8 | 5 | match::ObjectListener;cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::GoalnetFilm` | 0x25aad90 | 1 |  |
| `match::GoalnetFilmSheaf` | 0x25ab0e0 | 2 |  |
| `match::GoalnetFilmSheafIn` | 0x25ab110 | 2 | match::GoalnetFilmSheaf |
| `match::GoalnetFilmSheafOut` | 0x25ab0f8 | 2 | match::GoalnetFilmSheaf |
| `match::Highlight` | 0x2bd4238 | 14 | match::PlaybackController |
| `match::HighlightManager` | 0x2654228 | 1 |  |
| `match::Human` | 0x2612fb8 | 20 | match::HumanRetryWork;match_system::ObjectInterface;fox::SharedObject |
| `match::Injury` | 0x259ce20 | 1 |  |
| `match::Main` | 0x2597020 | 18 | match_system::ObjectInterface;fox::SharedObject |
| `match::Match` | 0x2596dc8 | 3 | match::MatchRetryWork |
| `match::MatchControl` | 0x25984c0 | 20 | match::MatchControlRetryWork;match_system::ObjectInterface;fox::Shared |
| `match::ObjectListener` | 0x259b178 | 5 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::Online` | 0x2bf1a28 | 105 | match::registry::online::Interface |
| `match::OnlineAbnormalEndSender` | 0x2bf57e0 | 3 |  |
| `match::OnlineAbnormalEndSenderImplement` | 0x2bf5800 | 6 | match::OnlineAbnormalEndSender |
| `match::OnlineCommandBinary` | 0x2bf6088 | 1 |  |
| `match::OnlineCommandController` | 0x2bf4318 | 43 |  |
| `match::OnlineCommandListener` | 0x2bf38e0 | 7 | match::CommandListener;match::CommandListenerRetryWork;cobra::game::Li |
| `match::OnlineCommandReceiver` | 0x2bf5e20 | 1 |  |
| `match::OnlineCommandSender` | 0x2bf6078 | 1 |  |
| `match::OnlinePause` | 0x2bf3ad8 | 19 |  |
| `match::OnlineProfile` | 0x2bf3060 | 1 |  |
| `match::ParallelListener` | 0x2598e08 | 3 |  |
| `match::ParameterGameCommandPriority` | 0x2bf5af8 | 1 |  |
| `match::ParsedAnimeInfo` | 0x25dc588 | 2 | sys::File |
| `match::PlaybackController` | 0x2bd3548 | 14 |  |
| `match::Player` | 0x26329a8 | 20 | match::PlayerRetryWork;match::Human;match::HumanRetryWork;match_system |
| `match::PostCollisionListener` | 0x2596de8 | 5 | match::ObjectListener;cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::RagdollManager` | 0x259da88 | 1 |  |
| `match::RandomControl` | 0x259bc00 | 2 | match::RandomControlRetryWork;match::record::Receiver |
| `match::RecordControl` | 0x2598c38 | 18 | match_system::ObjectInterface;fox::SharedObject |
| `match::RecordListener` | 0x259a350 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::RecordReplay` | 0x259dcb8 | 1 |  |
| `match::RecordReplayListener` | 0x259a708 | 3 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::Replay` | 0x2bd3e50 | 14 | match::PlaybackController |
| `match::ReplayBall` | 0x2bf7a68 | 22 | match::ReplayObjectInterface;draw::CameraTargetInterface;fox::SharedOb |
| `match::ReplayCameraListener` | 0x2bf82b8 | 9 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::ReplayHuman` | 0x2bf7d58 | 23 | match::ReplayObjectInterface;draw::CameraTargetInterface;fox::SharedOb |
| `match::ReplayObject` | 0x2bf7ff8 | 24 | match::ReplayObjectInterface;draw::CameraTargetInterface;fox::SharedOb |
| `match::ReplayObjectListener` | 0x2bf6fd0 | 9 | cobra::game::Listener;cobra::game::impl::ObjBase |
| `match::ReplayObjectPanel` | 0x2bf7040 | 24 | match::ReplayObject;match::ReplayObjectInterface;draw::CameraTargetInt |
| `match::ReplayObjectShootTarget` | 0x2bf7120 | 24 | match::ReplayObject;match::ReplayObjectInterface;draw::CameraTargetInt |
| `match::ReplayObjectThroughPassTarget` | 0x2bf7200 | 24 | match::ReplayObject;match::ReplayObjectInterface;draw::CameraTargetInt |
| `match::RouteEventParameter` | 0x259f538 | 6 | fox::DataElement;fox::Entity;? |
| `match::SituationBallData` | 0x25a0ac8 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| `match::SituationBallDataBody` | 0x25a0b48 | 25 | fox::TransformDataBody;fox::DataBody;fox::Entity;? |
| `match::SituationBallDataCallback` | 0x25a0c30 | 3 | cobra::game::util::LoopholeCallback |
| `match::SituationData` | 0x259e5b0 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| `match::SituationDataBody` | 0x259e630 | 25 | fox::TransformDataBody;fox::DataBody;fox::Entity;? |
| `match::SituationDataCallback` | 0x259e828 | 3 | cobra::game::util::LoopholeCallback |
| `match::SituationObjectData` | 0x25a1098 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| `match::SituationObjectDataBody` | 0x25a1118 | 25 | fox::TransformDataBody;fox::DataBody;fox::Entity;? |
| `match::SituationObjectDataCallback` | 0x25a1200 | 3 | cobra::game::util::LoopholeCallback |
| `match::SituationPlayerData` | 0x25a17b0 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| … 41 más en el CSV | | | |

### Mando y órdenes: `match::pad::*` y `match::CommandPlayer*` (81)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `match::CommandPlayer` | 0x2610df0 | 5 | match::CommandPlayerRetryWork;match::CommandPlayerBase |
| `match::CommandPlayerBase` | 0x2610dc0 | 5 |  |
| `match::pad::ThinkUnitAdvanceShoot` | 0x260fbf0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase;match::pad::Think |
| `match::pad::ThinkUnitAdvanceThroughPass` | 0x260fe50 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitAutoRestart` | 0x2610570 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitBase` | 0x260dc78 | 18 |  |
| `match::pad::ThinkUnitBlockTestAutoRestart` | 0x2610608 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCameraTarget` | 0x2610278 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitClear` | 0x260fee8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCornerKickDefenceType` | 0x2610900 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCornerKickTactics` | 0x2610868 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCursorChange` | 0x260eb50 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCursorChangeKeeper` | 0x260ec80 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCursorChangeManual` | 0x260ebe8 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitDemoChat` | 0x2610440 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitDemoSkip` | 0x260f990 | 18 | match::pad::ThinkUnitSkip;match::pad::ThinkUnitPadId;match::pad::Think |
| `match::pad::ThinkUnitDive` | 0x260e398 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitDribble` | 0x260e1d0 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFeint` | 0x260dd10 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreeKickPositionChange` | 0x260ea20 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreeKickTactics` | 0x260eab8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreeMove` | 0x260e138 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreekickKeeperMove` | 0x260e988 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreekickTypeChange` | 0x260e728 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreekickWallJump` | 0x260e7c0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreekickWallNotJump` | 0x260e858 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFreekickWallPress` | 0x260e8f0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitFriendPress` | 0x26101e0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitGoalkickPassSupport` | 0x2610738 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitInteractiveGoalDemo` | 0x260fac0 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperAutoMove` | 0x260edb0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperAutoPuntkick` | 0x260f1d8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperBlock` | 0x260eee0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperCoaching` | 0x260f140 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperDropBall` | 0x260f0a8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickAction` | 0x260e560 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickLayer` | 0x260e5f8 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickMove` | 0x260e4c8 | 18 | match::pad::ThinkUnitFreeMove;match::pad::ThinkUnitStickKind;match::pa |
| `match::pad::ThinkUnitKeeperPickupBall` | 0x260f010 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPress` | 0x2610148 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperTackle` | 0x260ef78 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperTrapMove` | 0x260ee48 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKickFeint` | 0x260dda8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKickOffChange` | 0x260f8f8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKickerSelect` | 0x26104d8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKickoff` | 0x260f860 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitLineControlBack` | 0x260f308 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitLineControlFront` | 0x260f270 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitLongPass` | 0x260fd20 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitManualOneTwo` | 0x260df70 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitMoveStopFront` | 0x260e268 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitMoveStopGoal` | 0x260e300 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitOutofPlaySkip` | 0x260fa28 | 18 | match::pad::ThinkUnitSkip;match::pad::ThinkUnitPadId;match::pad::Think |
| `match::pad::ThinkUnitPassRequest` | 0x2610310 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitPause` | 0x26103a8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitPenaltykick` | 0x26112b8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitPenaltykickGuide` | 0x260e690 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase;match::pad::T |
| `match::pad::ThinkUnitPress` | 0x26100b0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitQucikMemberChange` | 0x2610a30 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitSetplayGuide` | 0x2610998 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitShoot` | 0x260fb58 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitShortCorner` | 0x26107d0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitShortPass` | 0x260fc88 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitSliding` | 0x2610018 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitSuperCancel` | 0x260e430 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitSupportRequest` | 0x260e0a0 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitTackle` | 0x260ff80 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitTacticsAttackLevelDown` | 0x260f600 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitTacticsAttackLevelUp` | 0x260f568 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitTacticsBack` | 0x260f438 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| … 11 más en el CSV | | | |

### IA de jugadores `match::ai::*` (112)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `match::ai::ActionInterface` | 0x25c5858 | 4 |  |
| `match::ai::ActionMove` | 0x25c5880 | 23 | match::ai::ActionInterface |
| `match::ai::ActionSelector` | 0x25cd358 | 4 |  |
| `match::ai::ActionSelectorCenteringGet` | 0x25cde88 | 4 | match::ai::ActionSelector |
| `match::ai::ActionSelectorChanceSpaceRun` | 0x25c61b0 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorCounterSpaceRun` | 0x25c61f0 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorDiagonalRun` | 0x25cdd18 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorGoalGet` | 0x25c6318 | 4 | match::ai::ActionSelector |
| `match::ai::ActionSelectorLineBreak` | 0x25c6270 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorManager` | 0x25c6380 | 1 | match::ai::ActionSelectorManagerRetryWork |
| `match::ai::ActionSelectorOverlap` | 0x25c6148 | 4 | match::ai::ActionSelector |
| `match::ai::ActionSelectorOverlapCB` | 0x25c62f0 | 4 | match::ai::ActionSelector |
| `match::ai::ActionSelectorPostPlay` | 0x25c6340 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorPullAway` | 0x25c62b0 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::ActionSelectorScore` | 0x25c6170 | 7 | match::ai::ActionSelector |
| `match::ai::ActionSelectorSecondLineSpaceRun` | 0x25c6230 | 7 | match::ai::ActionSelectorScore;match::ai::ActionSelector |
| `match::ai::Avoid` | 0x25cc670 | 2 |  |
| `match::ai::AvoidArea` | 0x262fd88 | 2 | match::ai::Avoid |
| `match::ai::AvoidPlayer` | 0x262fd60 | 2 | match::ai::Avoid |
| `match::ai::Judge` | 0x25c50f0 | 1 | match::ai::JudgeRetryWork |
| `match::ai::PlayerAIBase` | 0x262fd50 | 1 |  |
| `match::ai::PlayerDefence` | 0x2bed9c0 | 1 | match::ai::PlayerAIBase |
| `match::ai::PlayerGKAuto` | 0x262fd78 | 1 | match::ai::PlayerAIBase |
| `match::ai::PlayerOffence` | 0x2bef218 | 1 | match::ai::PlayerAIBase |
| `match::ai::SubConcept` | 0x25c3cc0 | 7 |  |
| `match::ai::SubConceptChangeTriangle` | 0x25c3d40 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptCounterAfterGkCatch` | 0x25c3d80 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptCreateSpaceOneTwo` | 0x25c4200 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptDiagonalPostPlay` | 0x25c4480 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptDisorderByOneTwo` | 0x25c4100 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptDoublePress` | 0x25c4840 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptEarlyCross` | 0x25c3f40 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptFastCounter` | 0x25c3e00 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptForeCheckLineDown` | 0x25c4500 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptForeCheckLineUp` | 0x25c4540 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptForeCheckPress` | 0x25c44c0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptForwardPassAndLayOff` | 0x25c3f80 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptLongPassFlick` | 0x25c3e40 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptLongPassLayOffAndBreakthroughToDefenceLine` | 0x25c3ec0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptLongPassLayOffAndShoot` | 0x25c3f00 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptLongPassLayOffAndToSide` | 0x25c3e80 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptMoveDownMakeSpaceSide` | 0x25c4440 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptNetDefenceCentralSurroundingPress` | 0x25c4740 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptNetDefenceInducementIntoTheCenter` | 0x25c4700 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptOneTwoPassCutIn` | 0x25c43c0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptOneTwoPassFromSideToCenter` | 0x25c4380 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptPassAndGoTriangle` | 0x25c3fc0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptPassCut` | 0x25c4780 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptPenetrateInsideOneTwo` | 0x25c4000 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptPossessionMfJoinDefenceLine` | 0x25c3d00 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptRetreatBlockCreate` | 0x25c45c0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptRetreatBlockForwardPass` | 0x25c4640 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptRetreatCrossBlockCreate` | 0x25c4680 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptRetreatPressBack` | 0x25c4600 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptReverseSideCounter` | 0x25c46c0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptSealOffInducementToTheSides` | 0x25c47c0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptSealOffSideSurroundingPress` | 0x25c4800 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptSideChangeToWeakSide` | 0x25c4240 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptSideCounter` | 0x25c3dc0 | 7 | match::ai::SubConcept |
| `match::ai::SubConceptSideCrossRunning` | 0x25c4140 | 7 | match::ai::SubConcept |
| … 52 más en el CSV | | | |

### Física del motor `fox::ph::*` (cuerpos, restricciones, mundo) (134)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `fox::ph::Action` | 0x2a5b928 | 7 | fox::ph::MutualConnectionNode;fox::ph::ReferenceObject |
| `fox::ph::ActionEntity` | 0x2a5d5d8 | 11 | fox::SharedObject |
| `fox::ph::ActionParam` | 0x2a51b90 | 8 | fox::Entity;? |
| `fox::ph::AfterUpdateJob` | 0x2a5ffd0 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::AngularSpringAction` | 0x2a5b968 | 7 | fox::ph::BinaryAction;fox::ph::Action;fox::ph::MutualConnectionNode;fo |
| `fox::ph::BallsocketConstraint` | 0x2a55a00 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::BallsocketConstraintCore` | 0x2a5b830 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::BallsocketConstraintEntity` | 0x2a5b850 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::BallsocketConstraintParam` | 0x2a555d0 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::BigJobBase` | 0x2a5a8e8 | 10 | fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::BoxShapeCore` | 0x2a5d690 | 7 | fox::ph::ConvexShape;fox::ph::ShapeCore;fox::ph::ReferenceObject |
| `fox::ph::BroadPhaseJob` | 0x2a5fc68 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::CapsuleShapeCore` | 0x2a5b390 | 7 | fox::ph::ConvexShape;fox::ph::ShapeCore;fox::ph::ReferenceObject |
| `fox::ph::ClothHConstraint` | 0x2a58348 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::ClothHConstraintCore` | 0x2a5dcf0 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::ClothHConstraintEntity` | 0x2a5c2c0 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::ClothHConstraintParam` | 0x2a58050 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::ClothVConstraint` | 0x2a58988 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::ClothVConstraintCore` | 0x2a5c438 | 3 | fox::ph::LimitedBallsocketConstraintCore;fox::ph::BallsocketConstraint |
| `fox::ph::ClothVConstraintEntity` | 0x2a5c468 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::ClothVConstraintParam` | 0x2a58690 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::CollideCheckInputPacket` | 0x2a5d2e0 | 2 | fox::JobPacket;fox::SharedObject |
| `fox::ph::CollideCheckJob` | 0x2a5e040 | 8 | fox::ph::SmallJobBase;fox::SmallJob;fox::Job;fox::SharedObject |
| `fox::ph::CollisionBroad` | 0x2a5d4a8 | 1 |  |
| `fox::ph::CollisionCallbackInfo` | 0x2a59c08 | 2 | fox::ph::CallbackInfo;fox::SharedObject |
| `fox::ph::CollisionCallbackInside` | 0x2a59c20 | 6 | fox::ph::CollisionCallbackBase;fox::ph::ListNode;fox::ph::ReferenceObj |
| `fox::ph::Constraint` | 0x2a55230 | 17 | fox::ph::SubObject;fox::TransformData;fox::Data;fox::Entity;? |
| `fox::ph::ConstraintBase` | 0x2a5d970 | 3 | fox::ph::ReferenceObject |
| `fox::ph::ConstraintEntity` | 0x2a5b198 | 12 | fox::SharedObject |
| `fox::ph::ConstraintParam` | 0x2a519a0 | 8 | fox::Entity;? |
| `fox::ph::ContactConstraint` | 0x2a5f9f0 | 3 | fox::ph::DynamicConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceO |
| `fox::ph::CylinderConstraint` | 0x2a58ff8 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::CylinderConstraintCore` | 0x2a5ddc0 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::CylinderConstraintEntity` | 0x2a5c5c0 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::CylinderConstraintParam` | 0x2a58ce0 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::CylinderShapeCore` | 0x2a5b3d0 | 7 | fox::ph::ConvexShape;fox::ph::ShapeCore;fox::ph::ReferenceObject |
| `fox::ph::Daemon` | 0x2a52b30 | 6 | fox::Entity;? |
| `fox::ph::DaemonJob` | 0x2a5aa48 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::DefaultCollisionFilter` | 0x2a52ac8 | 6 | fox::ph::CollisionFilter;fox::ph::ReferenceObject |
| `fox::ph::DynamicConstraint` | 0x2a5f9d0 | 3 | fox::ph::ConstraintBase;fox::ph::ReferenceObject |
| `fox::ph::GroupShape` | 0x2a59c98 | 8 | fox::ph::ShapeCore;fox::ph::ReferenceObject |
| `fox::ph::HingeConstraint` | 0x2a56048 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::HingeConstraintCore` | 0x2a5bb30 | 3 | fox::ph::BallsocketConstraintCore;fox::ph::StaticConstraint;fox::ph::C |
| `fox::ph::HingeConstraintEntity` | 0x2a5bb50 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::HingeConstraintParam` | 0x2a55d50 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::Island` | 0x2a5fa10 | 1 | fox::ph::ReferenceObject |
| `fox::ph::IslandCollisionJob` | 0x2a5f190 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::IslandCollisionPostJob` | 0x2a5fea0 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::IslandFilterCache` | 0x2a5d498 | 1 |  |
| `fox::ph::IslandGeoCollisionJob` | 0x2a5e7f8 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::IslandSetupSolverJob` | 0x2a5f4d0 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::IslandSolverJob` | 0x2a5fd98 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::LargeLcpSolveJob` | 0x2a5e308 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::LcpSolveInputPacket` | 0x2a5d2f8 | 2 | fox::JobPacket;fox::SharedObject |
| `fox::ph::LcpSolveJob` | 0x2a5e1b0 | 8 | fox::ph::SmallJobBase;fox::SmallJob;fox::Job;fox::SharedObject |
| `fox::ph::LcpSolver` | 0x2a5e190 | 3 |  |
| `fox::ph::LimitedBallsocketConstraintCore` | 0x2a5b8c8 | 3 | fox::ph::BallsocketConstraintCore;fox::ph::StaticConstraint;fox::ph::C |
| `fox::ph::LimitedConstraint` | 0x2a5b8b8 | 1 |  |
| `fox::ph::LimitedHingeConstraintCore` | 0x2a5bbb8 | 3 | fox::ph::HingeConstraintCore;fox::ph::BallsocketConstraintCore;fox::ph |
| `fox::ph::MaterialInfo` | 0x2a51120 | 6 | fox::geo::MaterialInfoBase;fox::Entity;? |
| `fox::ph::MaterialManager` | 0x2a51158 | 6 | fox::Entity;? |
| `fox::ph::MultiHingeConstraint` | 0x2a576e8 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::MultiHingeConstraintCore` | 0x2a5dc30 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::MultiHingeConstraintEntity` | 0x2a5bfd0 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::MultiHingeConstraintParam` | 0x2a57280 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::MultiShoulderConstraint` | 0x2a56ec8 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::MultiShoulderConstraintCore` | 0x2a5dba0 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::MultiShoulderConstraintEntity` | 0x2a5be80 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::MultiShoulderConstraintParam` | 0x2a56ae0 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::MutualConnectionList` | 0x2a5caf0 | 1 |  |
| `fox::ph::MutualConnectionNode` | 0x2a5b800 | 1 | fox::ph::ReferenceObject |
| `fox::ph::NoTwistLimitedBallsocketConstraintCore` | 0x2a5b8f8 | 3 | fox::ph::LimitedBallsocketConstraintCore;fox::ph::BallsocketConstraint |
| `fox::ph::Object` | 0x2a52fa8 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| `fox::ph::ObjectDesc` | 0x2a533d0 | 15 | fox::Data;fox::Entity;? |
| `fox::ph::ObjectEntity` | 0x2a5b038 | 4 | fox::SharedObject |
| `fox::ph::ObjectParam` | 0x2a51d80 | 6 | fox::Entity;? |
| `fox::ph::PairIsland` | 0x2a5f700 | 1 |  |
| `fox::ph::PhModule` | 0x2a50d48 | 6 | fox::Module |
| `fox::ph::PolyhedronShape` | 0x2a54e88 | 17 | fox::ph::Shape;fox::ph::SubObject;fox::TransformData;fox::Data;fox::En |
| `fox::ph::PolyhedronShapeCore` | 0x2a5d870 | 7 | fox::ph::ConvexShape;fox::ph::ShapeCore;fox::ph::ReferenceObject |
| `fox::ph::PolyhedronShapeEntity` | 0x2a5b500 | 6 | fox::ph::ShapeEntity;fox::SharedObject |
| `fox::ph::PolyhedronShapeParam` | 0x2a54b70 | 8 | fox::ph::ShapeParam;fox::Entity;? |
| `fox::ph::PostDaemon` | 0x2a52b18 | 2 | fox::SharedObject |
| `fox::ph::PostDaemonJob` | 0x2a5ab58 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::PreDaemon` | 0x2a52b00 | 2 | fox::SharedObject |
| `fox::ph::PreDaemonJob` | 0x2a5a940 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::PrePreActionJob` | 0x2a5ea40 | 10 | fox::ph::BigJobBase;fox::BigJob;fox::Job;fox::SharedObject |
| `fox::ph::PrimitiveShape` | 0x2a54850 | 17 | fox::ph::Shape;fox::ph::SubObject;fox::TransformData;fox::Data;fox::En |
| `fox::ph::PrimitiveShapeEntity` | 0x2a5b318 | 6 | fox::ph::ShapeEntity;fox::SharedObject |
| `fox::ph::PrimitiveShapeParam` | 0x2a54478 | 8 | fox::ph::ShapeParam;fox::Entity;? |
| … 44 más en el CSV | | | |

### Escena y datos del balón `cobra::*Ball*`, `constant::Ball*` (59)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `cobra::cfg::BallConfig` | 0x2a924a8 | 5 | cobra::cfg::Config |
| `cobra::rc::scene::BallImpl` | 0x2c6b048 | 1 | cobra::rc::scene::DynamicObject;cobra::rc::Object |
| `cobra::rd::BallData` | 0x2a68ee0 | 15 | fox::Data;fox::Entity;? |
| `cobra::rd::BallDataBody` | 0x2a68f60 | 23 | fox::DataBody;fox::Entity;? |
| `cobra::rd::SceneBallNode` | 0x2a9ec68 | 14 | cobra::rd::TransformNode;cobra::rd::SceneNode;fox::SharedObject |
| `constant::Ball` | 0x2b57448 | 3 | constant::ConstantData;constant::BallBase |
| `constant::Ballplayer` | 0x2b5adc8 | 3 | constant::ConstantData;constant::BallplayerBase |
| `constant::BallplayerAnalyze` | 0x2b4d978 | 3 | constant::ConstantData;constant::BallplayerAnalyzeBase |
| `constant::BallplayerClear` | 0x2b4dd18 | 3 | constant::ConstantData;constant::BallplayerClearBase |
| `constant::BallplayerDebug` | 0x2b4f878 | 3 | constant::ConstantData;constant::BallplayerDebugBase |
| `constant::BallplayerDribble` | 0x2b4fa28 | 3 | constant::ConstantData;constant::BallplayerDribbleBase |
| `constant::BallplayerFeint` | 0x2b50338 | 3 | constant::ConstantData;constant::BallplayerFeintBase |
| `constant::BallplayerGk` | 0x2b516c8 | 3 | constant::ConstantData;constant::BallplayerGkBase |
| `constant::BallplayerPass` | 0x2b53c38 | 3 | constant::ConstantData;constant::BallplayerPassBase |
| `constant::BallplayerPlayImage` | 0x2b548c8 | 3 | constant::ConstantData;constant::BallplayerPlayImageBase |
| `constant::BallplayerSetplay` | 0x2b55918 | 3 | constant::ConstantData;constant::BallplayerSetplayBase |
| `constant::BallplayerShoot` | 0x2b568f8 | 3 | constant::ConstantData;constant::BallplayerShootBase |
| `draw::Ball` | 0x2b8b9b0 | 1 |  |
| `draw::load::BallList` | 0x2b61f30 | 18 | ?;draw::load::ListInterface;cobra::rc::LoadCallback;fox::SharedObject |
| `fox::ph::BallsocketConstraint` | 0x2a55a00 | 17 | fox::ph::Constraint;fox::ph::SubObject;fox::TransformData;fox::Data;fo |
| `fox::ph::BallsocketConstraintCore` | 0x2a5b830 | 3 | fox::ph::StaticConstraint;fox::ph::ConstraintBase;fox::ph::ReferenceOb |
| `fox::ph::BallsocketConstraintEntity` | 0x2a5b850 | 12 | fox::ph::ConstraintEntity;fox::SharedObject |
| `fox::ph::BallsocketConstraintParam` | 0x2a555d0 | 8 | fox::ph::ConstraintParam;fox::Entity;? |
| `fox::ph::LimitedBallsocketConstraintCore` | 0x2a5b8c8 | 3 | fox::ph::BallsocketConstraintCore;fox::ph::StaticConstraint;fox::ph::C |
| `fox::ph::NoTwistLimitedBallsocketConstraintCore` | 0x2a5b8f8 | 3 | fox::ph::LimitedBallsocketConstraintCore;fox::ph::BallsocketConstraint |
| `fox::ph::impl::BallsocketConstraintPropertyHandler` | 0x2a55850 | 13 | fox::PropertyEventHandler |
| `match::BallControl` | 0x25988b0 | 18 | match::BallControlRetryWork;match_system::ObjectInterface;fox::SharedO |
| `match::BallInfoUpdateDropPoint` | 0x2608b20 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateDropPointLast` | 0x2608b58 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateFutureDataAll` | 0x2608ab0 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateFutureDataLast` | 0x2608bc8 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateReceiveSpeedPos` | 0x2608b90 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::BallInfoUpdateTopPoint` | 0x2608ae8 | 6 | match::ball::CallBackFromCalcFutureData |
| `match::ReplayBall` | 0x2bf7a68 | 22 | match::ReplayObjectInterface;draw::CameraTargetInterface;fox::SharedOb |
| `match::SituationBallData` | 0x25a0ac8 | 15 | fox::TransformData;fox::Data;fox::Entity;? |
| `match::SituationBallDataBody` | 0x25a0b48 | 25 | fox::TransformDataBody;fox::DataBody;fox::Entity;? |
| `match::SituationBallDataCallback` | 0x25a0c30 | 3 | cobra::game::util::LoopholeCallback |
| `match::StepupTutorialClearingTheBall` | 0x25a4a00 | 41 | match::StepupTutorialBase |
| `match::ai::bp::BallPlayer` | 0x2bef090 | 1 | match::ai::PlayerAIBase |
| `match::ai::bp::ThinkUnitPassDirectHighBall` | 0x2beeed0 | 3 | match::ai::bp::ThinkUnitBase |
| `match::anime::action::goal_keeper::DropBall` | 0x25e04f8 | 28 | match::anime::action::Base |
| `match::anime::action::goal_keeper::PickupBall` | 0x25e05e0 | 28 | match::anime::action::Base |
| `match::camera::plugin::FreeTrainingBallSetCamera` | 0x260a208 | 8 | draw::CameraPluginInterface |
| `match::pad::ThinkUnitKeeperDropBall` | 0x260f0a8 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPickupBall` | 0x260f010 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::player::ActionKeeperDropBall` | 0x263ae18 | 8 | match::ai::ActionBall;match::ai::ActionInterface |
| `match::player::ActionKeeperPickupBall` | 0x2632360 | 8 | match::ai::ActionBall;match::ai::ActionInterface |
| `match::player::PassGetRouteBallAvoid` | 0x26300e0 | 15 | match::player::PassGetRouteInterface |
| `match::player::PassGetRouteBallFollow` | 0x26339d8 | 15 | match::player::PassGetRouteMoveStraightTrapRun;match::player::PassGetR |
| `match::record::ObserverBallArea` | 0x2640d90 | 4 | match::record::Observer |
| … 9 más en el CSV | | | |

### Registro de modelos 2D `match2D::*` (HUD: barra de potencia, cursores) (68)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `match2D::Model::Manager` | 0x2bd6630 | 6 | sys::TaskUnit |
| `match2D::Screen::Base` | 0x2bd9750 | 23 |  |
| `match2D::Screen::ChatSelectIcon` | 0x2be2150 | 23 | match2D::Screen::Base |
| `match2D::Screen::CoopNamePlate` | 0x2bdae28 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoCommentary` | 0x2bdc028 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoCompeWinner` | 0x2be7a30 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoGoalAssist` | 0x2be8130 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoMatchCard` | 0x2bdc588 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoMemberFormation` | 0x2bdd450 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoMemberSwap` | 0x2be8700 | 23 | match2D::Screen::Base |
| `match2D::Screen::DemoScoreVersus` | 0x2be94c0 | 23 | match2D::Screen::Base |
| `match2D::Screen::Event` | 0x2be0ec0 | 23 | match2D::Screen::Base |
| `match2D::Screen::Footer` | 0x2bd8a08 | 74 | menusystem::Window;flow::TaskState;sys::TaskUnit;flow::PostProcessMake |
| `match2D::Screen::GameSpeed` | 0x2be15c8 | 23 | match2D::Screen::Base |
| `match2D::Screen::GoalPerformance` | 0x2be1a88 | 23 | match2D::Screen::Base |
| `match2D::Screen::InplayChatSelect` | 0x2be1e48 | 23 | match2D::Screen::Base |
| `match2D::Screen::KickerSelect` | 0x2be2670 | 74 | menusystem::Window;flow::TaskState;sys::TaskUnit;flow::PostProcessMake |
| `match2D::Screen::LoadCallbackButtonPressBase` | 0x2beb458 | 5 | cobra::rc::LoadCallback;fox::SharedObject |
| `match2D::Screen::LoadCallbackCoopDemoButtonPressHint` | 0x2bddb58 | 5 | cobra::rc::LoadCallback;fox::SharedObject |
| `match2D::Screen::LoadCallbackFloatingScore` | 0x2bdec78 | 5 | cobra::rc::LoadCallback;fox::SharedObject |
| `match2D::Screen::LoadCallbackInplayChatMessage` | 0x2be0210 | 5 | cobra::rc::LoadCallback;fox::SharedObject |
| `match2D::Screen::Manager` | 0x2bd60f0 | 6 | sys::TaskUnit |
| `match2D::Screen::Message` | 0x2be1208 | 23 | match2D::Screen::Base |
| `match2D::Screen::ModelBase` | 0x2bdb198 | 23 |  |
| `match2D::Screen::ModelButtonPressBase` | 0x2beb488 | 28 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelButtonPressHint` | 0x2bddef8 | 28 | match2D::Screen::ModelButtonPressBase;match2D::Screen::ModelBase |
| `match2D::Screen::ModelButtonPressTitle` | 0x2bdd7a0 | 28 | match2D::Screen::ModelButtonPressBase;match2D::Screen::ModelBase |
| `match2D::Screen::ModelChatIcon` | 0x2be0568 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCoopDemoButtonPressHint` | 0x2bddb88 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCoopDemoCursor` | 0x2bdb7f0 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCoopNamePlate` | 0x2bdb500 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCursor` | 0x2bde210 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCursorName` | 0x2bde5a8 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelCursorNameOnline` | 0x2bde8a0 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelFloatingScore` | 0x2bdeca8 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelGauge` | 0x2bdef90 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelInplayChatMessage` | 0x2be0240 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelOffside` | 0x2bdf290 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelPlate` | 0x2bdfb98 | 23 | match2D::Screen::ModelBase |
| `match2D::Screen::ModelRoleControlCursor` | 0x2bdf590 | 23 | match2D::Screen::ModelBase |
| … 28 más en el CSV | | | |

### Modos de juego `mode::*` (625)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `mode::offline::?A0x3378f1ad::ChapterInterface` | 0x27f6288 | 1 |  |
| `mode::offline::?A0x9e70f198::TransferInfoGetter` | 0x27ea6d8 | 2 | tmpdb::ITransferInfoGetter |
| `mode::offline::?A0xd650ea10::ConditionInterface` | 0x27eba28 | 1 |  |
| `mode::offline::AcceptComOffer` | 0x27ec1e8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquireFailureProceed` | 0x27ec210 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquirePlayerCounter` | 0x27eb610 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquirePlayerReject` | 0x27eb670 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquireReject` | 0x27eb398 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquireSuccessProceed` | 0x27ec1b0 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquireTeamCounter` | 0x27eb3f8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::AcquireTeamReject` | 0x27eb468 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::Agent` | 0x27ed808 | 10 | mode::offline::ModeHuman;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::AppearanceBonusOption` | 0x27f7a18 | 15 | mode::offline::ContractOptionBase;mode::offline::ModeComponent;mode::o |
| `mode::offline::BaseModeObject` | 0x27e7f90 | 10 | mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::BuyOutOption` | 0x27f7c40 | 15 | mode::offline::ContractOptionBase;mode::offline::ModeComponent;mode::o |
| `mode::offline::CleanSheetBonusOption` | 0x27f80f8 | 15 | mode::offline::ContractOptionBase;mode::offline::ModeComponent;mode::o |
| `mode::offline::ClubUserCoachComponent` | 0x27fc908 | 12 | mode::offline::UserCoachComponentBase;mode::offline::UserComponentBase |
| `mode::offline::ClubUserPlayerComponent` | 0x27fc9d8 | 12 | mode::offline::UserPlayerComponentBase;mode::offline::UserComponentBas |
| `mode::offline::Coach` | 0x27f5610 | 10 | mode::offline::ModeHuman;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::CoachContract` | 0x27fcc30 | 12 | mode::offline::ContractBase;mode::offline::ModeUnit;mode::offline::Mod |
| `mode::offline::CoachTeamContract` | 0x27fcda0 | 12 | mode::offline::ContractBase;mode::offline::ModeUnit;mode::offline::Mod |
| `mode::offline::CoachTransferManager` | 0x27f7038 | 10 | mode::offline::ModeUnit;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::ComAgentComponent` | 0x27ed4f0 | 18 | mode::offline::ComAgentComponentBase;mode::offline::AgentComponentBase |
| `mode::offline::ComAgentComponentBase` | 0x27ed428 | 18 | mode::offline::AgentComponentBase;mode::offline::ModeComponent;mode::o |
| `mode::offline::ComCoachAgentComponent` | 0x27fb6f8 | 14 | mode::offline::AgentComponentBase;mode::offline::ModeComponent;mode::o |
| `mode::offline::ContractBase` | 0x27eb2f8 | 12 | mode::offline::ModeUnit;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::ContractOptionBase` | 0x27f7678 | 15 | mode::offline::ModeComponent;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::ContractRootModule` | 0x27f68b0 | 10 | mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::ContractStation` | 0x27f5380 | 10 | mode::offline::ModeUnit;mode::offline::ModeObject;sys::TaskUnit |
| `mode::offline::DetailLooting` | 0x27ebc78 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::DetailNoTime` | 0x27ebba0 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::DetailRequiredCancelFee` | 0x27ebbe8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::DetailRequiredOption` | 0x27ebc30 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::DetailWaitCom` | 0x27ebb48 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditAcquirePlayerFirst` | 0x27eb7c8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditAcquireTeamFirst` | 0x27eb768 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditPlayerSuccessRate0` | 0x27eb6a8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditRelease` | 0x27eb828 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditRenewalFirst` | 0x27eb430 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditRenewalRenegoCounter` | 0x27eb638 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditRenewalRenegoProceed` | 0x27eb5d8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditTeamSuccessRate0` | 0x27eb708 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditTransferRenegoCounterPlayer` | 0x27eb948 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditTransferRenegoCounterTeam` | 0x27eb8e8 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EditTransferRenegoProceed` | 0x27eb888 | 4 | mode::offline::SdReportBaseCondition |
| `mode::offline::EventCoachHistoryListener` | 0x27ffac8 | 3 | mode::offline::ModeBaseEventListener;? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl` | 0x27ffbd8 | 1 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateCompeResult` | 0x27ffb60 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateDayBegineCoachHistory` | 0x27ffba8 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateDayBegineConfirmSpecialTitle` | 0x27ffbc0 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateDayBegineConfrimPlayer` | 0x27ffb90 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateHistoryTitleCollectionUpdate` | 0x27ffb48 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateMonitoringDayBegin` | 0x27ffb30 | 2 | ? |
| `mode::offline::EventCoachHistoryListener::CoachHistoryImpl::StateMonitoringDayEnd` | 0x27ffb78 | 2 | ? |
| `mode::offline::EventCoachListener` | 0x27fe560 | 3 | mode::offline::ModeEventRegistListener;mode::offline::ModeBaseEventLis |
| `mode::offline::EventCoachOfferListener` | 0x2800070 | 3 | mode::offline::ModeEventRegistListener;mode::offline::ModeBaseEventLis |
| `mode::offline::EventCompeListMessage` | 0x27fe388 | 1 | mode::offline::ModeEventMessage |
| `mode::offline::EventCompeResultListener` | 0x27fe348 | 3 | mode::offline::ModeEventRegistListener;mode::offline::ModeBaseEventLis |
| `mode::offline::EventCompeResultListener::Impl` | 0x27fe480 | 1 | ? |
| `mode::offline::EventCompeResultListener::Impl::StateCheckStart` | 0x27fe3d8 | 2 | ? |
| … 565 más en el CSV | | | |

### Árbitro, reglas y reinicios (80)

| clase | vtable RVA | ranuras | bases |
|---|---|---:|---|
| `?A0x41364168::addFoulFunctor` | 0x2864ef0 | 9 | ?;?;onlinesystem::ParameterGameQueue::FunctorParent |
| `cobra::cfg::CornerFlagConfig` | 0x2a9d3d0 | 5 | cobra::cfg::Config |
| `cobra::cfg::CornerFlagConfig::CornerFlagConfigImpl` | 0x2a9d420 | 1 |  |
| `cobra::cfg::CornerFlagConfig::CornerFlagConfigImpl::AnimationElem` | 0x2a9d400 | 1 |  |
| `cobra::cfg::CornerFlagConfig::CornerFlagConfigImpl::SkeletonElem` | 0x2a9d410 | 1 |  |
| `cobra::rc::scene::CornerFlagImpl` | 0x2c6ada8 | 1 | cobra::rc::Object |
| `cobra::rd::AssistantRefereePropUnit` | 0x2ab0408 | 8 | cobra::rd::CharacterUnit;fox::SharedObject |
| `cobra::rd::RefereePropUnit` | 0x2ab01b8 | 8 | cobra::rd::CharacterUnit;fox::SharedObject |
| `command::CommandObjectCmdAddFoul` | 0x284c970 | 27 | onlinesystem::CommandObjectMio;onlinesystem::CommandObjectBase;sys::Ta |
| `commandautomake::CmdAddFoulOption` | 0x2b0e040 | 4 | fox::ncl::NclJsonMessageBase;fox::ncl::NclJsonMessage;fox::ncl::NclMsg |
| `commandautomake::CmdAddFoulResult` | 0x2b0e098 | 4 | fox::ncl::NclJsonMessageResult;fox::ncl::NclJsonMessageBase;fox::ncl:: |
| `constant::GoalKick` | 0x2b58a18 | 3 | constant::ConstantData;constant::GoalKickBase |
| `constant::Penaltykick` | 0x2b57ed8 | 3 | constant::ConstantData;constant::PenaltykickBase |
| `constant::SetplayGuideCornerKick` | 0x2b5c958 | 3 | constant::ConstantData;constant::SetplayGuideCornerKickBase |
| `constant::SetplayGuideGoalKick` | 0x2b4fd08 | 3 | constant::ConstantData;constant::SetplayGuideGoalKickBase |
| `draw::CornerFlag` | 0x2b67018 | 1 |  |
| `draw::load::CornerFlagList` | 0x2b622c0 | 18 | ?;draw::load::ListInterface;cobra::rc::LoadCallback;fox::SharedObject |
| `match2D::Screen::ModelOffside` | 0x2bdf290 | 23 | match2D::Screen::ModelBase |
| `match::CameraOffside` | 0x2bd3df8 | 10 | match::CameraReplay;draw::CameraPluginInterface |
| `match::StepupTutorialCornerKick` | 0x25a7140 | 41 | match::StepupTutorialArea;match::StepupTutorialBase |
| `match::StepupTutorialPenaltyKicks` | 0x25a6200 | 41 | match::StepupTutorialBase |
| `match::StepupTutorialPenaltyKicksGK` | 0x25a6540 | 41 | match::StepupTutorialBase |
| `match::anime::action::CornerKick` | 0x25df650 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::CornerKickLoop` | 0x25df568 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::GoalKick` | 0x25e0c98 | 29 | match::anime::action::GoalKickRetryWork;match::anime::action::Base |
| `match::anime::action::GoalKickLongPass` | 0x25e0e78 | 29 | match::anime::action::GoalKick;match::anime::action::GoalKickRetryWork |
| `match::anime::action::GoalKickShortPass` | 0x25e0d88 | 29 | match::anime::action::GoalKick;match::anime::action::GoalKickRetryWork |
| `match::anime::action::GoalKickStandby` | 0x25e0bb0 | 28 | match::anime::action::GoalKickStandbyRetryWork;match::anime::action::R |
| `match::anime::action::KickoffKicker` | 0x25e1138 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::KickoffKickerLoop` | 0x25e1050 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::KickoffOther` | 0x25e1308 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::KickoffReceiver` | 0x25e1220 | 28 | match::anime::action::Restart;match::anime::action::Base;match::anime: |
| `match::anime::action::PenaltyKick` | 0x25e15c0 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::PenaltyKickLoop` | 0x25e14d8 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::QuickRestartKick` | 0x25e0328 | 28 | match::anime::action::Restart;match::anime::action::Base |
| `match::anime::action::QuickRestartReady` | 0x25e0240 | 28 | match::anime::action::Restart;match::anime::action::Base;match::anime: |
| `match::anime::action::Referee` | 0x25e1fb8 | 28 | match::anime::action::Base |
| `match::anime::action::Restart` | 0x25df480 | 28 | match::anime::action::Base |
| `match::camera::plugin::CornerKickCamera` | 0x2609198 | 8 | draw::CameraPluginInterface |
| `match::camera::plugin::GoalKickCamera` | 0x26091e0 | 8 | draw::CameraPluginInterface |
| `match::camera::plugin::KickoffCamera` | 0x260a428 | 8 | draw::CameraPluginInterface |
| `match::camera::plugin::PenaltyKickCamera` | 0x2609228 | 8 | draw::CameraPluginInterface |
| `match::pad::ThinkUnitAutoRestart` | 0x2610570 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitBlockTestAutoRestart` | 0x2610608 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCornerKickDefenceType` | 0x2610900 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitCornerKickTactics` | 0x2610868 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickAction` | 0x260e560 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickLayer` | 0x260e5f8 | 18 | match::pad::ThinkUnitStickKind;match::pad::ThinkUnitBase |
| `match::pad::ThinkUnitKeeperPenaltykickMove` | 0x260e4c8 | 18 | match::pad::ThinkUnitFreeMove;match::pad::ThinkUnitStickKind;match::pa |
| `match::pad::ThinkUnitKickoff` | 0x260f860 | 18 | match::pad::ThinkUnitPadId;match::pad::ThinkUnitBase |
| … 30 más en el CSV | | | |

## Lo que ya sabíamos y encaja con este mapa

- `match::CommandPlayer` (vtable 0x2610df0; ctor FUN_1408B3BC0) posee el arreglo de `match::pad::ThinkUnit*` (ctor de unidades FUN_1408AD0B0). `ThinkUnitShoot` = 0x260FB58, `ThinkUnitClear` = 0x260FEE8: ver cap_73 §14.
- `match::AnimePlayer` (0x25e34d8/0x25e3508/0x25e35b0) y `match::Player` (0x26329a8) son los jugadores; el contenedor de manejadores de acción (`GetHandler`, FUN_1406D9010: `cont+0x1528[estado*8]`) y `SetState` (FUN_1406EEBB0) están en `PARTE1`.
- La física de la pelota pasa por `fox::ph::*` (no existe una clase llamada AndPh: el nombre viene de las funciones `AndPhPreProcess`). Frente B (cap_62) busca el cuerpo rígido y su velocidad.

## Uso
- Buscar una clase: `findstr /i "Corner" cap_74_clases_pes.csv`. - Para enganchar por firma: columna `firma_slot` + `vtable_rva`. - Para ver las ranuras: `fox.py`/`rtti.py` o `ListarClases.java` + `VtableClase.java`.
