# python port of canvas-curve.c from demos dir of libmatecanvas
# as such, under GPL
# FIXME: it seems to be somewhat broken...


import gtk
import gtk.gdk # event types
import matecanvas

STATE_INIT, STATE_FIRST_PRESS, STATE_FIRST_RELEASE, STATE_SECOND_PRESS = tuple(range(4))

class CurveExample:
    def __init__(self):
        self.width = 400
        self.height = 400
        #static State              current_state;
        self.current_state = STATE_INIT # initialized to 0?
        #static MateCanvasItem   *current_item;
        self.current_item = None
        #static MateCanvasPoints *current_points;
        self.current_points = [] # list of tuples of coordinates

        # Open window to hold canvas.
        win = gtk.Window()
        #win.connect('destroy', mainquit)
        win.set_title('Canvas Example')

        # Create VBox to hold canvas and buttons.
        vbox = gtk.VBox()
        win.add(vbox)
        vbox.show()

	# Some instructions for people using the example:
	label = gtk.Label("Drag - move object.\n" +
			 "Double click - change colour\n" +
			 "Right click - delete object")
	vbox.pack_start(label, expand=False)
	label.show()

        # Create canvas.
        self.acanvas = matecanvas.Canvas()
        self.acanvas.set_size_request(self.width, self.height)
        self.acanvas.set_scroll_region(0,0, self.width, self.height)
        vbox.pack_start(self.acanvas)
        self.acanvas.show()

        # FIXME: connect callback
        item = self.acanvas.root().add(matecanvas.CanvasRect,
                                outline_color= "black", fill_color= "white",
                                x1= 0.0, y1= 0.0, x2= self.width, y2= self.height)
        item.connect("event", self.canvas_event)

        win.show()
    def canvas_event(self, item, event, *args):# need re-check!!
        #print args
        #print item, event
        if event.type == gtk.gdk.BUTTON_PRESS:
            if event.button != 1:
                pass
            if self.current_state == STATE_INIT:
                self.draw_curve(item, event.x, event.y)
                self.current_state = STATE_FIRST_PRESS
            elif self.current_state == STATE_FIRST_RELEASE:
                self.draw_curve(item, event.x, event.y)
                self.current_state = STATE_SECOND_PRESS
            elif self.current_state == STATE_SECOND_PRESS:
                self.draw_curve(item, event.x, event.y)
                self.current_state = STATE_INIT
            else:
                0/0 # can't happen
        if event.type == gtk.gdk.BUTTON_RELEASE:
            if event.button != 1:
                pass
            if self.current_state == STATE_FIRST_PRESS:
                self.draw_curve(item, event.x, event.y)
                self.current_state = STATE_FIRST_RELEASE
                pass
        if event.type == gtk.gdk.MOTION_NOTIFY:
            if self.current_state==STATE_FIRST_PRESS:
                self.draw_curve(item, event.x, event.y)

        return False

    def item_event(self, item, event, *args):
        #print args
        #FIXME: does this work: ?
	if (event.type == gtk.gdk.BUTTON_PRESS) and (event.button == 1) and (event.state & gtk.gdk.SHIFT_MASK):
            item.destroy()
            if (item == self.current_item):
                self.current_item = None
                self.current_state = STATE_INIT
            return True

	return False

    def draw_curve(self, item, x, y):
	#MateCanvasPathDef *path_def;
	#MateCanvasGroup   *root;
	#root = MATE_CANVAS_GROUP (item->parent);

	if self.current_state == STATE_INIT:
            #if self.current_item is None:
            # g_assert (!current_item); (but that shouln't happen)
            #FIXEM: port creation of paths
            if self.current_points is None:
                    current_points = []
            self.current_points.append((x,y))

        elif self.current_state == STATE_FIRST_PRESS:
            self.current_points = self.current_points[:1]
            self.current_points.append((x,y))
            #coords [2] = x;
            #current_points->coords [3] = y;

            path=[]
            path.append((matecanvas.MOVETO_OPEN, 0,0))
            path.append((matecanvas.MOVETO,
                         self.current_points[0][0], self.current_points[0][1]))
            path.append((matecanvas.LINETO,
                         self.current_points[1][0], self.current_points[1][1]))
            
            path_def = matecanvas.path_def_new(path)
            if self.current_item:
                    self.current_item.set_bpath(path_def)
            else:
                    self.current_item = self.acanvas.root().add(
                        matecanvas.CanvasBpath,
                        outline_color="blue",
                        width_pixels= 5,
                        cap_style=gtk.gdk.CAP_ROUND)
                    self.current_item.set_bpath(path_def)
                    self.current_item.connect('event', self.item_event)


	elif self.current_state == STATE_FIRST_RELEASE:
            #g_assert (current_item);
            self.current_points.append((x,y))
            #current_points->coords [4] = x;
            #current_points->coords [5] = y;
            path=[]
            #path_def = mate_canvas_path_def_new ();
            path.append((matecanvas.MOVETO_OPEN, 0,0))
            path.append((matecanvas.MOVETO,
                         self.current_points[0][0], self.current_points[0][1]))
            path.append((matecanvas.CURVETO,
                         self.current_points[2][0], self.current_points[2][1],
                         self.current_points[2][0], self.current_points[2][1],
                         self.current_points[1][0], self.current_points[1][1]))
            path_def = matecanvas.path_def_new(path)
            self.current_item.set_bpath(path_def)
            #mate_canvas_path_def_unref (path_def);

	elif self.current_state == STATE_SECOND_PRESS:
            #g_assert (current_item);
            self.current_points.append((x,y))
            #current_points->coords [6] = x;
            #current_points->coords [7] = y;
            path=[]
            path.append((matecanvas.MOVETO_OPEN, 0,0))
            path.append((matecanvas.MOVETO,
                         self.current_points[0][0], self.current_points[0][1]))
            path.append((matecanvas.CURVETO,
                         self.current_points[2][0], self.current_points[2][1],
                         self.current_points[3][0], self.current_points[3][1],
                         self.current_points[1][0], self.current_points[1][1]))
            path_def = matecanvas.path_def_new(path)
            self.current_item.set_bpath(path_def)
            #mate_canvas_path_def_unref (path_def);

            self.current_item = None

	else:
            0/0# shouldn't happen


if __name__ == '__main__':
    c = CurveExample()
    gtk.main()
