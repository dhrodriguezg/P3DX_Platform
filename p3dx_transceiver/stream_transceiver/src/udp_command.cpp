#include <math.h>
#include <ros/ros.h>

#include "std_msgs/Bool.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Int32.h"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Twist.h"
#include "std_srvs/Empty.h"
#include "sensor_msgs/JointState.h"
#include "sensor_msgs/CompressedImage.h"
#include "sensor_msgs/Joy.h"

#include <boost/thread/thread.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int CNTRL_FREQ = 100; // Frequency at which we will publish the final stream
const int PORT = 58528;
const int BUFSIZE = 256;
const char velocity_str[] = "velocity";
const char ptz_str[] = "ptz";

//UDPCommand Class
class UDPCommand {
  public:

    // Node Handlers
    ros::NodeHandle nh_;

    //Publishers
    ros::Publisher command_pub;
    ros::Publisher ptz_pub;

    //Subscribers
    ros::Subscriber rosaria_velocity_sub;
    ros::Subscriber ptz_sub;
    ros::Subscriber joy_sub;


    bool udpReady;                    /* # bytes received */

    //udp socket
    struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen;              /* length of addresses */
    int recvlen;                    /* # bytes received */
    int fd;                         /* our socket */
    char buf[BUFSIZE];     /* receive buffer */

    // Name our nodehandle "wam" to preceed our messages/services
    UDPCommand() : nh_("udp_command") {}
    void init();
    void transceiveMsgs();
    void transceiverTwist(const geometry_msgs::Twist::ConstPtr& msg);
    void transceiverPTZ(const std_msgs::Int32::ConstPtr& msg);

    void joyCallback(const sensor_msgs::Joy::ConstPtr& joy_msg);
    ~UDPCommand() { close(fd); }
};

// WAM Teleoperation Initialization Function
void UDPCommand::init() {

    int fail=0;
    udpReady = true;

    //Subscribers
    rosaria_velocity_sub = nh_.subscribe<geometry_msgs::Twist> ("/android/cmd_vel", 1, &UDPCommand::transceiverTwist, this );
    ptz_sub = nh_.subscribe<std_msgs::Int32> ("/android/camera_ptz", 1, &UDPCommand::transceiverPTZ, this );
    joy_sub = nh_.subscribe < sensor_msgs::Joy > ("joy", 1, &UDPCommand::joyCallback, this); // /joy

    //Publishers
    command_pub = nh_.advertise<geometry_msgs::Twist>( "/rosaria/cmd_vel", 1);
    ptz_pub = nh_.advertise<std_msgs::Int32>( "/first_person/camera_ptz", 1);

    //socket config
    socklen_t addrlen = sizeof(remaddr);
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ROS_INFO_STREAM("Can't create the socket :(");
	fail = fail+1;
    }
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF, &BUFSIZE, sizeof(BUFSIZE) );

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(PORT);
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
	ROS_INFO_STREAM("bind failed :(");
	fail = fail+1;
    }

    if( fail>0 )
        ROS_INFO_STREAM("UDP Command Socket NOT created :(");
    else
        ROS_INFO_STREAM("UDP Command Socket created!");

}

void UDPCommand::transceiveMsgs(){

while ( nh_.ok() ){

    geometry_msgs::Twist velocity_msg;
    std_msgs::Int32 ptz_msg;
    velocity_msg.linear.y = 0.0;
    velocity_msg.linear.z = 0.0;
    velocity_msg.angular.x = 0.0;
    velocity_msg.angular.y = 0.0;
    int command = 0;
    int ptz = 0;

    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen > 0) {
        buf[recvlen] = 0;
	char* pch;
	pch = strtok (buf,";");
	while (pch != NULL)
	{
	    if( command==1 )
		velocity_msg.linear.x = (float)atof(pch);
	    if( command==2 ){
		velocity_msg.angular.z = (float)atof(pch);
	        command_pub.publish(velocity_msg);
	    }

	    if( command > 0 )
	  	command=command+1;
	    if (strcmp (pch,velocity_str) == 0){
		command=1;
		ptz=0;
	    }

	    if( ptz==1 ){
		ptz_msg.data = (int)atoi(pch);
        	ptz_pub.publish(ptz_msg);
	    }

	    if( ptz > 0 )
	  	ptz=ptz+1;

	    if (strcmp (pch,ptz_str) == 0){
		command=0;
		ptz=1;
	    }

	    pch = strtok (NULL, ";");
	}
    }

}

}

void UDPCommand::transceiverTwist(const geometry_msgs::Twist::ConstPtr& msg){
    geometry_msgs::Twist velocity_msg;
    velocity_msg.linear.x = msg->linear.x;
    velocity_msg.linear.y = msg->linear.y;
    velocity_msg.linear.z = msg->linear.z;
    velocity_msg.angular.x = msg->angular.x;
    velocity_msg.angular.y = msg->angular.y;
    velocity_msg.angular.z = msg->angular.z;
    command_pub.publish(velocity_msg);
}

void UDPCommand::transceiverPTZ(const std_msgs::Int32::ConstPtr& msg){
    std_msgs::Int32 ptz_msg;
    ptz_msg.data = msg->data;
    ptz_pub.publish(ptz_msg);
}

void UDPCommand::joyCallback(const sensor_msgs::Joy::ConstPtr& joy_msg) {

//izq 1 der -1
    float izqder1 = joy_msg->axes[0];
    float topdown1 =joy_msg->axes[1];

    float izqder2 = joy_msg->axes[2];
    float topdown2 =joy_msg->axes[3];

    geometry_msgs::Twist velocity_msg;
    std_msgs::Int32 ptz_msg;
    velocity_msg.linear.y = 0.0;
    velocity_msg.linear.z = 0.0;
    velocity_msg.angular.x = 0.0;
    velocity_msg.angular.y = 0.0;
    
    velocity_msg.linear.x = izqder1;
    velocity_msg.angular.z = topdown2;
    command_pub.publish(velocity_msg);

}


int main(int argc, char** argv)
{
    using namespace std;
    ros::init(argc, argv, "command_transceiver");
    UDPCommand udpCommand;
    udpCommand.init();
    ros::Rate pub_rate(CNTRL_FREQ);
    boost::thread t( &UDPCommand::transceiveMsgs, &udpCommand );
    ros::spin();

    return 0;
}
