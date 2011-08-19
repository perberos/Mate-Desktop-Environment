! Israeli keyboard
! from description contributed by Tzafrir Cohen <see@techst02.technion.ac.il>
!
!
clear Mod1
clear Mod2
clear Lock
clear Control
!charset "iso-8859-8"
keycode   9 = Escape Escape Escape Escape
keycode  10 = 1 exclam 1 exclam
keycode  11 = 2 at 2 at
keycode  12 = 3 numbersign 3 numbersign
keycode  13 = 4 dollar 4 dollar
keycode  14 = 5 percent 5 percent
keycode  15 = 6 asciicircum 6 asciicircum
keycode  16 = 7 ampersand 7 ampersand
keycode  17 = 8 asterisk 8 asterisk
keycode  18 = 9 parenleft 9 parenleft
keycode  19 = 0 parenright 0 parenright
keycode  20 = minus underscore minus underscore
keycode  21 = equal plus
keycode  22 = BackSpace BackSpace BackSpace BackSpace
keycode  23 = Tab Tab Tab Tab
keycode  24 = q Q slash q 
keycode  25 = w W apostrophe w
keycode  26 = e E hebrew_qoph e
keycode  27 = r R hebrew_resh r
keycode  28 = t T hebrew_aleph t
keycode  29 = y Y hebrew_tet y
keycode  30 = u U hebrew_waw u
keycode  31 = i I hebrew_finalnun i
keycode  32 = o O hebrew_finalmem o
keycode  33 = p P hebrew_pe p
keycode  34 = bracketleft braceleft bracketleft braceleft
keycode  35 = bracketright braceright bracketright braceright
keycode  36 = Return
keycode  37 = Control_L
keycode  38 = a A hebrew_shin a
keycode  39 = s S hebrew_dalet s
keycode  40 = d D hebrew_gimel d
keycode  41 = f F hebrew_kaph f
keycode  42 = g G hebrew_ayin g
keycode  43 = h H hebrew_yod h
keycode  44 = j J hebrew_chet j
keycode  45 = k K hebrew_lamed k
keycode  46 = l L hebrew_finalkaph l
keycode  47 = semicolon colon hebrew_finalpe colon
keycode  48 = apostrophe quotedbl comma quotedbl
keycode  49 = grave asciitilde semicolon asciitilde
keycode  50 = Shift_L
keycode  51 = backslash bar backslash bar
keycode  52 = z Z hebrew_zain z
keycode  53 = x X hebrew_samech x
keycode  54 = c C hebrew_bet c
keycode  55 = v V hebrew_he v
keycode  56 = b B hebrew_nun b
keycode  57 = n N hebrew_mem n
keycode  58 = m M hebrew_zade m
keycode  59 = comma less hebrew_taw less
keycode  60 = period greater hebrew_finalzade greater
keycode  61 = slash question period question
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
keycode  94 = less greater less greater
keycode  95 = F11 F11 F11 F11
keycode  96 = F12 F12 F12 F12
keycode 108 = KP_Enter
keycode 112 = KP_Divide
keycode 113 = Mode_switch
keycode 114 = Break
keycode 110 = Find
keycode  98 = Up
keycode  99 = Prior
keycode 100 = Left
keycode 102 = Right
keycode 115 = Select
keycode 104 = Down
keycode 105 = Next
keycode 106 = Insert
! 
!  left windows key
!  right windows key
keycode 116 = Mode_switch
!  menu key (on windows keyboars)
keycode 117 = Multi_key
add Mod1 = Alt_L
add Mod2 = Mode_switch Control_R
add Lock = Control_R
add Control = Control_L
