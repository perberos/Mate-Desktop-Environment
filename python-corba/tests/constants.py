'''constants from MateCORBA2/test/everything/constants.h.'''

STRING_IN        = 'In string'
STRING_INOUT_IN  = 'Inout in string'
STRING_INOUT_OUT = 'Inout out string'
STRING_OUT       = 'Out string'
STRING_RETN      = 'Retn String'

LONG_IN          = 0x12345678
LONG_INOUT_IN    = 0x34567812
LONG_INOUT_OUT   = 0x56781234
LONG_OUT         = 0x78123456
LONG_RETN        = -1430532899 # 0xAABBCCDD

LONG_LONG_IN          = 0x12345678L
LONG_LONG_INOUT_IN    = 0x34567812L
LONG_LONG_INOUT_OUT   = 0x56781234L
LONG_LONG_OUT         = 0x78123456L
LONG_LONG_RETN        = 0xAABBCCDDL

FLOAT_IN         = 127.13534
FLOAT_INOUT_IN   = 124.89432
FLOAT_INOUT_OUT  = 975.12694
FLOAT_OUT        = 112.54575
FLOAT_RETN       = 354.23535

SHORT_IN         = 0x1234
SHORT_INOUT_IN   = 0x3456
SHORT_INOUT_OUT  = 0x5678
SHORT_OUT        = 0x7812
SHORT_RETN       = -21829 # 0xAABB

SEQ_STRING_IN        = [ 'in1', 'in2', 'in3', 'in4' ]
SEQ_STRING_INOUT_IN  = ['inout1', 'inout2', 'inout3', 'inout4' ]
SEQ_STRING_INOUT_OUT = [ 'inout21', 'inout22', 'inout23', 'inout24' ]
SEQ_STRING_OUT       = [ 'out1', 'out2', 'out3', 'out4' ]
SEQ_STRING_RETN      = [ 'retn1', 'retn2', 'retn3', 'retn4' ]

SEQ_LONG_IN          = [ LONG_IN, LONG_INOUT_IN, 15, 7 ]
SEQ_LONG_INOUT_IN    = [ LONG_INOUT_OUT, LONG_OUT, 7, 15 ]
SEQ_LONG_INOUT_OUT   = [ LONG_OUT, LONG_RETN, 8, 9 ]
SEQ_LONG_OUT         = [ LONG_INOUT_IN, LONG_INOUT_OUT, 15, 7 ]
SEQ_LONG_RETN        = [ LONG_RETN, LONG_IN, 2, 3 ]
