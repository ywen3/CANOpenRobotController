#include "MultiControllerState.h"

double timeval_to_sec_t(struct timespec *ts)
{
    return (double)(ts->tv_sec + ts->tv_nsec / 1000000000.0);
}

void MultiControllerState::entry(void) {

    spdlog::info("Multi Controller State is entered.");

    //Timing
    clock_gettime(CLOCK_MONOTONIC, &initTime);
    lastTime = timeval_to_sec_t(&initTime);

    // set dynamic parameter server
    dynamic_reconfigure::Server<CORC::dynamic_paramsConfig>::CallbackType f;
    f = boost::bind(&MultiControllerState::dynReconfCallback, this, _1, _2);
    server_.setCallback(f);

    robot_->initTorqueControl();
    robot_->applyCalibration();
    robot_->calibrateForceSensors();
    robot_->tau_spring[0] = 0;   // for ROS publish only

    // FOR TRANSPERANCY STUFF
    q = Eigen::VectorXd::Zero(1);
    dq = Eigen::VectorXd::Zero(1);
    tau = Eigen::VectorXd::Zero(1);
    tau_s = Eigen::VectorXd::Zero(1);
    tau_cmd = Eigen::VectorXd::Zero(1);

    // initialize parameters
    control_freq = 800.0;
    error = 0;
    delta_error = 0;
    integral_error = 0;
    tick_count = 0;
    controller_mode_ = -1;
    cut_off = 6;

}
void MultiControllerState::during(void) {

    //Compute some basic time values
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    double now = timeval_to_sec_t(&ts);
    elapsedTime = (now-timeval_to_sec_t(&initTime));
    dt = now - lastTime;
    lastTime = now;

    //
    tick_count = tick_count + 1;
    if(controller_mode_ == 0){  // Homing
        if(cali_stage == 1){
            // set calibration velocity
            JointVec dq_t;
            dq_t(0) = cali_velocity; // calibration velocity
            if(robot_->setJointVel(dq_t) != SUCCESS){
                std::cout << "Error: " << std::endl;
            }

            // monitor velocity and interaction torque
            dq= robot_->getJointVel();
            tau = robot_->getJointTor_s();
            if ((dq(0) <= 2) & (tau(0) >= 2)){
                cali_velocity = 0;
                robot_->applyCalibration();
                robot_->initPositionControl();
                cali_stage = 2;
            }
            else {
                robot_->printJointStatus();
            }
        }
        else if(cali_stage == 2)
        {
            // set position control: 16 is vertical for #2
            q(0) = 16;
            if(robot_->setJointPos(q) != SUCCESS){
                std::cout << "Error: " << std::endl;
            }
            JointVec tau = robot_->getJointTor_s();
            if (tau(0)>0.2 || tau(0)<-0.2)
            {
                robot_->m1ForceSensor->calibrate();
            }
            robot_->printJointStatus();
            std::cout << "Calibration done!" << std::endl;
            cali_stage = 3;
        }
    }
    else if(controller_mode_ == 1){  // zero torque mode
        robot_->setJointTor(Eigen::VectorXd::Zero(M1_NUM_JOINTS));
    }
    else if(controller_mode_ == 2){ // follow position commands
        robot_->setJointPos(multiM1MachineRos_->jointPositionCommand_);
    }
    else if(controller_mode_ == 3){ // follow torque commands
        robot_->setJointTor(multiM1MachineRos_->jointTorqueCommand_);
    }
    else if(controller_mode_ == 4){ // virtual spring - torque mode
        tau = robot_->getJointTor();
        //tau_s = (robot_->getJointTor_s()+tau_s)/2.0;
        dq = robot_->getJointVel();

        // filter q
        q = robot_->getJointPos();
        q_raw = q(0);
        alpha_q = (2*M_PI*dt*cut_off)/(2*M_PI*dt*cut_off+1);
        robot_->filter_q(alpha_q);
        q = robot_->getJointPos();
        q_filtered = q(0);

//         filter torque signal
        tau_s = robot_->getJointTor_s();
        tau_raw = tau_s(0);
        alpha_tau = (2*M_PI*dt*cut_off)/(2*M_PI*dt*cut_off+1);
        robot_->filter_tau(alpha_tau);
        tau_s = robot_->getJointTor_s();
        tau_filtered = tau_s(0);
//        std::cout << "Pre :" << tau_raw << "; Post :" << tau_filtered << std::endl;

        // get interaction torque from virtual spring
        spring_tor = -multiM1MachineRos_->interactionTorqueCommand_(0);
//        spring_tor = spk_*M_PIf64*(45-q(0))/180.0;  //stiffness; q(0) in degree
        robot_->tau_spring[0] = spring_tor;   // for ROS publish only

        // torque tracking with PD controller
        error = tau_s(0) + spring_tor;  // interaction torque error, desired interaction torque is spring_tor, 1.5 is ratio to achieve the desired torque
        delta_error = (error-torque_error_last_time_step)*control_freq;  // derivative of interaction torque error;
        integral_error = integral_error + error/control_freq;
        tau_cmd(0) = error*kp_ + delta_error*kd_ + integral_error*ki_;  // tau_cmd = P*error + D*delta_error; 1 and 0.001
        torque_error_last_time_step = error;
//        std::cout << "spring_tor:" << spring_tor  << "; sensor_tor: " << tau_s(0) << "; cmd_tor: " << tau_cmd(0) << "; motor_tor: " << tau(0) << std::endl;
        robot_->setJointTor_comp(tau_cmd, tau_s, ffRatio_);

        // reset integral_error every 1 mins, to be decided
        if(tick_count >= control_freq*60){
            integral_error = 0;
            tick_count = 0;
        }
    }
    else if(controller_mode_ == 5){ // transperancy - torque mode
        tau = robot_->getJointTor();
        //tau_s = (robot_->getJointTor_s()+tau_s)/2.0;
        dq = robot_->getJointVel();

        // filter q
        q = robot_->getJointPos();
        q_raw = q(0);
        alpha_q = (2*M_PI*dt*cut_off)/(2*M_PI*dt*cut_off+1);
        robot_->filter_q(alpha_q);
        q = robot_->getJointPos();
        q_filtered = q(0);

//         filter torque signal
        tau_s = robot_->getJointTor_s();
        tau_raw = tau_s(0);
        alpha_tau = (2*M_PI*dt*cut_off)/(2*M_PI*dt*cut_off+1);
        robot_->filter_tau(alpha_tau);
        tau_s = robot_->getJointTor_s();
        tau_filtered = tau_s(0);

        // torque tracking with PD controller
        error = tau_s(0);  // interaction torque error, desired interaction torque is 0
        delta_error = (error-torque_error_last_time_step)*control_freq;  // derivative of interaction torque error;
        integral_error = integral_error + error/control_freq;
        tau_cmd(0) = error*kp_ + delta_error*kd_ + integral_error*ki_;  // tau_cmd = P*error + D*delta_error + ; 1 and 0.001
        torque_error_last_time_step = error;
        robot_->setJointTor_comp(tau_cmd, tau_s, ffRatio_);

        // reset integral_error every 1 mins, to be decided
        if(tick_count >= control_freq*tick_max_){
            integral_error = 0;
            tick_count = 0;
        }
    }
}
void MultiControllerState::exit(void) {

}

void MultiControllerState::dynReconfCallback(CORC::dynamic_paramsConfig &config, uint32_t level) {

    kp_ = config.kp;
    kd_ = config.kd;
    ki_ = config.ki;
    ffRatio_ = config.ff_ratio;
    spk_ = config.spk;

    if(tick_max_ != config.tick_max)
    {
        tick_max_ = config.tick_max;
        tick_count = 0;
    }

    multiM1MachineRos_->setFlipFlag(config.flip);

//    controller_mode_ = config.controller_mode;
    if(controller_mode_!=config.controller_mode)
    {
        controller_mode_ = config.controller_mode;
        if(controller_mode_ == 0) {
            robot_->initVelocityControl();
            cali_stage = 1;
            cali_velocity = -30;
        }
        if(controller_mode_ == 1) robot_->initTorqueControl();
        if(controller_mode_ == 2) robot_->initPositionControl();
        if(controller_mode_ == 3) robot_->initTorqueControl();
        if(controller_mode_ == 4) robot_->initTorqueControl();
        if(controller_mode_ == 5) robot_->initTorqueControl();

        if(controller_mode_ == 4) triggerValue_ = 1;
        else triggerValue_ = 0;

        sendInitTrigger(triggerValue_);
    }

    return;
}

bool MultiControllerState::sendInitTrigger(int value) {

    std::chrono::steady_clock::time_point time0;
    time0 = std::chrono::steady_clock::now();

    std::string valueStr = value ? "1" : "0";

    std::stringstream sstream;
    // set mode of operation
    sstream << "[1] " << 1 << " write 0x2010 14 i16 " << valueStr;

    std::string strCOmmand = sstream.str();

    char *SDO_Message = (char *)(strCOmmand.c_str());
    char *returnMessage;
    cancomm_socketFree(SDO_Message, &returnMessage);
    SDOTriggerTime_ = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - time0).count()/1000.0;
    std::cout<<returnMessage<<"--It took "<<SDOTriggerTime_<< "ms"<< std::endl;
}


