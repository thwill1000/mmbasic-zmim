' Copyright (c) 2019-2025 Thomas Hugo Williams
' License MIT <https://opensource.org/licenses/MIT>
' For MMBasic 6.00

' Storage for z-machine memory
Dim m(128 * 1024 / 8 + 16)
Dim mad = Peek(VarAddr m()) + 8
Dim FILE_LEN
Dim BASE_STATIC

' Reads a byte from 'pc' and increments 'pc'
Function rp()
'!if !defined(NO_EXTRA_CHECKS)
  If pc < 0 Or pc >= FILE_LEN Then Error "Invalid address: " + fmt_hex$(pc)
'!endif
  rp = LGetByte(m(), pc)
  Inc pc
End Function

' Reads a byte from 'a' but DOES NOT increment a
Function rb(a)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= FILE_LEN Then Error "Invalid address: " + fmt_hex$(a)
'!endif
  rb = LGetByte(m(), a)
End Function

' Reads a 16-bit word from 'a' but DOES NOT increment a
Function rw(a)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= FILE_LEN - 1 Then Error "Invalid address: " + fmt_hex$(a)
'!endif
  rw = LGetByte(m(), a) * 256 + LGetByte(m(), a + 1)
End Function

' Writes byte 'x' to 'a'
Sub wb(a, x)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= BASE_STATIC Then Error "Invalid address: " + fmt_hex$(a)
  If (x And &hFF) <> x Then Error "Invalid write value: " + Str$(x)
'!endif
  Poke Byte mad + a, x
End Sub

' Writes 16-bit word 'x' to 'a'
Sub ww(a, x)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= BASE_STATIC - 1 Then Error "Invalid address: " + fmt_hex$(a)
  If (x And &hFFFF) <> x Then Error "Invalid write value: " + Str$(x)
'!endif
  Poke Byte mad + a, x \ 256
  Poke Byte mad + a + 1, x Mod 256
End Sub

Sub mem_init(f$)
  Local ad, i, j, s$, sz

  LongString Clear m()

  con.println("Loading '" + f$ + "'")
  con.print("0% ... ")

  sz = mm.info(filesize f$)
  Open f$ For Input As #1
  j = 1
  Do
    s$ = Input$(255, #1)
    If Len(s$) = 0 Then Exit Do
    LongString Append m(), s$
    Inc ad, Len(s$)
    If ad > j * sz \ 10 Then con.print(Str$(j * 10) + "% ... ") : Inc j
  Loop
  con.println("100%")
  con.println("Read " + Str$(ad) + " bytes")

  Close #1

  ' Read memory sizes from header
  FILE_LEN = 512 ' temporary value to avoid "Invalid address" error from rw()
  BASE_STATIC = rw(&h0E)
  FILE_LEN = rw(&h1A) * 2

  If FILE_LEN > ad Then Error "Story file is truncated"
End Sub
