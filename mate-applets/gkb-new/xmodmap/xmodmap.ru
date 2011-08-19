! Converted keytable file to xmodmap file
! with mk_modmap by root@linux.chanae.stben.be Thu Jun 12 00:28:41 MET DST 1997
clear Mod1
clear Mod2
clear Lock
clear Control
! Russian Cyrillic keyboard.map. "Cyrillic" mode is toggled by 
! Right_Ctrl key and shifted by AltGr key.
!
! This file uses the X11 keysysms for cyrillic letters
!
keycode   9 = Escape Escape Escape Escape
keycode  10 = 1 exclam 1 exclam
keycode  11 = 2 at 2 quotedbl
keycode  12 = 3 numbersign 3 question
keycode  13 = 4 dollar 4 semicolon
keycode  14 = 5 percent 5 colon
keycode  15 = 6 asciicircum 6 comma
keycode  16 = 7 ampersand 7 period
keycode  17 = 8 asterisk 8 asterisk
keycode  18 = 9 parenleft 9 parenleft
keycode  19 = 0 parenright 0 parenright
keycode  20 = minus underscore minus underscore
keycode  21 = equal plus equal plus
keycode  22 = BackSpace BackSpace BackSpace BackSpace
keycode  23 = Tab Tab Tab Tab
keycode  24 = q Q Cyrillic_shorti Cyrillic_SHORTI
keycode  25 = w W Cyrillic_tse Cyrillic_TSE
keycode  26 = e E Cyrillic_u Cyrillic_U
keycode  27 = r R Cyrillic_ka Cyrillic_KA
keycode  28 = t T Cyrillic_ie Cyrillic_IE
keycode  29 = y Y Cyrillic_en Cyrillic_EN
keycode  30 = u U Cyrillic_ghe Cyrillic_GHE
keycode  31 = i I Cyrillic_sha Cyrillic_SHA
keycode  32 = o O Cyrillic_shcha Cyrillic_SHCHA
keycode  33 = p P Cyrillic_ze Cyrillic_ZE
keycode  34 = bracketleft braceleft Cyrillic_ha Cyrillic_HA
keycode  35 = bracketright braceright Cyrillic_hardsign Cyrillic_HARDSIGN
keycode  36 = Return
keycode  37 = Control_L
keycode  38 = a A Cyrillic_ef Cyrillic_EF
keycode  39 = s S Cyrillic_yeru Cyrillic_YERU
keycode  40 = d D Cyrillic_ve Cyrillic_VE
keycode  41 = f F Cyrillic_a Cyrillic_A
keycode  42 = g G Cyrillic_pe Cyrillic_PE
keycode  43 = h H Cyrillic_er Cyrillic_ER
keycode  44 = j J Cyrillic_o Cyrillic_O
keycode  45 = k K Cyrillic_el Cyrillic_EL
keycode  46 = l L Cyrillic_de Cyrillic_DE
keycode  47 = semicolon colon Cyrillic_zhe Cyrillic_ZHE
keycode  48 = apostrophe quotedbl Cyrillic_e Cyrillic_E
keycode  49 = grave asciitilde Cyrillic_io Cyrillic_IO
keycode  50 = Shift_L
keycode  51 = backslash bar slash bar
keycode  52 = z Z Cyrillic_ya Cyrillic_YA
keycode  53 = x X Cyrillic_che Cyrillic_CHE
keycode  54 = c C Cyrillic_es Cyrillic_ES
keycode  55 = v V Cyrillic_em Cyrillic_EM
keycode  56 = b B Cyrillic_i Cyrillic_I
keycode  57 = n N Cyrillic_te Cyrillic_TE
keycode  58 = m M Cyrillic_softsign Cyrillic_SOFTSIGN
keycode  59 = comma less Cyrillic_be Cyrillic_BE
keycode  60 = period greater Cyrillic_yu Cyrillic_YU
keycode  61 = slash question period comma
keycode  62 = Shift_R
keycode  63 = KP_Multiply
keycode  64 = Alt_L Meta_L
keycode  65 = space space space space
keycode  66 = Caps_Lock
keycode  67 = F1 F11 F1 F11
keycode  68 = F2 F12 F2 F12
keycode  69 = F3 F13 F3 F13
keycode  70 = F4 F14 F4 F14
keycode  71 = F5 F15 F5 F15
keycode  72 = F6 F16 F6 F16
keycode  73 = F7 F17 F7 F17
keycode  74 = F8 F18 F8 F18
keycode  75 = F9 F19 F9 F19
keycode  76 = F10 F20 F10 F20
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
keycode  94 = less greater bar brokenbar
keycode  95 = F11 F11 F11 F11
keycode  96 = F12 F12 F12 F12
keycode 108 = KP_Enter
keycode 109 = Control_R
keycode 112 = KP_Divide
keycode 113 = Mode_switch
keycode 114 = Break
keycode 110 = Find
keycode  98 = Up
keycode  99 = Prior
keycode 100 = Left
keycode 102 = Right
keycode 104 = Down
keycode 105 = Next
keycode 106 = Insert
! right windows-logo key
! in "windows" keyboards the postion of the key is annoying, is where AltGr
! usually resides, so go definie it as AltGr
keycode 116 = Mode_switch
! right windows-menu key
keycode 117 = Multi_key
!
add Mod1 = Alt_L
add Mod2 = Mode_switch Control_R
add Lock = Control_R
add Control = Control_L
