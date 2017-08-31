#!/usr/bin/env python
#
# Basic PTZ node, based on documentation here:
#   http://www.axis.com/files/manuals/vapix_ptz_45621_en_1112.pdf
#
import threading
import urllib
import urllib2
import base64
import httplib, urllib
import rospy 
from axis_camera.msg import Axis
from std_msgs.msg import Bool
from std_msgs.msg import Int32
import math
from dynamic_reconfigure.server import Server
from axis_camera.cfg import PTZConfig

class StateThread(threading.Thread):
    '''This class handles the publication of the positional state of the camera 
    to a ROS message'''
    
    def __init__(self, axis):
        threading.Thread.__init__(self)
        self.axis = axis
        self.daemon = True 

    def run(self):
        r = rospy.Rate(100)
        self.msg = Axis()

        while True:
            self.changeCameraPosition()
            r.sleep()
   
    def changeCameraPosition(self):

        self.movement = ''
        if self.axis.command == 0:
            self.movement = 'home'
        if self.axis.command == 1:
            self.movement = 'up'
        if self.axis.command == 2:
            self.movement = 'down'
        if self.axis.command == 3:
            self.movement = 'left'
        if self.axis.command == 4:
            self.movement = 'right'

        self.url = 'http://%s/cgi-bin/viewer/camctrl.cgi?move=%s' % (self.axis.hostname, self.movement)


        if self.axis.command > -1:
            # create a password manager
            rospy.logwarn( self.url )
	    chimpConfig = {
	        "headers" : {
	        "Content-Type": "application/json",
	        "Authorization": "Basic " + base64.encodestring("admin:admin").replace('\n', '')
	        },
	        "url": self.url}
	    datas = None
	    self.url = urllib2.Request(chimpConfig["url"], datas, chimpConfig["headers"])
            try:
		self.axis.command = -1
	        self.fp = urllib2.urlopen(self.url)
		self.axis.command = -1
            except urllib2.URLError, e:
                rospy.logwarn('Error opening URL %s' % (self.url) + 'Possible timeout.  Looping until camera appears')

class AxisPTZ:
    '''This class creates a node to manage the PTZ functions of an Axis PTZ 
    camera'''
    def __init__(self, hostname, username, password, flip, speed_control):
        self.hostname = hostname
        self.username = username
        self.password = password

	self.command = -1

        self.st = None
        self.sub_command = rospy.Subscriber("/camera_ptz", Int32, self.cameraPositionCallback, queue_size=1)

        self.st = StateThread(self)
        self.st.start()

    def cameraPositionCallback(self, msg):
        self.command = msg.data

def main():
    rospy.init_node("axis_twist")

    arg_defaults = {
        'hostname': '192.168.0.90',
        'username': 'ros',
        'password': '',
        'flip': False,  # things get weird if flip=true
        'speed_control': False
        }
    args = {}
    
    # go through all arguments
    for name in arg_defaults:
        full_param_name = rospy.search_param(name)
        # make sure argument was found (https://github.com/ros/ros_comm/issues/253)
        if full_param_name == None:
            args[name] = arg_defaults[name]
        else:
            args[name] = rospy.get_param(full_param_name, arg_defaults[name])

    # create new PTZ object and start dynamic_reconfigure server
    my_ptz = AxisPTZ(**args)
    rospy.spin()

if __name__ == "__main__":
    main()
    
