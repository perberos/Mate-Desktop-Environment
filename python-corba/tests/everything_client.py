#!/usr/bin/env python
'''This is a client to the test/everything/server test program
distributed with MateCORBA2.'''

import sys, os
import unittest
import MateCORBA

MateCORBA.load_typelib('Everything')

import CORBA
# this one is loaded from the typelib ...
from matecorba import test

import constants

# from test/everything/constants.h

class EverythingTestCase(unittest.TestCase):

    def test01_attribute(self):
        '''attributes'''
        objref = factory.getBasicServer()
        assert objref._is_a('IDL:matecorba/test/BasicServer:1.0')

        val = objref.foo
        assert val == constants.STRING_RETN

        objref.foo = constants.STRING_IN

        lval = objref.bah
        assert lval == constants.LONG_RETN

    def test02_string(self):
        '''strings'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opString(constants.STRING_IN,
                                           constants.STRING_INOUT_IN)
        assert retn == constants.STRING_RETN
        assert inout == constants.STRING_INOUT_OUT
        assert out == constants.STRING_OUT

        objref.opOneWay(constants.STRING_IN)

    def test03_long(self):
        '''longs'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opLong(constants.LONG_IN,
                                         constants.LONG_INOUT_IN)
        assert retn == constants.LONG_RETN
        assert inout == constants.LONG_INOUT_OUT
        assert out == constants.LONG_OUT

    def test04_longlong(self):
        '''long longs'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opLongLong(constants.LONG_LONG_IN,
                                             constants.LONG_LONG_INOUT_IN)
        assert retn == constants.LONG_LONG_RETN
        assert inout == constants.LONG_LONG_INOUT_OUT
        assert out == constants.LONG_LONG_OUT

    def test05_float(self):
        '''floats'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opFloat(constants.FLOAT_IN,
                                          constants.FLOAT_INOUT_IN)
        assert abs(retn - constants.FLOAT_RETN) < 0.00005
        assert abs(inout - constants.FLOAT_INOUT_OUT) < 0.00005
        assert abs(out - constants.FLOAT_OUT) < 0.00005

    def test06_double(self):
        '''doubles'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opDouble(constants.FLOAT_IN,
                                           constants.FLOAT_INOUT_IN)
        assert retn == constants.FLOAT_RETN
        assert inout == constants.FLOAT_INOUT_OUT
        assert out == constants.FLOAT_OUT

    def test07_enum(self):
        '''enums'''
        objref = factory.getBasicServer()

        retn, inout, out = objref.opEnum(test.ENUM_IN, test.ENUM_INOUT_IN)
        assert retn == test.ENUM_RETN
        assert inout == test.ENUM_INOUT_OUT
        assert out == test.ENUM_OUT

    def test08_exception(self):
        '''exceptions'''
        objref = factory.getBasicServer()

        got_exception = 0
        try:
            objref.opException()
        except test.TestException, e:
            assert e.reason == constants.STRING_IN
            assert e.number == constants.LONG_IN
            assert len(e.aseq) == 1
            assert e.aseq[0] == constants.LONG_IN
            got_exception = 1
        assert got_exception

    def test09_is_a(self):
        '''is_a'''
        assert factory._is_a('IDL:CORBA/Object:1.0')
        assert factory._is_a('IDL:omg.org/CORBA/Object:1.0')

    def test10_fixed_length_struct(self):
        '''fixed length structs'''
        objref = factory.getStructServer()

        inarg = test.FixedLengthStruct()
        inarg.a = constants.SHORT_IN
        inoutarg = test.FixedLengthStruct()
        inoutarg.a = constants.SHORT_INOUT_IN

        retn, inout, out = objref.opFixed(inarg, inoutarg)
        assert retn.a == constants.SHORT_RETN
        assert inout.a == constants.SHORT_INOUT_OUT
        assert out.a == constants.SHORT_OUT
    
    def test11_variable_length_struct(self):
        '''variable length structs'''
        objref = factory.getStructServer()

        inarg = test.VariableLengthStruct(constants.STRING_IN)
        inoutarg = test.VariableLengthStruct(constants.STRING_INOUT_IN)

        retn, inout, out = objref.opVariable(inarg, inoutarg)
        assert retn.a == constants.STRING_RETN
        assert inout.a == constants.STRING_INOUT_OUT
        assert out.a == constants.STRING_OUT
    
    def test12_compound_struct(self):
        '''compound structs'''
        objref = factory.getStructServer()

        inarg = test.CompoundStruct(test.VariableLengthStruct(constants.STRING_IN))
        inoutarg = test.CompoundStruct(test.VariableLengthStruct(constants.STRING_INOUT_IN))

        retn, inout, out = objref.opCompound(inarg, inoutarg)
        assert retn.a.a == constants.STRING_RETN
        assert inout.a.a == constants.STRING_INOUT_OUT
        assert out.a.a == constants.STRING_OUT

    def test13_object_struct(self):
        '''object structs'''
        objref = factory.getStructServer()

        inarg = test.ObjectStruct()
        inarg.serv = objref

        objref.opObjectStruct(inarg)

    def test14_struct_any(self):
        '''any structs'''
        objref = factory.getStructServer()

        a = objref.opStructAny()
        assert a.a == constants.STRING_IN
        assert a.b.value() == constants.LONG_IN

    def test15_unbounded_sequence(self):
        '''unbounded sequences'''
        objref = factory.getSequenceServer()

        retn, inout, out = objref.opStrSeq(constants.SEQ_STRING_IN,
                                           constants.SEQ_STRING_INOUT_IN)
        assert retn == constants.SEQ_STRING_RETN[:len(retn)]
        assert inout == constants.SEQ_STRING_INOUT_OUT[:len(inout)]
        assert out == constants.SEQ_STRING_OUT[:len(out)]

    def test16_bounded_sequence(self):
        '''bounded sequences'''
        objref = factory.getSequenceServer()

        inarg = [
            test.CompoundStruct(test.VariableLengthStruct(constants.SEQ_STRING_IN[0])),
            test.CompoundStruct(test.VariableLengthStruct(constants.SEQ_STRING_IN[1]))
            ]
        inoutarg = [
            test.CompoundStruct(test.VariableLengthStruct(constants.SEQ_STRING_INOUT_IN[0])),
            test.CompoundStruct(test.VariableLengthStruct(constants.SEQ_STRING_INOUT_IN[1]))
            ]

        retn, inout, out = objref.opBoundedStructSeq(inarg, inoutarg)
        for i in range(len(retn)):
            assert retn[i].a.a == constants.SEQ_STRING_RETN[i]
        for i in range(len(inout)):
            assert inout[i].a.a == constants.SEQ_STRING_INOUT_OUT[i]
        for i in range(len(out)):
            assert out[i].a.a == constants.SEQ_STRING_OUT[i]

    def test17_fixed_length_union(self):
        '''fixed length unions'''
        objref = factory.getUnionServer()

        inarg = test.FixedLengthUnion('a', constants.LONG_IN)
        inoutarg = test.FixedLengthUnion('b', 't')

        retn, inout, out = objref.opFixed(inarg, inoutarg)
        assert retn._d == 'd'
        assert retn._v == CORBA.FALSE
        assert inout._d == 'c'
        assert inout._v == CORBA.TRUE
        assert out._d == 'a'
        assert out._v == constants.LONG_OUT

    def test18_variable_length_union(self):
        '''variable length unions'''
        objref = factory.getUnionServer()

        inarg = test.VariableLengthUnion(1, constants.LONG_IN)
        inoutarg = test.VariableLengthUnion(2, constants.STRING_INOUT_IN)

        retn, inout, out = objref.opVariable(inarg, inoutarg)
        assert retn._d == 4
        assert retn._v == CORBA.FALSE
        assert inout._d == 3
        assert inout._v == CORBA.TRUE
        assert out._d == 1
        assert out._v == constants.LONG_OUT
    
    def test19_misc_union(self):
        '''misc type unions'''
        objref = factory.getUnionServer()

        inseq = [
            test.VariableLengthUnion(4, CORBA.TRUE),
            test.VariableLengthUnion(2, "blah"),
            test.VariableLengthUnion(55, constants.LONG_IN)
            ]
        inarg = test.BooleanUnion(1, "blah de blah")

        retn, out = objref.opMisc(inseq, inarg)

        assert retn._d == test.EnumUnion.red
        assert retn._v == constants.LONG_IN
        assert out._d == 22
        for i in range(20):
            assert out._v[i] == 'Numero %d' % i
    
    def test20_fixed_length_array(self):
        '''fixed length arrays'''
        objref = factory.getArrayServer()

        retn, inout, out = objref.opLongArray(constants.SEQ_LONG_IN,
                                              constants.SEQ_LONG_INOUT_IN)
        assert retn == constants.SEQ_LONG_RETN
        assert inout == constants.SEQ_LONG_INOUT_OUT
        assert out == constants.SEQ_LONG_OUT

    def test21_variable_length_array(self):
        '''variable length arrays'''
        objref = factory.getArrayServer()

        retn, inout, out = objref.opStrArray(constants.SEQ_STRING_IN,
                                             constants.SEQ_STRING_INOUT_IN)
        assert retn == constants.SEQ_STRING_RETN
        assert inout == constants.SEQ_STRING_INOUT_OUT
        assert out == constants.SEQ_STRING_OUT

    def test22_any_long(self):
        '''any with longs'''
        objref = factory.getAnyServer()

        tc = CORBA.TypeCode('IDL:omg.org/CORBA/long:1.0')
        retn, inout, out = objref.opAnyLong(CORBA.Any(tc, constants.LONG_IN),
                                            CORBA.Any(tc, constants.LONG_INOUT_IN))

        assert retn.typecode() == tc
        assert retn.value() == constants.LONG_RETN
        assert inout.typecode() == tc
        assert inout.value() == constants.LONG_INOUT_OUT
        assert out.typecode() == tc
        assert out.value() == constants.LONG_OUT

    def test23_any_string(self):
        '''any with strings'''
        objref = factory.getAnyServer()

        tc = CORBA.TypeCode('IDL:omg.org/CORBA/string:1.0')
        retn, inout, out = objref.opAnyString(CORBA.Any(tc, constants.STRING_IN),
                                              CORBA.Any(tc, constants.STRING_INOUT_IN))

        assert retn.typecode() == tc
        assert retn.value() == constants.STRING_RETN
        assert inout.typecode() == tc
        assert inout.value() == constants.STRING_INOUT_OUT
        assert out.typecode() == tc
        assert out.value() == constants.STRING_OUT

    def test24_any_str_seq(self):
        '''any with string sequences'''
        objref = factory.getAnyServer()

        retn = objref.opAnyStrSeq()

    def test25_any_struct(self):
        '''any with structs'''
        objref = factory.getAnyServer()

        tc = CORBA.TypeCode('IDL:matecorba/test/VariableLengthStruct:1.0')
        inarg = CORBA.Any(tc, test.VariableLengthStruct(constants.STRING_IN))
        inoutarg = CORBA.Any(tc, test.VariableLengthStruct(
            constants.STRING_INOUT_IN))

        retn, inout, out = objref.opAnyStruct(inarg, inoutarg)

        assert retn.typecode() == tc
        assert retn.value().a == constants.STRING_RETN
        assert inout.typecode() == tc
        assert inout.value().a == constants.STRING_INOUT_OUT
        assert out.typecode() == tc
        assert out.value().a == constants.STRING_OUT

    def test26_any_exception(self):
        '''any with exception'''

        tc = CORBA.TypeCode('IDL:matecorba/test/TestException:1.0')
        any = CORBA.Any(tc, test.TestException('reason', 42, [], factory))
        del any

    def test27_any_equivalence(self):
        '''anys equivalence'''

        tc = CORBA.TypeCode('IDL:matecorba/test/unionSeq:1.0')
        seq = [
            test.VariableLengthUnion(4, CORBA.TRUE),
            test.VariableLengthUnion(2, 'blah'),
            test.VariableLengthUnion(55, constants.LONG_IN)
            ]
        a = CORBA.Any(tc, seq)
        b = CORBA.Any(tc, seq)

        assert a == b

    def test28_typecode(self):
        '''TypeCodes'''
        objref = factory.getAnyServer()

        inarg = CORBA.TypeCode('IDL:matecorba/test/ArrayUnion:1.0')
        inoutarg = CORBA.TypeCode('IDL:matecorba/test/AnyServer:1.0')
        retn, inout, out = objref.opTypeCode(inarg, inoutarg)

        assert retn == CORBA.TypeCode('IDL:matecorba/test/VariableLengthStruct:1.0')
        assert inout == CORBA.TypeCode('IDL:matecorba/test/TestException:1.0')
        assert out == CORBA.TypeCode('IDL:matecorba/test/AnEnum:1.0')

if __name__ == '__main__':
    orb = CORBA.ORB_init(sys.argv, 'matecorba-local-orb')

    filename = os.environ.get('TEST_SERVER_IORFILE',
                              '../../../MateCORBA2/test/everything/iorfile')
    ior = file(filename, 'r').read()
    factory = orb.string_to_object(ior)
    del filename, ior

    unittest.main()
