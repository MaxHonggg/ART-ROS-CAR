#include <iostream>
#include <sstream>
#include "ros/ros.h"
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>
#include <string.h>
#include <std_msgs/Int8.h>

 
using namespace std;
double goal_array[11][2]={
	{-14.3,8.89},//1
	{-9.5,8.89},
	{-4.87,7.77},
	{0.573,8.79},
	{0.402,3.67},
	{0.405,0.072},
	{-6.94,0.137},
	{-10.9,0.042},
	{-14.5,0.335},
	{-7.95,4.29},//10
	{-5.62,6.52}//11	
};
class Goal_Publisher
{
    public:
        Goal_Publisher();
        void initMarker();
		void calculate_order();

    private:
        ros::NodeHandle n_;
		int goal_number;
		int goal_count;
		
		int goal_index[11];
		int goal_index_out[11];
        ros::Subscriber L1_controller_v2_sub;
        ros::Publisher goal_pub,marker_pub,goal_region_pub;

        void on_goal(const std_msgs::Int8& goal_num);
        void on_time(const ros::TimerEvent& x);
        ros::Timer timer1;
        visualization_msgs::Marker sphere;


};

//======================================
void Goal_Publisher::calculate_order()
{
    double start_end_vector[1][2];
	double start_goal_vector[11][2];
	double projection_distance[11];
	double cos_start_goal_angel[11];
	double square_root;
	double distance_last;
	int goal_index_last;
	start_end_vector[0][0] = goal_array[goal_index[7]-1][0]-goal_array[goal_index[0]-1][0];
	start_end_vector[0][1] = goal_array[goal_index[7]-1][1]-goal_array[goal_index[0]-1][1];
	for(int i=1;i<goal_number-1;i++)
	{
		start_goal_vector[i][0] = goal_array[goal_index[i]-1][0]-goal_array[goal_index[0]-1][0];
		start_goal_vector[i][1] = goal_array[goal_index[i]-1][1]-goal_array[goal_index[0]-1][1];
		square_root = sqrt((start_goal_vector[i][0])*(start_goal_vector[i][0])+(start_goal_vector[i][1])*(start_goal_vector[i][1]))*sqrt((start_end_vector[0][0]*start_end_vector[0][0])+(start_end_vector[0][1]*start_end_vector[0][1]));
		cos_start_goal_angel[i] = (start_goal_vector[i][0]*start_end_vector[0][0]+start_goal_vector[i][1]*start_end_vector[0][1])/square_root;
		projection_distance[i] = sqrt((start_goal_vector[i][0])*(start_goal_vector[i][0])+(start_goal_vector[i][1])*(start_goal_vector[i][1]))*cos_start_goal_angel[i];
	}
	for (int j=1;j<goal_number-2;j++)
	{
		for (int i=1;i<goal_number-2;i++)
		{
			if(projection_distance[i] > projection_distance[i+1])
			{
				distance_last = projection_distance[i+1];
				goal_index_last = goal_index[i+1];
				projection_distance[i+1] = projection_distance[i];
				goal_index[i+1] = goal_index[i];
				projection_distance[i] = distance_last;
				goal_index[i] = goal_index_last;
			}
			else
			{
				
			}
		}
	}
	goal_index_out[0] = goal_index[0];
	goal_index_out[7] = goal_index[7];
	for (int i=i;i<goal_number-1;i++)
	{
		goal_index_out[i] = goal_index[i];
	}

	//======  输出测试	=========//
	ROS_INFO_STREAM("//======  output test	=========");
	for(int i=0;i<goal_number;i++)
	{
		ROS_INFO("%d, ",goal_index_out[i]);
	}
	ROS_INFO_STREAM("//=========================");
}


//======================================
void Goal_Publisher::initMarker()
{
    sphere.header.frame_id = "map";
    sphere.ns = "Markers";
    sphere.action = visualization_msgs::Marker::ADD;
    sphere.pose.orientation.w = 1.0;
    sphere.id = 0;

    sphere.type = visualization_msgs::Marker::SPHERE;

    sphere.scale.x = 0.35;
    sphere.scale.y = 0.35;
    sphere.scale.z = 0.35;

    // sphere are RED
    sphere.color.r = 1.0;
    sphere.color.g = 0.0;
    sphere.color.b = 0.0;
    sphere.color.a = 0.5;
}

Goal_Publisher::Goal_Publisher()
{
	/*
	add_executable(Goal_Publisher src/Goal_Publisher.cpp)
	target_link_libraries(Goal_Publisher ${catkin_LIBRARIES})
	add_dependencies(Goal_Publisher findLine_tutorials_generate_messages_cpp)

	
	launch文件格式：
	<rosparam command="load" file="$(find racecar_gazebo)/config/goal_list.yaml" />
	
	<node pkg="racecar_gazebo" type="Goal_Publisher" name="Goal_Publisher" />
	
	*/
    ros::NodeHandle n_("Goal_Publisher");
    if (n_.getParam("goal_number", goal_number))
    {
        ROS_INFO_STREAM("Loaded goal_number" << ": " << goal_number);
    }
    else
    {
        ROS_ERROR_STREAM("Failed to load goal_number");
        n_.shutdown();
    }	
	
	for(int i=0;i<goal_number;i++)
	{
		string sname;
		stringstream ss;
		ss<<"goal_"<<i+1;
		sname=ss.str();
		//sprintf(sname,"goal_%d",i);
		n_.getParam(sname, goal_index[i]);
		ROS_WARN("[param] goal_index: %d", goal_index[i]);   //区域编号输入
	}

	goal_count = 0;
	goal_pub = n_.advertise<geometry_msgs::PoseStamped>("/move_base_simple/goal", 1);
    marker_pub = n_.advertise<visualization_msgs::Marker>("car_goal", 10);
	goal_region_pub = n_.advertise<std_msgs::Int8>("/goal_region", 10);

    L1_controller_v2_sub = n_.subscribe("/L1_controller_v2/goal_finish", 1, &Goal_Publisher::on_goal, this);

	timer1 = n_.createTimer(ros::Duration(0.5), &Goal_Publisher::on_time, this); 
	initMarker();	
    //registerPub(n_);
	calculate_order();	//输出为goal_index_out
}
void Goal_Publisher::on_goal(const std_msgs::Int8& goal_num)
{	
	// ROS_WARN("on goal feedback %d",goal_num.data);

	if(goal_num.data == goal_index[goal_count])
	{
		goal_count++;
	}
	
}
void Goal_Publisher::on_time(const ros::TimerEvent&)
{	
	// ROS_WARN("goal_count=%d,goal_number=%d",goal_count,goal_number);

	if(goal_count == goal_number)
	{
		ROS_WARN("|||=====@ Congratulations! @====||||");
		ros::shutdown();		
		return;
	}
	if(ros::ok())
	{
		//querry and publish
		geometry_msgs::PoseStamped odom_goal; 
		
		odom_goal.header.frame_id = "map";
		// odom_goal.header.stamp = ros::Time::now();
		
		odom_goal.pose.position.x = goal_array[goal_index[goal_count]-1][0];
		odom_goal.pose.position.y = goal_array[goal_index[goal_count]-1][1];
		odom_goal.pose.orientation.w = 1;


		goal_pub.publish(odom_goal);

		std_msgs::Int8 goal_region;
		goal_region.data = goal_index[goal_count];
		goal_region_pub.publish(goal_region);
		
		sphere.pose.position.x = goal_array[goal_index[goal_count]-1][0];
		sphere.pose.position.y = goal_array[goal_index[goal_count]-1][1];
		marker_pub.publish(sphere);
		
		// ROS_WARN("publish goal%d,region %d,%d left", goal_count+1,goal_index[goal_count],goal_number-goal_count);
	}
}

/*****************/
/* MAIN FUNCTION */
/*****************/
int main(int argc, char **argv)
{
    //Initiate ROS
    ros::init(argc, argv, "Goal_Publisher");
	Goal_Publisher goal_Publisher;
	while(ros::ok())
    	ros::spin();
    return 0;
}





