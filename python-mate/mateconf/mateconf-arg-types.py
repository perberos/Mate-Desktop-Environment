import argtypes

class MateConfEngineArg (argtypes.ArgType):
    engine = ('    %(name)s = pymateconf_engine_from_pyobject (py_%(name)s);\n'
              '    if (PyErr_Occurred ())\n'
              '        return NULL;\n')
        
    def write_param(self, ptype, pname, pdflt, pnull, info):
        # pdflt and pnull not handled - we don't use "default" or "null-ok"
        info.varlist.add ('MateConfEngine*', pname)
        info.varlist.add ('PyObject', '*py_' + pname + ' = NULL')
        info.codebefore.append (self.engine % { 'name' : pname })
        info.arglist.append (pname)
        info.add_parselist ('O', ['&py_' + pname], [pname])
        
    def write_return (self, ptype, ownsreturn, info):
        if ptype[-1] == '*':
            ptype = ptype[:-1]
        info.varlist.add (ptype, '*ret')
        if ownsreturn:
            info.varlist.add ('PyObject', '*py_ret')
            info.codeafter.append ('    py_ret = pymateconf_engine_new (ret);\n'
                                   '    if (ret != NULL)\n'
                                   '        mateconf_engine_unref (ret);\n'
                                   '    return py_ret;')
        else:
            info.codeafter.append ('    /* pymateconf_engine_new() handles NULL checking */\n' +
                                   '    return pymateconf_engine_new (ret);')

argtypes.matcher.register ("MateConfEngine*", MateConfEngineArg ())
