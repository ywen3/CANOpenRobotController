#include "MultiM1MachineROS.h"

MultiM1MachineROS::MultiM1MachineROS(RobotM1 *robot) {
    robot_ = robot;
}

MultiM1MachineROS::~MultiM1MachineROS() {
    ros::shutdown();
}

void MultiM1MachineROS::initialize() {
    spdlog::debug("MultiM1MachineROS::init()");

    jointCommandSubscriber_ = nodeHandle_->subscribe("joint_commands", 1, &MultiM1MachineROS::jointCommandCallback, this);
    interactionTorqueCommandSubscriber_ = nodeHandle_->subscribe("interaction_effort_commands", 1, &MultiM1MachineROS::interactionTorqueCommandCallback, this);
    jointStatePublisher_ = nodeHandle_->advertise<sensor_msgs::JointState>("joint_states", 10);
    interactionWrenchPublisher_ = nodeHandle_->advertise<geometry_msgs::WrenchStamped>("interaction_wrench", 10);

    jointPositionCommand_ = Eigen::VectorXd::Zero(M1_NUM_JOINTS);
    jointVelocityCommand_ = Eigen::VectorXd::Zero(M1_NUM_JOINTS);
    jointTorqueCommand_ = Eigen::VectorXd::Zero(M1_NUM_JOINTS);
    interactionTorqueCommand_ = Eigen::VectorXd(M1_NUM_INTERACTION);
    flipFlag_ = false;
}

void MultiM1MachineROS::update() {
    publishJointStates();
    publishInteractionForces();
}

void MultiM1MachineROS::publishJointStates() {
    Eigen::VectorXd jointPositions = robot_->getPosition();
    Eigen::VectorXd jointVelocities = robot_->getVelocity();
    Eigen::VectorXd jointTorques = robot_->getTorque();

    jointStateMsg_.header.stamp = ros::Time::now()-ros::Duration(5);
    jointStateMsg_.name.resize(M1_NUM_JOINTS);
    jointStateMsg_.position.resize(M1_NUM_JOINTS);
    jointStateMsg_.velocity.resize(M1_NUM_JOINTS);
    jointStateMsg_.effort.resize(M1_NUM_JOINTS);
    jointStateMsg_.name[0] = "M1_joint";

    // changing the visualized actual position depending on the flip flag read from GUI
    if(flipFlag_){
        jointStateMsg_.position[0] = 1.6-jointPositions[0];
    }
    else{
        jointStateMsg_.position[0] = jointPositions[0];
    }

    jointStateMsg_.velocity[0] = jointVelocities[0];
    jointStateMsg_.effort[0] = jointTorques[0]; /// BE CAREFUL CHANGED FROM JOINT TORQUE TO DESIRED INTERACTION TORQUE FOR SINGLE ROBOT FORCE CONTROL TEST

    jointStatePublisher_.publish(jointStateMsg_);
}

void MultiM1MachineROS::publishInteractionForces() {
    Eigen::VectorXd interactionTorque = robot_->getJointTor_s();
    ros::Time time = ros::Time::now();

    interactionWrenchMsg_.header.stamp = time;
    interactionWrenchMsg_.header.frame_id = "interaction_torque_sensor";
    interactionWrenchMsg_.wrench.torque.y = -robot_->tau_spring[0];
    interactionWrenchMsg_.wrench.torque.z = interactionTorque[0];

    interactionWrenchPublisher_.publish(interactionWrenchMsg_);
}

void MultiM1MachineROS::setNodeHandle(ros::NodeHandle &nodeHandle) {
    nodeHandle_ = &nodeHandle;
}

void MultiM1MachineROS::setFlipFlag(bool flipFlag) {

    flipFlag_ = flipFlag;
}

bool & MultiM1MachineROS::getFlipFlag() {
    return flipFlag_;
}

void MultiM1MachineROS::jointCommandCallback(const sensor_msgs::JointState &msg) {

    for(int i=0; i<M1_NUM_JOINTS; i++){
        jointPositionCommand_[i] = msg.position[i];
        jointVelocityCommand_[i] = msg.velocity[i];
        jointTorqueCommand_[i] = msg.effort[i];
    }
}

void MultiM1MachineROS::interactionTorqueCommandCallback(const std_msgs::Float64MultiArray &msg) {

    for(int i=0; i<M1_NUM_JOINTS; i++){
        interactionTorqueCommand_[i] = msg.data[i];
    }
}
