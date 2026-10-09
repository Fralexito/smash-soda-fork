# Audit A — `shirtless_celebration.lua` (V4.2, author "alston2016")

Scope: `SiderAddons/modules/shirtless_celebration.lua` (2,423 lines, 107,621 bytes, read in full), `shirtless_celebration.ini`, `shirtless_player_profiles.ini`, the `shirtless_skin_map.bin` reference, `shirtless_celebration_debug.log`, `sider.log`.
The module in `SiderAddons/modules/` and the ConmeGOL Patch 26 copy are byte-identical (`cmp`).
Load order: `sider.ini` line 137, Lua stack position 17 (after goalscreams, before PES ID / Chatty_AutoLineup). Its asset root `cpk.root = .\livecpk\ShirtlessCelebration` is on line 115, after all the PRDX Body Model roots (lines 13–61), so it has lower priority than they do.

Note: `shirtless_skin_map.bin` and the `livecpk\ShirtlessCelebration` asset tree are **not in the audited copy**. The module would `error()` at init without them. The log shows it loaded with 29,493 records, so they exist on the user's PC.

---

## 1. Purpose

It unlocks the "take the shirt off" goal celebration (R3), which the game normally allows only for Neymar (player ID 40352 = `0x9DA0`), for **every player**. It also:
- removes the "booked player can't do it" gate (but keeps the yellow card);
- keeps the scorer's real player ID instead of the game's hard-coded Neymar ID;
- picks the scorer's skin tone (1..6) from a 29k-entry skin map and swaps in a matching body texture and hand models;
- supports four modes: naked body, undershirt, random (50/50 via `rdtsc`), and season-auto (`ctx.season`: 0 = naked, 1 = undershirt);
- supports per-player custom bodies, textures and hands (62 profiles active on the user's PC);
- routes the celebrating player's shorts and lower-body meshes to Konami-original copies, so the PRDX "Body Global" pants and socks don't mismatch the torso.

Neymar keeps the native chain.

## 2. Registered events

| Event | What it does | Frequency / cost |
|---|---|---|
| `init` (module load) | Truncates the debug log, reads the ini, scans profile folders, **copies 127 files (32.76 MB)** into `_runtime`, checks that the assets exist, reads the skin map, allocates the code cave, writes 9 code patches plus table and pointer data, saves the ini. | Once at startup (took about 3 s in the log, 19:49:19 → 19:49:22). |
| `set_teams` | `read_match_season`, then **calls `patch(ctx)` again** (re-checks all bytes, re-applies anything missing, resets the model, texture and hand slots and `data+0x0c`/`+0x10`), then logs `MATCH` and a STATE snapshot. | Once per match. |
| `after_set_conditions` | Updates the season and effective mode (writes 4 bytes to `data+0x10` and reads them back). | Once per match. |
| `livecpk_rewrite` | **Runs on every file request the game makes.** It calls `debug_snapshot` (reads 4 bytes, writes a log line only when the event counter changed), `debug_resource` (about 15 `string.find` calls; writes a line the first time it sees each phase/event/filename), and `lower_body_event_rewrite` (only after a celebration has happened: rewrites `Asset\model\character\face\real\<scorerID>\#Win\{pants_*,socks_*,thigh_*,tights_*}.fmdl` to `Asset\model\character\parts\shirtless\lower\#Win\<name>`). | Per file request. |
| `livecpk_get_filepath` | For the virtual root `Asset\model\character\parts\shirtless\lower\#Win\` plus the 17 whitelisted names, returns `<sider_dir>\livecpk\ShirtlessCelebration\Asset\...\lower\#Win\<name>`. **It does not check that the file exists.** | Per file request (cheap prefix check). |
| `livecpk_data_ready` | `debug_snapshot` plus `debug_resource("DATA_READY", ...)`. | Per file loaded. |
| `overlay_on` | Builds the overlay text and may re-resolve the season. | Per frame while the overlay is shown on this module. |
| `key_down` | PageUp (`0x21`) and PageDown (`0x22`) cycle the mode, then `save_config()` rewrites the ini. | Only while this module is the active overlay page (Sider behavior). |
| `gamepad_input` | Right stick X (`RSx`) beyond ±0.5 cycles the mode (edge-triggered), then **rewrites the ini file**. | Only while this module is the active overlay page. |

There is no `display_frame` handler and no per-frame memory write outside the overlay.

## 3. Memory writes

### 3a. Signature and base

- `memory.search_process("\x75\x1f\x3d\xa0\x9d\x00\x00\x75\x18\x8b")`: first hit, then `base = hit - 0x47B170`.
- Fallback when PES21_Hook has already patched it: `memory.search_process("\x84\xdb\x75\x1f")`, then `base = hit + 2 - 0x47B170`. **This is a 4-byte pattern that is extremely common.** The first hit is almost certainly not the right place, but the later byte checks then fail and the module refuses to patch (it errors out). It does not crash.
- If neither is found: `error("FL signature not found")`. Multiple hits are not detected; it always takes the first.
- Every other target is a **hard-coded RVA from that base**. There are no other pattern searches. The user's log confirms base = `0x140000000` (`hand_l_ptr=0x142AADFA0` = base + 0x2AADFA0).

### 3b. Code patches in game .text

All are applied in `init` and re-checked in every `set_teams`. All are checked before any write: each target must hold either the original bytes or this module's own patch, otherwise the module calls `error()` and writes nothing. They are verified again after writing. **They are never restored** (there is no unload or restore path).

| # | RVA (absolute @0x140000000) | Original bytes | Patched bytes | Meaning |
|---|---|---|---|---|
| P1 | 0x47B172 (0x14047B172) | `3D A0 9D 00 00` (cmp eax,40352) | `39 C0 90 90 90` (cmp eax,eax) | Neymar-ID gate, always true |
| P2 | 0x7D9D03 | same | same | Neymar-ID gate |
| P3 | 0x7D9D69 | same | same | Neymar-ID gate |
| P4 | 0x7DDF69 | same | same | Neymar-ID gate |
| P5 | 0x1F5FFE7 | same | same | Neymar-ID gate |
| P6 | 0x47B170 | `75 1F` (jne) | `90 90` | status (booked-player) gate removed |
| P7 | 0x47B18A | `41 0F 44 CC` (cmovz ecx,r12d) | `44 89 E1 90` (mov ecx,r12d) | booking guard forced |
| H1 | 0x1EC45E2 | `C7 43 0C A0 9D 00 00` (mov [rbx+0C],40352) | `E9 rel32 90 90` → cave+0x100, returns to 0x1EC45E9 | scorer/skin capture hook #1 (also accepts the older all-NOP variant `90 x7`) |
| H2 | 0x1EC45EB | same | `E9 rel32 90 90` → cave+0x2100, returns to 0x1EC45F2 | scorer/skin capture hook #2 |
| H3 | 0x1A97309 | `E8 E2 9B E7 FF 48 8B C8 45 8B C6 48 8D 55 00` (15 bytes) | `E9 rel32` + 10×`90` → cave+0x4800, returns to 0x1A97318 | arm material tone hook (re-does `call 0x1910EF0`) |
| H4 | 0x1A97342 | `E8 A9 9B E7 FF 48 8B C8 45 8B C6 48 8D 55 10` (15 bytes) | `E9 rel32` + 10×`90` → cave+0x4900, returns to 0x1A97351 | torso material tone hook |
| H5 | 0x1A972E3 | `83 FE 09 0F 85 AF 00 00 00` (cmp esi,9; jne) | `E9 rel32` + 4×`90` → cave+0x4A00, returns to 0x1A972EC or 0x1A9739B | body-part dispatch (slot 9/11) |
| H6 | 0x1A8A4CE | `48 8D 15 FB 22 02 01` (lea rdx,[0x2AAC7D0]) | `E9 rel32 90 90` → cave+0x4B00, returns to 0x1A8A4D5 | Konami-shorts path redirect (one shot) |

Byte ranges touched: 0x47B170–0x47B18D, 0x7D9D03–07, 0x7D9D69–6D, 0x7DDF69–6D, 0x1F5FFE7–EB, 0x1A8A4CE–D4, 0x1A972E3–EB, 0x1A97309–17, 0x1A97342–50, 0x1EC45E2–F1.

I checked the cave code by hand:
- All registers and flags used in the capture caves are saved and restored (pushfq, rax, rcx, rdx, r8–r11, then popped in reverse order).
- The material caves clobber only r11 and flags, and only right after a call, where both are volatile anyway.
- The rel32 targets of the original call and lea match the RVAs used (0x1910EF0, 0x2AAC7D0).

None of the caves calls anything except the game's own resource manager, at the same stack depth as the original call. I found no stack-alignment or register bug.

### 3c. Data writes in game .data/.rdata (pointer tables)

These are overwritten by the module and by its caves **at runtime, during every celebration**:

| RVA | Content | Written by |
|---|---|---|
| 0x3517E30 + 11×8 = **0x3517E88** | body model table slot 11 (u64 pointer) | `patch()` resets it to native naked `base+0x2AAF950`; the capture cave sets it to the custom path, the native undershirt (`+0x2AAF900`) or a profile path |
| 0x3517E90 + 11×8 = **0x3517EE8** | body texture table slot 11 | `patch()` resets it to `base+0x2AAF530`; the cave sets the tone path, undershirt (`+0x2AAFC50`) or a profile |
| **0x3517C68 / 0x3517C70** | naked hand L/R pointer slots | `patch()` resets them to `+0x2AADFA0`/`+0x2AAE000`; the cave sets tone, torso-hand or profile paths |
| read-only check | 0x3517E78 (model slot 9), 0x3517ED8 (texture slot 9), 0x3517C78/0x3517C80 (torso hand slots) | must equal the native pointers, otherwise `error()` |

The cave reads and does not write: 0x2AAF900, 0x2AAF950, 0x2AAFC50, 0x2AAF530, 0x2AADFA0, 0x2AAE000, 0x2AAE060, 0x2AAE0C0 and 0x2AAC7D0 (native path strings).

### 3d. Private code cave

- `ffi.C.VirtualAlloc` is called at addresses starting just past the highest image section, stepping by 64 KB up to 4,096 tries or about 1.75 GB, with `MEM_COMMIT|MEM_RESERVE` and **`PAGE_EXECUTE_READWRITE`**.
- The size is computed from the data, roughly 0x4C00 + 29,493×8 + hand paths + 62 profiles × 8 × 0x100, which comes to about 0x80000 (512 KB).
- It checks that the cave is within rel32 range of the hooks, otherwise `error`. It is never freed.
- Layout:
  - +0: marker `SHIRTLESS_SKIN_V4`
  - +0x40: data block: player ID, tone, event counter, actual body, mode, hand counters, profile index and pointer, shorts flag and count, Neymar flag, lower flag, plus a 64-byte copy of the scorer descriptor
  - +0x100 / +0x2100: capture caves
  - +0x4100: texture path strings
  - +0x4800 / +0x4900 / +0x4A00 / +0x4B00: other caves
  - +0x4C00: sorted skin table (binary search done in asm)
  - then hand paths and profile paths and records.

## 4. ffi / VirtualProtect / hooks

- `ffi.cdef` declares only `VirtualAlloc`. It uses `ffi.cast` for `uint64_t` and `void*`.
- It does **not** call VirtualProtect itself. Writes to .text go through Sider's `memory.write`, which handles page protection.
- It installs 6 inline jmp hooks (H1–H6) and 7 byte patches (P1–P7). It uses `memory.get_process_info()` for the section list and `fs.find_files` / `fs.make_dirs` for the folder scan.

## 5. Files written

| File | When | Size / growth |
|---|---|---|
| `SiderAddons\shirtless_celebration_debug.log` | Truncated (`"wt"`) on every game start, then appended with an open/write/close **for each line**. | Startup alone wrote 391 lines / 155,743 bytes (current file). Each match adds 1 MATCH line, STATE lines (each ~1 KB), and one RESOURCE line per unique (phase, event, filename) for any file whose path contains `character\face\real`, `parts\naked`, `parts\torso`, `character\players`, `pants_`, `shorts`, `uniform\`, … in both REQUEST and DATA_READY phases. **The seen-set is keyed by event, so the whole set is logged again after every goal celebration.** Expect hundreds of KB per session with matches (the 444 KB figure in the brief fits a session with matches); it resets each launch. |
| `modules\shirtless_celebration.ini` | At init (every launch), and on every mode change by key or right stick. | 71 bytes |
| `livecpk\ShirtlessCelebration\Asset\model\character\parts\shirtless\_runtime\players\<id>\{naked,undershirt}\...` | At **every** init: copies each short-folder profile file again. | **127 files, 32,760,098 bytes rewritten per launch** |

## 6. Error handling

- **There is no `pcall` anywhere in the module.** Every failure is a deliberate `error(...)`: signature missing, unexpected bytes, a table pointer not in the known set, the cave out of range, assets missing, a skin map that is invalid or unsorted, more than 64 profiles, verification failures.
- The checks are well designed: every target is validated **before** the first write, and nothing is written when an unknown value is found. So a version mismatch or another mod's patch makes the module stop instead of corrupting code.
- An error in `init` makes Sider report that the module failed to initialize. If that happens before `ctx.register`, the module is inactive. Patches already written are not rolled back, but `patch()` writes only after all checks pass, so no half-patched state is expected.
- Sider runs event callbacks inside a protected call, so an error in `set_teams`, `livecpk_rewrite` or another callback is logged, not a crash. Two exceptions:
  - The error could repeat on **every** file request if `livecpk_rewrite` threw (for example, `memory.read` on a freed cave can't happen here, so this is low).
  - If `patch()` in `set_teams` hits its final verification error after writing, the hooks stay active. That is harmless, because each hook is self-consistent.
- **The real crash surface is the asm**, which has no exception handling. If any hard-coded RVA does not match the EXE, the byte checks catch it first. Once checked, the caves are sound.

## 7. Log evidence

- `sider.log` 347–443: the module loaded cleanly. `OK: Lua module initialized: shirtless_celebration.lua (stack position: 17)` (line 443).
- 348: `auto profile scan found 127 resource override(s)`; 411: `loaded 62 player profiles`; 415: `six naked-body skin atlases active (29493 player records)`; 413: `all-player R3 unlock active`, meaning all the byte checks passed and every patch was applied.
- Only warning-level line, 412: `profile 56810 is absent from skin map; automatic profile uses default skin 3` (harmless).
- There are no errors, tracebacks, retries or "unsupported bytes" lines for this module in either log.
- The debug log contains only INIT, SHORT_STAGE ×127, AUTO_PROFILE_RESOURCE ×127, PROFILE_RESOURCE ×133, 1 MODE, and 1 STATE (`event=0 … actual=NOT_TRIGGERED`). There are **no MATCH, RESOURCE or LOWER_BODY lines**: `set_teams` never fired in the captured session.
- The sider.log tail shows the user cycling overlay pages (shirtless_celebration was active once) and then a clean `DLL detaching … All done.`
- **So the captured session never reached a match, and the runtime hook paths were not exercised.** These logs cannot confirm or rule out an in-match crash from this module.

## 8. Keys / buttons

- Keyboard: **PageUp (VK 0x21)** cycles the mode backward, **PageDown (VK 0x22)** forward.
- Gamepad: **Right stick X** left or right (|RSx| > 0.5). PES also uses this stick in play. Sider sends these events only while this module is the active overlay page, but anyone who leaves the overlay on this page and plays will change the mode (and rewrite the ini) with every right-stick flick.
- Other modules that also read gamepad input: `camera.lua` and `Chants-Server.lua` also read `RSx`. BroadCastCam, CommonCam, DynamicWideCam, FanViewCam, PenaltyCam, ReplayCam, StadiumCam and VerticalCam also register `gamepad_input`. They don't conflict, because only the active overlay module gets the event.

## 9. Overlap table (for cross-module comparison; base 0x140000000)

| Kind | Pattern / location | Offset written | Length | Bytes written |
|---|---|---|---|---|
| AOB (anchor) | `75 1F 3D A0 9D 00 00 75 18 8B` @ RVA 0x47B170 | 0x47B170 | 2 | `90 90` |
| | same anchor | 0x47B172 | 5 | `39 C0 90 90 90` |
| | same anchor | 0x47B18A | 4 | `44 89 E1 90` |
| AOB fallback | `84 DB 75 1F` (hit+2 = RVA 0x47B170) | — | — | (base only) |
| fixed RVA | 0x7D9D03, 0x7D9D69, 0x7DDF69, 0x1F5FFE7 | each | 5 | `39 C0 90 90 90` (was `3D A0 9D 00 00`) |
| fixed RVA | 0x1EC45E2 | +0 | 7 | `E9 xx xx xx xx 90 90` (was `C7 43 0C A0 9D 00 00`) |
| fixed RVA | 0x1EC45EB | +0 | 7 | `E9 xx xx xx xx 90 90` (same original) |
| fixed RVA | 0x1A972E3 | +0 | 9 | `E9 rel32 90×4` (was `83 FE 09 0F 85 AF 00 00 00`) |
| fixed RVA | 0x1A97309 | +0 | 15 | `E9 rel32 90×10` (was `E8 E2 9B E7 FF 48 8B C8 45 8B C6 48 8D 55 00`) |
| fixed RVA | 0x1A97342 | +0 | 15 | `E9 rel32 90×10` (was `E8 A9 9B E7 FF 48 8B C8 45 8B C6 48 8D 55 10`) |
| fixed RVA | 0x1A8A4CE | +0 | 7 | `E9 rel32 90 90` (was `48 8D 15 FB 22 02 01`) |
| data ptr | 0x3517E88 (model tbl slot 11) | +0 | 8 | u64 pointer (at runtime, per celebration) |
| data ptr | 0x3517EE8 (texture tbl slot 11) | +0 | 8 | u64 pointer (at runtime) |
| data ptr | 0x3517C68 / 0x3517C70 (naked hand L/R) | +0 | 8 each | u64 pointer (at runtime) |
| new memory | VirtualAlloc RWX, ~512 KB, just after the image | — | — | caves and data |

Cross-check: none of these addresses appears among Sider's own hook addresses in `sider.log` (lines 179–272: 0x1415AD5F0, 0x141F5184E, 0x141E5528B, 0x141ED972D, …). The closest is the Sider hook at 0x141F51855 compared with this module's 0x141F5FFE7, about 58 KB apart, so no overlap. No other module in `modules/` mentions these RVAs or the Neymar compare bytes. Of the modules in the brief, those that use `memory.search_process` are SleeveBadge, Commentary-Server, Competition-Server, UIColors, StartingYearChanger, awaygoals and camera, and their patterns differ textually. The lead should compare the resolved addresses.

## 10. Risk rating: **MEDIUM** (low for a hard crash, medium for conflicts and side effects)

Why the crash risk is low:
- Strict before-write byte checks with an error instead of a blind write.
- Self-healing and checked on every `set_teams`.
- Correct register and flag saving in the caves.
- It loaded and patched without any error on the user's PC.

Why it is medium overall:
1. **It is tied to one exact EXE build.** Fifteen hard-coded RVAs, with only one AOB used as an anchor. Any other EXE (PES21_Hook variants, another FL build) leads to an error or the module being inactive. That is safe but silent for the user.
2. **The fallback signature `84 DB 75 1F` is close to meaningless.** It is only safe because of the later checks.
3. **It changes global game state at runtime, not only code.** The model, texture and hand pointer slots are overwritten during every celebration and are reset only in the next `set_teams`. Another module (or a body or kit mod that also patches those tables) would collide. Any module writing near 0x3517C68–0x3517EF0 or 0x2AAxxxx is a direct conflict.
4. **The material hooks H3/H4 run for every call of that selector, not only celebrations.** The data block starts with tone = 4 (`data+4`) and keeps the last scorer's tone, so any undershirt or torso material built later uses that tone instead of the game's fixed one. This is visual, not a crash.
5. **The one-shot hooks can hit the wrong player.** The Konami-shorts flag (`data+0x28`) and the Lua lower-body route window stay open until the next pants or thigh request, even if that request belongs to another player. This is visual.
6. **`livecpk_get_filepath` returns a path without checking the file exists.** The 17 `parts\shirtless\lower\#Win\*.fmdl` files are **not** checked at init (only `pants_001..016`, the hands, textures and `shirtless_body.fmdl` are). If those files are missing, the game gets a non-existent path for a model mid-match, which is a possible load failure or crash during the celebration.
7. **Heavy I/O:**
   - 32.8 MB of file copies on every launch (about 3 s of startup);
   - an open/append/close of the debug log for each line on the file-loading thread, during loading and after each goal;
   - the ini rewritten on every mode change.
   This can cause stutter or hitching, which is easy to mistake for "the game froze".
8. **No pcall**, and the module has `livecpk_rewrite` and `livecpk_data_ready` running on every file request. Any future runtime error would repeat thousands of times in sider.log.
9. **No runtime evidence.** The logs given contain no match, so the hooks have not been seen working on this PC.

### Mitigations

1. Make sure the 17 files in `livecpk\ShirtlessCelebration\Asset\model\character\parts\shirtless\lower\#Win\` exist: `pants_in_sub`, `pants_out_inner_sub`, `pants_out_sub`, `pants_out_sub_tight`, `socks_{long,middle,noguard,short}`, `thigh_{long,middle,short}[_classic]`, `tights_{long,middle,short}`, all `.fmdl`. Alternatively, patch `livecpk_get_filepath` to return nil when `io.open` fails.
2. To cut I/O and the risk of hitches, disable the diagnostics: make `debug_write` a no-op, or set `debug_path = nil` after INIT. Removing the `livecpk_data_ready` registration and the `debug_snapshot` / `debug_resource` calls in `livecpk_rewrite` costs nothing.
3. Stop re-copying the profiles on every launch: skip when the size already matches. Or move the 62 profiles to the `parts\shirtless\players\<id>\` legacy layout, which needs no copy.
4. Don't leave the Sider overlay on this module during play, because the right stick changes the mode. Or delete the `gamepad_input` registration.
5. To confirm, play one match with a goal and check the debug log for `MATCH`, `STATE ... actual=...`, `LOWER_BODY_ROUTE` and `LOWER_BODY_FILEPATH` lines, and sider.log for any `[shirtless_celebration.lua]` errors.
6. If a crash happens during a goal celebration, isolate it first by commenting out `lua.module = "shirtless_celebration.lua"` (sider.ini line 137). Keep the `cpk.root` line or not: it is inert without the module.
7. Make sure no other installed tool (PES21_Hook, another "shirtless for all" patch, or a CE table) patches 0x47B170–0x47B18D, the 0x7D9Dxx/0x7DDF69/0x1F5FFE7 compares, or 0x1EC45E2–F1. The module's checks will stop it with an error, but the other tool may write blindly over this module's jmp.
