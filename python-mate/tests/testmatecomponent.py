import unittest
import pygtk; pygtk.require("2.0")
import gobject
import MateCORBA
from common import matecomponent
import CORBA
import MateComponent

class MateComponentTest(unittest.TestCase):

    def testEventSource(self):
        """tests matecomponent.EventSource and listener helper"""
        es = matecomponent.EventSource()
        self.event_received = False

        def event_cb(listener, event, value):
            self.assertEqual(event, "test")
            self.assertEqual(value.value(), "xxx")
            self.event_received = True

        def fire_event():
            es.notify_listeners("test", CORBA.Any(CORBA.TC_string, "xxx"))
            matecomponent.main_quit()

        matecomponent.event_source_client_add_listener(es.corba_objref(), event_cb, "test")
        gobject.idle_add(fire_event)
        matecomponent.main()
        self.assert_(self.event_received)

    def testClosure(self):
        """tests pymatecomponent_closure_new"""
        def enum_objects_cb(handler):
            return ()

        def get_object_cb(handler, item_name, only_if_exists):
            raise MateComponent.ItemContainer.NotFound

        matecomponent.activate()
        item_handler = matecomponent.ItemHandler(enum_objects_cb, get_object_cb)
        self.assertRaises(MateComponent.ItemContainer.NotFound,
                          item_handler.corba_objref().getObjectByName,
                          "ObjectThatDoesNotExist", False)

if __name__ == '__main__':
    unittest.main()
