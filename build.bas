#!/usr/local/bin/mmbasic

' Copyright (c) 2025-2026 Thomas Hugo Williams
' License MIT <https://opensource.org/licenses/MIT>
' For MMBasic 6.00

If Mm.Device$ <> "MMB4L" Then Error "Requires MMBasic for Linux"

Const PATH$ = Choice(Mm.Info$(Path) = "NONE", Cwd$, Mm.Info$(Path))
Const OLD$ = Cwd$

ChDir PATH$
Kill "bin/zmim.bas"
Kill "bin/zmim_rp2040.bas"

flags$ = "-e=1 -i=1 -n -s=1 -T -DNO_INCLUDE_GUARDS -DNO_EXTRA_CHECKS -DINLINE_CONSTANTS"
cmd$ = "sptrans " + flags$ + " src/main.bas bin/zmim.bas"
Print cmd$
System cmd$

cmd$ = "sptrans " + flags$ + " -DLOW_MEMORY " + " src/main.bas bin/zmim_rp2040.bas"
Print cmd$
System cmd$

ChDir OLD$
