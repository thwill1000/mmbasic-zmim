' Copyright (c) 2019-2025 Thomas Hugo Williams
' License MIT <https://opensource.org/licenses/MIT>
' For MMBasic 6.00

'!define USE_VIRTUAL_MEMORY

'!if defined(INLINE_CONSTANTS)
  '!replace PAGE_SIZE 512
  '!replace NUM_PHYSICAL_PAGES 80
  '!replace NUM_VIRTUAL_PAGES 256
'!else
  Const PAGE_SIZE = 512
  Const NUM_PHYSICAL_PAGES = 80
  Const NUM_VIRTUAL_PAGES = 128 * 1024 \ PAGE_SIZE
'!endif

Dim FILE_LEN                          ' length of z-machine file
                                      ' also the size of virtual memory used
Dim BASE_STATIC                       ' memory addresses below this are read on
                                      ' startup and not swapped in/out
Dim FIRST_SWAP_PAGE                   ' index of the first physical page that
                                      ' can be swapped in/out
Dim m(NUM_PHYSICAL_PAGES*PAGE_SIZE\4) ' physical z-machine memory
Dim pf                                ' counter for number of page faults
Dim MEM_FILE$                         ' the z-machine file
Dim pp_to_vp(NUM_PHYSICAL_PAGES - 1)  ' map of physical pages -> virtual pages
Dim vp_to_pp(NUM_VIRTUAL_PAGES - 1)   ' map of virtual pages -> physical pages
Dim next_page                         ' next page of physical memory to use on
                                      ' a page fault

' Reads a byte from 'pc' and increments 'pc'
Function rp()
'!if !defined(NO_EXTRA_CHECKS)
  If pc < 0 Or pc >= FILE_LEN Then Error "Invalid address: " + fmt_hex$(a)
'!endif
  Const vp = pc \ PAGE_SIZE
  Local pp = vp_to_pp(vp)
  If pp = 0 Then pp = mem_load(vp) : Inc pf
  rp = Peek(Var m(0), pp * PAGE_SIZE + (pc Mod PAGE_SIZE))
  Inc pc
End Function

' Reads a byte from 'a' but DOES NOT increment a
Function rb(a)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= FILE_LEN Then Error "Invalid address: " + fmt_hex$(a)
'!endif
  If a < BASE_STATIC Then rb = Peek(Var m(0), a) : Exit Function
  Const vp = a \ PAGE_SIZE
  Local pp = vp_to_pp(vp)
  If pp = 0 Then pp = mem_load(vp) : Inc pf
  rb = Peek(Var m(0), pp * PAGE_SIZE + (a Mod PAGE_SIZE))
End Function

' Reads a 16-bit word from 'a' but DOES NOT increment a
Function rw(a)
  rw = rb(a) * 256 + rb(a + 1)
End Function

' Writes byte 'x' to 'a'
Sub wb(a, x)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= BASE_STATIC Then Error "Invalid address: " + fmt_hex$(a)
  If (x And &hFF) <> x Then Error "Invalid write value: " + Str$(x)
'!endif
  Poke Var m(0), a, x
End Sub

' Writes 16-bit word 'x' to 'a'
Sub ww(a, x)
'!if !defined(NO_EXTRA_CHECKS)
  If a < 0 Or a >= BASE_STATIC - 1 Then Error "Invalid address: " + fmt_hex$(a)
  If (x And &hFFFF) <> x Then Error "Invalid write value: " + Str$(x)
'!endif
  Poke Var m(0), a, x \ 256
  Poke Var m(0), a + 1, x Mod 256
End Sub

' Loads virtual page 'vp' from '$file'.
' @return physical page number
Function mem_load(vp)
  Local buf$, i
  Local pp = next_page

  ' TODO: Implement some form of Least Recently Used algorithm.
  Inc next_page
  If next_page = NUM_PHYSICAL_PAGES Then next_page = FIRST_SWAP_PAGE

  ' TODO: Should the file be opened once globally and kept open until exit?
  Open MEM_FILE$ For random As #1
  Seek #1, vp * PAGE_SIZE + 1
  Local ad = pp * PAGE_SIZE
  Local to_read = PAGE_SIZE
  Local buf_sz = 255
  Do While to_read > 0
    If to_read < 255 Then buf_sz = to_read
    buf$ = Input$(buf_sz, 1)
    For i = 1 To buf_sz
      Poke Var m(0), ad, Peek(Var buf$, i)
      Inc ad
    Next
    Inc to_read,  -buf_sz
  Loop
  Close #1

  vp_to_pp(pp_to_vp(pp)) = 0
  vp_to_pp(vp) = pp
  pp_to_vp(pp) = vp

  mem_load = pp
End Function

Sub mem_init(f$)
  Local i

  MEM_FILE$ = f$

  con.println("Loading '" + MEM_FILE$ + "'")

  ' Load page 0 which contains the header
  con.println("  Header page: 0")
  next_page = 0
  If mem_load(0) <> 0 Then Error

  ' Read memory sizes from header.
  ' Note: we can't call rw() until both of these are initialised.
  BASE_STATIC = Peek(Var m(0),&h0E)*256 + Peek(Var m(0),&h0F)
  FILE_LEN = (Peek(Var m(0),&h1A) * 256 + Peek(Var m(0),&h1B)) * 2

  ' Initialise dynamic memory
  FIRST_SWAP_PAGE = BASE_STATIC \ PAGE_SIZE
  If BASE_STATIC Mod PAGE_SIZE > 0 Then FIRST_SWAP_PAGE = FIRST_SWAP_PAGE + 1
  con.print("  Dynamic pages: ")
  For i = 1 To FIRST_SWAP_PAGE - 1
    If i > 1 Then con.print(", ")
    con.print(Str$(i))
    If mem_load(i) <> i Then Error
  Next
  con.endl()
  con.println("  Paged memory starts at page " + Str$(FIRST_SWAP_PAGE))
End Sub
