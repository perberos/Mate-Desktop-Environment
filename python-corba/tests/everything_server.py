import sys
import MateCORBA

MateCORBA.load_typelib('Everything')

import CORBA
import matecorba.test
import matecorba__POA.test
import constants

class MyBasicServer(matecorba__POA.test.BasicServer):
    def _get_foo(self):
        return constants.STRING_RETN
    def _set_foo(self, value):
        assert value == constants.STRING_IN
    def _get_bah(self):
        return constants.LONG_RETN

    def opString(self, inArg, inoutArg):
        assert inArg == constants.STRING_IN
        assert inoutArg == constants.STRING_INOUT_IN
        return (constants.STRING_RETN,
                constants.STRING_INOUT_OUT,
                constants.STRING_OUT)

    def opLong(self, inArg, inoutArg):
        assert inArg == constants.LONG_IN
        assert inoutArg == constants.LONG_INOUT_IN
        return (constants.LONG_RETN,
                constants.LONG_INOUT_OUT,
                constants.LONG_OUT)

    def opLongLong(self, inArg, inoutArg):
        assert inArg == constants.LONG_LONG_IN
        assert inoutArg == constants.LONG_LONG_INOUT_IN
        return (constants.LONG_LONG_RETN,
                constants.LONG_LONG_INOUT_OUT,
                constants.LONG_LONG_OUT)

    def opFloat(self, inArg, inoutArg):
        assert abs(inArg - constants.FLOAT_IN) < 0.00005
        assert abs(inoutArg - constants.FLOAT_INOUT_IN) < 0.00005
        return (constants.FLOAT_RETN,
                constants.FLOAT_INOUT_OUT,
                constants.FLOAT_OUT)

    def opDouble(self, inArg, inoutArg):
        assert inArg == constants.FLOAT_IN
        assert inoutArg == constants.FLOAT_INOUT_IN
        return (constants.FLOAT_RETN,
                constants.FLOAT_INOUT_OUT,
                constants.FLOAT_OUT)

    def opEnum(self, inArg, inoutArg):
        assert inArg == matecorba.test.ENUM_IN
        assert inoutArg == matecorba.test.ENUM_INOUT_IN
        return (matecorba.test.ENUM_RETN,
                matecorba.test.ENUM_INOUT_OUT,
                matecorba.test.ENUM_OUT)

    def opException(self):
        raise matecorba.test.TestException(constants.STRING_IN,
                                       constants.LONG_IN,
                                       [ constants.LONG_IN ],
                                       self._this())

    def opOneWay(self, inArg):
        assert inArg == constants.STRING_IN

class MyStructServer(matecorba__POA.test.StructServer, MyBasicServer):
    def opFixed(self, inArg, inoutArg):
        assert inArg.a == constants.SHORT_IN
        assert inoutArg.a == constants.SHORT_INOUT_IN
        return (matecorba.test.FixedLengthStruct(constants.SHORT_RETN),
                matecorba.test.FixedLengthStruct(constants.SHORT_INOUT_OUT),
                matecorba.test.FixedLengthStruct(constants.SHORT_OUT))

    def opVariable(self, inArg, inoutArg):
        assert inArg.a == constants.STRING_IN
        assert inoutArg.a == constants.STRING_INOUT_IN
        return (matecorba.test.FixedLengthStruct(constants.STRING_RETN),
                matecorba.test.FixedLengthStruct(constants.STRING_INOUT_OUT),
                matecorba.test.FixedLengthStruct(constants.STRING_OUT))
    
    def opCompound(self, inArg, inoutArg):
        assert inArg.a.a == constants.STRING_IN
        assert inoutArg.a.a == constants.STRING_INOUT_IN
        return (matecorba.test.CompoundStruct(matecorba.test.FixedLengthStruct(constants.STRING_RETN)),
                matecorba.test.CompoundStruct(matecorba.test.FixedLengthStruct(constants.STRING_INOUT_OUT)),
                matecorba.test.CompoundStruct(matecorba.test.FixedLengthStruct(constants.STRING_OUT)))

    def opObjectStruct(self, inArg):
        assert inArg.serv == self._this()

    def opStructAny(self):
        tc = CORBA.TypeCode('IDL:omg.org/CORBA/long:1.0')
        return matecorba.test.StructAny(constants.STRING_IN,
                                    CORBA.Any(tc, constants.LONG_IN))

class MySequenceServer(matecorba__POA.test.SequenceServer):
    def opStrSeq(self, inArg, inoutArg):
        for i in range(len(inArg)):
            assert inArg[i] == constants.SEQ_STRING_IN[i]
        for i in range(len(inoutArg)):
            assert inoutArg[i] == constants.SEQ_STRING_INOUT_IN[i]
        return (constants.SEQ_STRING_RETN,
                constants.SEQ_STRING_INOUT_OUT,
                constants.SEQ_STRING_OUT)
    def opBoundedStructSeq(self, inArg, inoutArg):
        for i in range(len(inArg)):
            assert inArg[i].a.a == constants.SEQ_STRING_IN[i]
        for i in range(len(inoutArg)):
            assert inoutArg[i].a.a == constants.SEQ_STRING_INOUT_IN[i]
        retn = [
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_RETN[0])),
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_RETN[1]))
            ]
        inout = [
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_INOUT_OUT[0])),
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_INOUT_OUT[1]))
            ]
        out = [
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_OUT[0])),
            matecorba.test.CompoundStruct(matecorba.test.VariableLengthStruct(constants.SEQ_STRING_OUT[1]))
            ]
        return retn, inout, out

class MyUnionServer(matecorba__POA.test.UnionServer):
    def opFixed(self, inArg, inoutArg):
        assert inArg.x == constants.LONG_IN
        assert inoutArg.y == 't'
        return (matecorba.test.FixedLengthUnion('d', CORBA.FALSE),
                matecorba.test.FixedLengthUnion('c', CORBA.TRUE),
                matecorba.test.FixedLengthUnion('a', constants.LONG_OUT))
    def opVariable(self, inArg, inoutArg):
        assert inArg.x == constants.LONG_IN
        assert inoutArg.y == constants.STRING_INOUT_IN
        return (matecorba.test.VariableLengthUnion(4, CORBA.FALSE),
                matecorba.test.VariableLengthUnion(3, CORBA.TRUE),
                matecorba.test.VariableLengthUnion(1, constants.LONG_OUT))
    def opMisc(self, inSeq, inArg):
        assert len(inSeq) == 3
        assert inSeq[0]._d == 4
        assert inSeq[0]._v == CORBA.TRUE
        assert inSeq[1].y == "blah"
        assert inSeq[2]._d == 55
        assert inSeq[2].w == constants.LONG_IN
        assert inArg.y == "blah de blah"
        return (matecorba.test.EnumUnion(matecorba.test.EnumUnion.red,
                                     constants.LONG_IN),
                matecorba.test.ArrayUnion(22,
                                      [ 'Numero %d' % i for i in range(20) ]))

class MyArrayServer(matecorba__POA.test.ArrayServer):
    def opLongArray(self, inArg, inoutArg):
        for i in range(len(inArg)):
            assert inArg[i] == constants.SEQ_LONG_IN[i]
        for i in range(len(inoutArg)):
            assert inoutArg[i] == constants.SEQ_LONG_INOUT_IN[i]
        return (constants.SEQ_LONG_RETN,
                constants.SEQ_LONG_INOUT_OUT,
                constants.SEQ_LONG_OUT)

    def opStrArray(self, inArg, inoutArg):
        for i in range(len(inArg)):
            assert inArg[i] == constants.SEQ_STRING_IN[i]
        for i in range(len(inoutArg)):
            assert inoutArg[i] == constants.SEQ_STRING_INOUT_IN[i]
        return (constants.SEQ_STRING_RETN,
                constants.SEQ_STRING_INOUT_OUT,
                constants.SEQ_STRING_OUT)

class MyAnyServer(matecorba__POA.test.AnyServer):
    def opAnyStrSeq(self):
        tc = CORBA.TypeCode('IDL:matecorba/test/StrSeq:1.0')
        return CORBA.Any(tc, [ 'foo' ] * 16)

    def opAnyLong(self, inArg, inoutArg):
        tc = CORBA.TypeCode('IDL:omg.org/CORBA/long:1.0')
        assert inArg.typecode() == tc
        assert inArg.value() == constants.LONG_IN
        assert inoutArg.typecode() == tc
        assert inoutArg.value() == constants.LONG_INOUT_IN
        return (CORBA.Any(tc, constants.LONG_RETN),
                CORBA.Any(tc, constants.LONG_INOUT_OUT),
                CORBA.Any(tc, constants.LONG_OUT))

    def opAnyString(self, inArg, inoutArg):
        tc = CORBA.TypeCode('IDL:omg.org/CORBA/string:1.0')
        assert inArg.typecode() == tc
        assert inArg.value() == constants.STRING_IN
        assert inoutArg.typecode() == tc
        assert inoutArg.value() == constants.STRING_INOUT_IN
        return (CORBA.Any(tc, constants.STRING_RETN),
                CORBA.Any(tc, constants.STRING_INOUT_OUT),
                CORBA.Any(tc, constants.STRING_OUT))

    def opAnyStruct(self, inArg, inoutArg):
        tc = CORBA.TypeCode('IDL:matecorba/test/VariableLengthStruct:1.0')
        assert inArg.typecode() == tc
        assert inArg.value().a == constants.STRING_IN
        assert inoutArg.typecode() == tc
        assert inoutArg.value().a == constants.STRING_INOUT_IN
        return (CORBA.Any(tc, matecorba.test.VariableLengthStruct(constants.STRING_RETN)),
                CORBA.Any(tc, matecorba.test.VariableLengthStruct(constants.STRING_INOUT_OUT)),
                CORBA.Any(tc, matecorba.test.VariableLengthStruct(constants.STRING_OUT)))

    def opTypeCode(self, inArg, inoutArg):
        assert inArg == CORBA.TypeCode('IDL:matecorba/test/ArrayUnion:1.0')
        assert inoutArg == CORBA.TypeCode('IDL:matecorba/test/AnyServer:1.0')
        return (CORBA.TypeCode('IDL:matecorba/test/VariableLengthStruct:1.0'),
                CORBA.TypeCode('IDL:matecorba/test/TestException:1.0'),
                CORBA.TypeCode('IDL:matecorba/test/AnEnum:1.0'))


class MyFactory(matecorba__POA.test.TestFactory):
    def __init__(self):
        matecorba__POA.test.TestFactory.__init__(self)
        self.basicServer = MyBasicServer()
        self.structServer = MyStructServer()
        self.sequenceServer = MySequenceServer()
        self.unionServer = MyUnionServer()
        self.arrayServer = MyArrayServer()
        self.anyServer = MyAnyServer()
    def getBasicServer(self):
        return self.basicServer._this()
    def getStructServer(self):
        return self.structServer._this()
    def getSequenceServer(self):
        return self.sequenceServer._this()
    def getUnionServer(self):
        return self.unionServer._this()
    def getArrayServer(self):
        return self.arrayServer._this()
    def getAnyServer(self):
        return self.anyServer._this()

if __name__ == '__main__':
    orb = CORBA.ORB_init(sys.argv)

    factory = MyFactory()
    objref = factory._this()
    file('iorfile', 'w').write(orb.object_to_string(objref))

    factory._default_POA().the_POAManager.activate()
    orb.run()
