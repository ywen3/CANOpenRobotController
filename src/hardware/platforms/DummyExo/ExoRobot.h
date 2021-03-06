/**
 *
 * \file ExoRobot.h
 * \author William Campbell
 * \version 0.1
 * \date 2019-09-24
 * \copyright Copyright (c) 2019
 *
 * \breif  The<code> ExoRobot</ code> class represents an ExoSkeleton Robot in terms of its
 * representation of the Alex exoskeleton hardware whose memory is managed in this class.
 *
 *
 * Version 0.1
 * Date: 07/04/2020
 */

#ifndef EXOROBOT_H_INCLUDED
#define EXOROBOT_H_INCLUDED

#include <map>

#include "CopleyDrive.h"
#include "DummyActJoint.h"
#include "Keyboard.h"
#include "Robot.h"
#include "RobotParams.h"
/**
     * \todo Load in paramaters and dictionary entries from JSON file.
     *
     */
/**
 * \brief Example implementation of the Robot class, representing an X2 Exoskeleton, using DummyActuatedJoint and DummyTrajectoryGenerator.
 *
 */
class ExoRobot : public Robot {
   private:
    /**
     * \brief motor drive position control profile paramaters, user defined.
     *
     */
    motorProfile posControlMotorProfile{4000000, 240000, 240000};

   public:
    /**
      * \brief Default <code>ExoRobot</code> constructor.
      * Initialize memory for the Exoskelton <code>Joint</code> + sensors.
      * Load in exoskeleton paramaters to  <code>TrajectoryGenerator.</code>.
      */
    ExoRobot();
    ~ExoRobot();
    Keyboard *keyboard;
    std::vector<CopleyDrive *> copleyDrives;

    // /**
    //  * \brief Timer Variables for moving through trajectories
    //  *
    //  */
    struct timeval tv, tv_diff, moving_tv, tv_changed, stationary_tv, start_traj, last_tv;

    /**
       * \brief Initialises all joints to position control mode.
       *
       * \return true If all joints are successfully configured
       * \return false  If some or all joints fail the configuration
       */
    bool initPositionControl();

    /**
       * \brief Initialises all joints to velocity control mode.
       *
       * \return true If all joints are successfully configured
       * \return false  If some or all joints fail the configuration
   */
    bool initVelocityControl();

    /**
       * \brief Initialises all joints to torque control mode.
       *
       * \return true If all joints are successfully configured
       * \return false  If some or all joints fail the configuration
   */
    bool initTorqueControl();

    /**
      * \brief For each joint, move through(send appropriate commands to joints) the currently
      * generated trajectory of the TrajectoryGenerator object - this assumes the trajectory and robot is in position control.
      *
      * \return true if successful
      * \return false if not successful (e.g. any joint not in position control.)
      */
    bool moveThroughTraj();

    /**
    * \brief Set the target positions for each of the joints
    *
    * \param positions a vector of target positions - applicable for each of the actuated joints
    * \return MovementCode representing success or failure of the application
    */
    setMovementReturnCode_t setPosition(std::vector<double> positions);

    /**
    * \brief Set the target velocities for each of the joints
    *
    * \param velocities a vector of target velocities - applicable for each of the actuated joints
    * \return MovementCode representing success or failure of the application
    */
    setMovementReturnCode_t setVelocity(std::vector<double> velocities);

    /**
    * \brief Set the target torque for each of the joints
    *
    * \param torques a vector of target torques - applicable for each of the actuated joints
    * \return MovementCode representing success or failure of the application
    */
    setMovementReturnCode_t setTorque(std::vector<double> torques);

    /**
    * \brief Get the actual position of each joint
    *
    * \return std::vector<double> a vector of actual joint positions
    */
    std::vector<double> getPosition();

    /**
    * \brief Get the actual velocity of each joint
    *
    * \return std::vector<double> a vector of actual joint positions
    */
    std::vector<double> getVelocity();

    /**
    * \brief Get the actual torque of each joint
    *
    * \return std::vector<double> a vector of actual joint positions
    */
    std::vector<double> getTorque();

    /**
   * Determine if the currently generated trajectory is complete.
   * \return bool
   */
    bool isTrajFinished();

    /**
       * \brief Implementation of Pure Virtual function from <code>Robot</code> Base class.
       * Create designed <code>Joint</code> and <code>Driver</code> objects and load into
       * Robot joint vector.
       */
    bool initialiseJoints();

    /**
       * \brief Implementation of Pure Virtual function from <code>Robot</code> Base class.
       * Initialize each <code>Drive</code> Objects underlying CANOpen Networking.

      */
    bool initialiseNetwork();
    /**
       * \brief Implementation of Pure Virtual function from <code>Robot</code> Base class.
       * Initialize each <code>Input</code> Object.

      */
    bool initialiseInputs();
    /**
       * \brief Free robot objects vector pointer memory.
       */
    void freeMemory();
    /**
       * \brief update current state of the robot, including input and output devices.
       * Overloaded Method from the Robot Class.
       * Example. for a keyboard input this would poll the keyboard for any button presses at this moment in time.
       */
    void updateRobot();
    /**
       * \brief Joint Limit Map between Joint value and min Degrees possible
       * \param int Joint value
       * \return double minDeg
       */

    /**
    * \todo Move jointMinMap and jointMaxMap to RobotParams.h
    *
    */
    /**
       * \brief Joint Limit Map between Joint value and max Degrees possible
       * \param int Joint value
       * \return int maxDeg
       */
    std::map<int, double> jointMinMap = {{LEFT_HIP, 70},
                                         {RIGHT_HIP, 70},
                                         {LEFT_KNEE, 0},
                                         {RIGHT_KNEE, 0},
                                         {LEFT_ANKLE, 75},
                                         {RIGHT_ANKLE, 75}};
    /**
       * \brief Joint Limit Map between Joint value and max Degrees possible
       * \param int Joint value
       * \return int maxDeg
       */
    std::map<int, double> jointMaxMap = {{LEFT_HIP, 210},
                                         {RIGHT_HIP, 210},
                                         {LEFT_KNEE, 120},
                                         {RIGHT_KNEE, 120},
                                         {LEFT_ANKLE, 105},
                                         {RIGHT_ANKLE, 105}};
};
#endif /*EXOROBOT_H*/
