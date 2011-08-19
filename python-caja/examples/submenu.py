import caja

class ExampleMenuProvider(caja.MenuProvider):
    
    # Caja crashes if a plugin doesn't implement the __init__ method.
    # See Bug #374958
    def __init__(self):
        pass
        
    def get_file_items(self, window, files):
        top_menuitem = caja.MenuItem('ExampleMenuProvider::Foo', 'Foo', '')

        submenu = caja.Menu()
        top_menuitem.set_submenu(submenu)

        sub_menuitem = caja.MenuItem('ExampleMenuProvider::Bar', 'Bar', '')
        submenu.append_item(sub_menuitem)

        return top_menuitem,

    def get_background_items(self, window, file):
        submenu = caja.Menu()
        submenu.append_item(caja.MenuItem('ExampleMenuProvider::Bar', 'Bar', ''))

        menuitem = caja.MenuItem('ExampleMenuProvider::Foo', 'Foo', '')
        menuitem.set_submenu(submenu)

        return menuitem,

