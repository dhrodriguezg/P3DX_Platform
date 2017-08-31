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

const int CNTRL_FREQ = 20; // Frequency at which we will publish the final stream

//Transceiver Class
class Transceiver {
  public:

    // Node Handlers
    ros::NodeHandle nsimulator_, np3dx1_, np3dx2_;
    ros::NodeHandle nh_;

    // Android variables
    int selection;
    bool simulator;
    bool topdown;
    bool firstperson;
    bool usb;
    bool assigned;
    //Messages
    sensor_msgs::CompressedImage simulatorView;
    sensor_msgs::CompressedImage topdownView;
    sensor_msgs::CompressedImage fisrtpersonView;
    sensor_msgs::CompressedImage usbView;
    sensor_msgs::CompressedImage viewToSend;

    //Subscribers
    ros::Subscriber simulator_sub;
    ros::Subscriber topdown_sub;
    ros::Subscriber firstperson_sub;
    ros::Subscriber usbcam_sub;
    ros::Subscriber selection_sub;

    //Publishers
    ros::Publisher android_view_pub;

    // Name our nodehandle "wam" to preceed our messages/services
    Transceiver() : nsimulator_("rosaria/simulator"), np3dx1_("rosaria/p3dx_1"), np3dx2_("rosaria/p3dx_2") {}
    void init();

    void selectionCallback(const std_msgs::Int32::ConstPtr& selection_msg);
    void simulatorViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg);
    void topdownViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg);
    void fisrtpersonViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg);
    void usbCamViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg);
    void sendMsgs();
    ~Transceiver() {}
};

// WAM Teleoperation Initialization Function
void Transceiver::init() {

     // initializing Android variables
    selection=0; //it sends the msg to all systems.
    simulator = false;
    topdown = false;
    firstperson = false;
    usb = false;
    assigned = false;

    std::string firstperson_ns;
    std::string simulator_ns;
    std::string topdown_ns;
    std::string usbcam_ns;

    nh_.param<std::string>("firstperson_ns", firstperson_ns, "/first_person");
    nh_.param<std::string>("simulator_ns", simulator_ns, "/simulator");
    nh_.param<std::string>("topdown_ns", topdown_ns, "/top_down");
    nh_.param<std::string>("usbcam_ns", usbcam_ns, "/usb_cam");

    //Subscribers
    simulator_sub = nh_.subscribe <sensor_msgs::CompressedImage> ( simulator_ns + "/image_raw/compressed", 1, &Transceiver::simulatorViewCallback, this);
    topdown_sub = nh_.subscribe <sensor_msgs::CompressedImage> ( topdown_ns + "/image_raw/compressed", 1, &Transceiver::topdownViewCallback, this);
    firstperson_sub = nh_.subscribe <sensor_msgs::CompressedImage> ( firstperson_ns + "/image_raw/compressed", 1, &Transceiver::fisrtpersonViewCallback, this);
    usbcam_sub = nh_.subscribe <sensor_msgs::CompressedImage> ( usbcam_ns + "/image_raw/compressed", 1, &Transceiver::usbCamViewCallback, this);
    selection_sub = nh_.subscribe <std_msgs::Int32> ( "/stream/selection", 1, &Transceiver::selectionCallback, this);

    //Publishers
    android_view_pub = nh_.advertise<sensor_msgs::CompressedImage>( "/android/image_raw/compressed", 1);

}

void Transceiver::selectionCallback(const std_msgs::Int32::ConstPtr& selection_msg){
    selection = selection_msg->data;
}


void Transceiver::simulatorViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg){
    simulatorView.header = image_msg->header;
    simulatorView.format = image_msg->format;
    simulatorView.data = image_msg->data;
    simulator = true;
}

void Transceiver::topdownViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg){
    topdownView.header = image_msg->header;
    topdownView.format = image_msg->format;
    topdownView.data = image_msg->data;
    topdown = true;
}

void Transceiver::fisrtpersonViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg){
    fisrtpersonView.header = image_msg->header;
    fisrtpersonView.format = image_msg->format;
    fisrtpersonView.data = image_msg->data;
    firstperson = true;
}

void Transceiver::usbCamViewCallback(const sensor_msgs::CompressedImage::ConstPtr& image_msg){
    usbView.header = image_msg->header;
    usbView.format = image_msg->format;
    usbView.data = image_msg->data;
    usb = true;
}

void Transceiver::sendMsgs(){

    if(selection==0 && simulator){
	viewToSend=simulatorView;
        assigned = true;
    }else if(selection==1 && topdown){
	viewToSend=topdownView;
        assigned = true;
    }else if(selection==2 && firstperson){
	viewToSend=fisrtpersonView;
        assigned = true;
    }else if(selection==3 && usb){
	viewToSend=usbView;
        assigned = true;
    }

    //checks if there are data to send.
    if (assigned)
	android_view_pub.publish(viewToSend);

}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "stream_transceiver");
    Transceiver transceiver;
    transceiver.init();

    ros::Rate pub_rate(CNTRL_FREQ);

    while (transceiver.nh_.ok()) {
        ros::spinOnce();
        transceiver.sendMsgs();
        pub_rate.sleep();
    }
    return 0;
}
