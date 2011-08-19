! Converted keytable file to xmodmap file
! with mk_modmap by root@chanae.alphanet.ch mié jul 29 13:46:38 CEST 1998
clear Mod1
clear Mod2
! Spanish keymap, note the spanish IBM keyboard lacks an ascitilde (~), I
! have used ALT-Gr 4 as per IBM/AIX and some sun keyboards. ascitilde is also
! on ALT-Gr-exclamdown for compatibility with Julio Sanchez' Spanish keymap.
! 
! Jon Tombs <jon@gtex02.us.es> with corrections from 
! Julio Sanchez <jsanchez@gmv.es>
! otros toquecitos por mi <srtxg@f2219.n293.z2.fidonet.org>
!
keycode   8 =
keycode   9 = Escape Escape
keycode  10 = 1 exclam bar
keycode  11 = 2 quotedbl at
keycode  12 = 3 periodcentered numbersign
keycode  13 = 4 dollar asciitilde
keycode  14 = 5 percent
keycode  15 = 6 ampersand notsign
keycode  16 = 7 slash braceleft
keycode  17 = 8 parenleft bracketleft
keycode  18 = 9 parenright bracketright
keycode  19 = 0 equal braceright
keycode  20 = apostrophe question backslash
keycode  21 = exclamdown questiondown asciitilde
keycode  22 = BackSpace Delete
keycode  23 = Tab Tab
keycode  24 = q
keycode  25 = w
keycode  26 = e E currency
keycode  27 = r
keycode  28 = t
keycode  29 = y
keycode  30 = u
keycode  31 = i
keycode  32 = o
keycode  33 = p
keycode  34 = dead_acute dead_circumflex bracketleft
keycode  35 = plus asterisk bracketright
keycode  36 = Return
keycode  37 = Control_L
keycode  38 = a
keycode  39 = s
keycode  40 = d
keycode  41 = f
keycode  42 = g
keycode  43 = h
keycode  44 = j
keycode  45 = k
keycode  46 = l
keycode  47 = ntilde Ntilde
keycode  48 = dead_acute dead_diaeresis braceleft
keycode  49 = masculine ordfeminine backslash
keycode  50 = Shift_L
keycode  51 = ccedilla Ccedilla braceright
keycode  52 = z
keycode  53 = x
keycode  54 = c
keycode  55 = v
keycode  56 = b
keycode  57 = n
keycode  58 = m
keycode  59 = comma semicolon
keycode  60 = period colon Multi_key
keycode  61 = minus underscore
keycode  62 = Shift_R
keycode  63 = KP_Multiply
keycode  64 = Alt_L Meta_L
keycode  65 = space space
keycode  66 = Caps_Lock
keycode  67 = F1
keycode  68 = F2 
keycode  69 = F3
keycode  70 = F4
keycode  71 = F5
keycode  72 = F6
keycode  73 = F7
keycode  74 = F8
keycode  75 = F9
keycode  76 = F10
keycode  77 = Num_Lock
keycode  78 = Scroll_Lock
keycode  79 = KP_7
keycode  80 = KP_8
keycode  81 = KP_9
keycode  82 = KP_Subtract
keycode  83 = KP_4
keycode  84 = KP_5
keycode  85 = KP_6
keycode  86 = KP_Add
keycode  87 = KP_1
keycode  88 = KP_2
keycode  89 = KP_3
keycode  90 = KP_0
keycode  94 = less greater
keycode  95 = F11
keycode  96 = F12
keycode  98 = Up
keycode  99 = Prior
keycode 100 = Left
keycode 102 = Right
keycode 104 = Down
keycode 105 = Next
keycode 106 = Insert
keycode 108 = KP_Enter
keycode 109 = Control_R
keycode 110 = Find
keycode 112 = KP_Divide
keycode 113 = Mode_switch
keycode 114 = Break
keycode 115 = Select
! right windows-logo key
! in "windows" keyboards the postion of the key is annoying, is where AltGr
! usually resides, so go definie it as AltGr
keycode 116 = Mode_switch
! right windows-menu key
keycode 117 = Multi_key
add Mod1 = Alt_L
add Mod2 = Mode_switch
