import matecomponent
import MateComponent

matecomponent.activate ()

obj = matecomponent.get_object ('OAFIID:MateComponent_Sample_Echo', 'MateComponent/Sample/Echo')
obj.echo ('This is the message from the python client\n')

