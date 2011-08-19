# -*- python -*-

import Options

import Configure
Configure.autoconfig = True

import ccroot
ccroot.USE_TOP_LEVEL = True

import Task
Task.file_deps = Task.extract_deps

import Build

VERSION = '2.28.1'
APPNAME = 'mate-python'
srcdir = '.'
blddir = 'build'

import misc
import os
import shutil
import glob
import sys
import types

def dist_hook():
    for html_file in glob.glob(os.path.join('..', 'build', 'default', 'docs', 'matevfs', 'html', '*')):
        shutil.copy2(html_file, os.path.join('docs', 'matevfs', 'html'))
    ## Copy WAF to the distdir
    assert os.path.basename(sys.argv[0]) == 'waf'
    shutil.copy(os.path.join('..', sys.argv[0]), '.')

    subprocess.Popen([os.path.join(srcdir, "generate-ChangeLog")],  shell=True).wait()
    try:
        os.chmod(os.path.join(srcdir, "ChangeLog"), 0644)
    except OSError:
        pass
    shutil.copy(os.path.join(srcdir, "ChangeLog"), '.')


def set_options(opt):
    opt.tool_options('compiler_cc')
    opt.tool_options('mate')
    opt.tool_options('gnu_dirs')
    opt.tool_options('cflags')

    opt.add_option('--enable-modules',
                   help=('Enable only the specified modules.'),
                   type="string", default='all', metavar="MODULES_LIST (comma separated)",
                   dest='enable_modules')


def configure(conf):
    conf.check_tool('misc')
    conf.check_tool('compiler_cc')
    conf.check_tool('mate')
    conf.check_tool('python')
    conf.check_tool('command')
    conf.check_tool('pkgconfig')
    conf.check_tool('gnu_dirs')
    conf.check_tool('cflags')

    if sys.platform == 'darwin':
        conf.check_tool('osx')

    conf.check_python_version((2,4))
    conf.check_python_headers()
    conf.define('VERSION', VERSION)

    version = [int(s) for s in VERSION.split('.')]
    conf.define('MATE_PYTHON_MAJOR_VERSION', version[0])
    conf.define('MATE_PYTHON_MINOR_VERSION', version[1])
    conf.define('MATE_PYTHON_MICRO_VERSION', version[2])

    # Define pygtk required version, for runtime check
    pygtk_version = [2, 10, 3]
    conf.define('PYGTK_REQUIRED_MAJOR_VERSION', pygtk_version[0])
    conf.define('PYGTK_REQUIRED_MINOR_VERSION', pygtk_version[1])
    conf.define('PYGTK_REQUIRED_MICRO_VERSION', pygtk_version[2])

    conf.pkg_check_modules('PYGTK', 'pygtk-2.0 >= %s' % ('.'.join([str(x) for x in pygtk_version]),))
    conf.env['PYGTK_DEFSDIR'] = conf.pkg_check_module_variable('pygtk-2.0', 'defsdir')

    conf.pkg_check_modules('PYGOBJECT', 'pygobject-2.0 >= 2.17.0', mandatory=True)

    if not conf.find_program('pygobject-codegen-2.0', var='CODEGEN'):
        if not conf.find_program('pygtk-codegen-2.0', var='CODEGEN'):
            Params.fatal("Could not find pygobject/pygtk codegen")

    conf.env.append_value('CCDEFINES', 'HAVE_CONFIG_H')

    conf.env['ENABLE_MODULES'] = Options.options.enable_modules.split(',')
    
    conf.sub_config('matecomponent')
    conf.sub_config('mate')
    conf.sub_config('matecanvas')
    conf.sub_config('mateconf')
    conf.sub_config('matevfs')
    conf.sub_config('docs/matevfs')

    for module in conf.env['ENABLE_MODULES']:
        if module == 'all':
            continue
        if module not in conf.env['MODULES_AVAILABLE']:
            Logs.warn("Requested module %r is not available in this source tree." % module)
    if conf.env['MODULES_TO_BUILD']:
        print "** The following modules will be built:"
        for m in conf.env['MODULES_TO_BUILD']:
            print "\t%s" % m

    not_built = list(conf.env['MODULES_AVAILABLE'])
    for mod in conf.env['MODULES_TO_BUILD']:
        not_built.remove(mod)
    if not_built:
        print "** The following modules will NOT be built:"
        for m in not_built:
            print "\t%s" % m

    conf.write_config_header('config.h')
    
    for docs_module in ['matevfs']:
        d = os.path.join('docs', docs_module, 'html')
        try:
            os.mkdir(d)
        except OSError:
            pass
        #    print "* Directory %r already exists." % (d,)
        #else:
        #    print "* Creating directory %r." % (d,)


def codegen(bld, module, local_load_types=(), register=(), local_register=(), prefix=None):
    cmd = bld.new_task_gen('command',
                           source=['%s.defs' % module, '%s.override' % module],
                           target=['%s.c' % module])
    cmd.command = ['${CODEGEN}']
    cmd.command.append('--py_ssize_t-clean')
    
    for load in local_load_types:
        cmd.source.append(load)
        cmd.command.extend(['--load-types', '${SRC[%i]}' % (len(cmd.source)-1)])
    
    for reg in local_register:
        cmd.source.append(reg)
        cmd.command.extend(['--register', '${SRC[%i]}' % (len(cmd.source)-1)])

    for reg in register:
        cmd.command.extend(['--register', reg])

    if prefix:
        cmd.command.extend(['--prefix', prefix])
    else:
        cmd.command.extend(['--prefix', 'py'+module])

    cmd.command.extend(['--override', "${SRC[1]}",
                        '${SRC[0]}',
                        '>', '${TGT[0]}'])

    return cmd


def build(bld):

    # Attach the 'codegen' method to the build context
    bld.codegen = types.MethodType(codegen, bld)

    def create_pyext(bld):
        return bld.new_task_gen('cc', 'shlib', 'pyext')
    bld.create_pyext = types.MethodType(create_pyext, bld)

    ## generate and install the .pc file
    obj = bld.new_task_gen('subst')
    obj.source = 'mate-python-2.0.pc.in'
    obj.target = 'mate-python-2.0.pc'
    obj.dict = {
        'VERSION': VERSION,
        'prefix': bld.env['PREFIX'],
        'exec_prefix': bld.env['PREFIX'],
        'libdir': bld.env['LIBDIR'],
        'includedir': os.path.join(bld.env['PREFIX'], 'include'),
        'datadir': bld.env['DATADIR'],
        'datarootdir': bld.env['DATADIR'],
        }
    obj.fun = misc.subst_func
    obj.install_path = '${LIBDIR}/pkgconfig'

    ## subdirs
    bld.add_subdirs('matecomponent')
    bld.add_subdirs('mate')
    bld.add_subdirs('matecanvas')
    bld.add_subdirs('mateconf')
    bld.add_subdirs('matevfs')
    bld.add_subdirs('docs/matevfs')


def shutdown():
    env = Build.bld.env
    if Options.commands['check']:
        _run_tests(env)


def _run_tests(env):
    import pproc as subprocess
    import shutil
    builddir = os.path.join(blddir, env.variant())
    # copy the __init__.py files
    for subdir in ["matecomponent", "mate", "matevfs"]:
        src = os.path.join(subdir, "__init__.py")
        dst = os.path.join(builddir, subdir)
        shutil.copy(src, dst)
    os_env = dict(os.environ)
    if 'PYTHONPATH' in os_env:
        os_env['PYTHONPATH'] = os.pathsep.join([builddir, os_env['PYTHONPATH']])
    else:
        os_env['PYTHONPATH'] = builddir
    cmd = [#'gdb', '--args',
           env['PYTHON'], os.path.join('tests', 'runtests.py'), builddir]
    retval = subprocess.Popen(cmd, env=os_env).wait()
    if retval:
        sys.exit(retval)
