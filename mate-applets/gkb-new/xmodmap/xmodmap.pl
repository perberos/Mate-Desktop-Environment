!
! This is an `xmodmap' input file
! for PC 101 key keyboard #2 (Linux/XFree86 US layout) keyboards.
! This file was automatically generated on Wed Nov  2 10:29:07 1994
! by Ryszard Mikke with XKeyCaps 2.11;
! Copyright 1991-1994 Jamie Zawinski <jwz@lucid.com>.
!
! The file used wrongly latin1 keysysms instead of latin2 ones !!
! I corrected on 1999-10-07  -- Pablo Saratxaga <pablo@mandrakesoft.com>
!
! This file makes the following changes:
!
! The "& 7" key generates 7, ampersand, and section
! The "E" key generates e, E, eogonek, and Eogonek
! The "O" key generates o, O, oacute, and Oacute
! The "A" key generates a, A, aogonek, and Aogonek
! The "S" key generates s, S, sacute, and Sacute
! The "L" key generates l, L, lstroke, and Lstroke
! The "Z" key generates z, Z, zabovedot, and Zabovedot
! The "X" key generates x, X, zacute, and Zacute
! The "C" key generates c, C, cacute, and Cacute
! The "N" key generates n, N, nacute, and Nacute
! The "AltGr" key generates Mode_switch

keycode 0x09 =	Escape
keycode 0x43 =	F1
keycode 0x44 =	F2
keycode 0x45 =	F3
keycode 0x46 =	F4
keycode 0x47 =	F5
keycode 0x48 =	F6
keycode 0x49 =	F7
keycode 0x4A =	F8
keycode 0x4B =	F9
keycode 0x4C =	F10
keycode 0x5F =	F11
keycode 0x60 =	F12
keycode 0x6F =	Print
keycode 0x4E =	Multi_key
keycode 0x6E =	Pause
keycode 0x31 =	grave		asciitilde
keycode 0x0A =	1		exclam
keycode 0x0B =	2		at
keycode 0x0C =	3		numbersign
keycode 0x0D =	4		dollar
keycode 0x0E =	5		percent
keycode 0x0F =	6		asciicircum
keycode 0x10 =	7		ampersand	section
keycode 0x11 =	8		asterisk
keycode 0x12 =	9		parenleft
keycode 0x13 =	0		parenright
keycode 0x14 =	minus		underscore
keycode 0x15 =	equal		plus
keycode 0x33 =	backslash	bar
keycode 0x16 =	BackSpace
keycode 0x6A =	Insert
keycode 0x61 =	Home
keycode 0x63 =	Prior
keycode 0x4D =	Num_Lock
keycode 0x70 =	KP_Divide
keycode 0x3F =	KP_Multiply
keycode 0x52 =	KP_Subtract
keycode 0x17 =	Tab
keycode 0x18 =	Q
keycode 0x19 =	W
keycode 0x1A =	e		E		eogonek		Eogonek
keycode 0x1B =	R
keycode 0x1C =	T
keycode 0x1D =	Y
keycode 0x1E =	U
keycode 0x1F =	I
keycode 0x20 =	o		O		oacute		Oacute
keycode 0x21 =	P
keycode 0x22 =	bracketleft	braceleft
keycode 0x23 =	bracketright	braceright
keycode 0x24 =	Return
keycode 0x6B =	Delete
keycode 0x67 =	End
keycode 0x69 =	Next
keycode 0x4F =	KP_7
keycode 0x50 =	KP_8
keycode 0x51 =	KP_9
keycode 0x56 =	KP_Add
keycode 0x42 =	Caps_Lock
keycode 0x26 =	a		A		aogonek		Aogonek
keycode 0x27 =	s		S		sacute		Sacute
keycode 0x28 =	D
keycode 0x29 =	F
keycode 0x2A =	G
keycode 0x2B =	H
keycode 0x2C =	J
keycode 0x2D =	K
keycode 0x2E =	l		L		lstroke		Lstroke
keycode 0x2F =	semicolon	colon
keycode 0x30 =	apostrophe	quotedbl
keycode 0x53 =	KP_4
keycode 0x54 =	KP_5
keycode 0x55 =	KP_6
keycode 0x32 =	Shift_L
keycode 0x34 =	z		Z		zabovedot	Zabovedot
keycode 0x35 =	x		X		zacute		Zacute
keycode 0x36 =	c		C		cacute		Cacute
keycode 0x37 =	V
keycode 0x38 =	B
keycode 0x39 =	n		N		nacute		Nacute
keycode 0x3A =	M
keycode 0x3B =	comma		less
keycode 0x3C =	period		greater		Multi_key 
keycode 0x3D =	slash		question
keycode 0x3E =	Shift_R
keycode 0x62 =	Up
keycode 0x57 =	KP_1
keycode 0x58 =	KP_2
keycode 0x59 =	KP_3
keycode 0x6C =	KP_Enter
keycode 0x25 =	Control_L
keycode 0x40 =	Alt_L		Meta_L
keycode 0x41 =	space
keycode 0x71 =	Mode_switch
keycode 0x6D =	Control_R
keycode 0x64 =	Left
keycode 0x68 =	Down
keycode 0x66 =	Right
keycode 0x5A =	KP_0
keycode 0x5B =	KP_Decimal
! right windows-logo key
! in "windows" keyboards the postion of the key is annoying, is where AltGr
! usually resides, so go definie it as AltGr
keycode 116 = Mode_switch
! right windows-menu key
keycode 117 = Multi_key

clear Shift
clear Lock
clear Control
clear Mod1
clear Mod2
clear Mod3
clear Mod4
clear Mod5

add    Shift   = Shift_L Shift_R
add    Lock    = Caps_Lock
add    Control = Control_L Control_R
add    Mod1    = Alt_L 
!Mode_switch
add    Mod2    = Mode_switch
