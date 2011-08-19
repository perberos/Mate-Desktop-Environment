#ifndef _MATECORBA_INIT_H_
#define _MATECORBA_INIT_H_

DynamicAny_DynAnyFactory
     MateCORBA_DynAnyFactory_new (CORBA_ORB orb, CORBA_Environment *ev);
void MateCORBA_init_internals    (CORBA_ORB orb, CORBA_Environment *ev);

#endif /* _MATECORBA_INIT_H */
