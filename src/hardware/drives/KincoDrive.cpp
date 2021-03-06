#include "KincoDrive.h"

#include <iostream>

KincoDrive::KincoDrive(int NodeID) : Drive::Drive(NodeID) {
    //Remap torque reading and writting registers
    OD_Addresses[ACTUAL_TOR] = 0x6078;
    OD_Addresses[TARGET_TOR] = 0x60F6;
}
KincoDrive::~KincoDrive() {
    spdlog::debug("KincoDrive Deleted");
}

bool KincoDrive::init() {
    std::cout << "KincoDrive::init()" << std::endl;
    preop();//Set preop first to disable PDO during initialisation
    if(initPDOs()) {
        resetError();
        return true;
    }
    return false;
}
bool KincoDrive::init(motorProfile profile) {
    std::cout << "KincoDrive::init(motorProfile profile)" << std::endl;
    preop();//Set preop first to disable PDO during initialisation
    if(setMotorProfile(profile)) {
        if(initPDOs()) {
            return true;
        }
    }
    return false;
}

bool KincoDrive::posControlConfirmSP() {
    // for kinco driver, there is no need to set postion control confirm
//    DEBUG_OUT("NodeID " << NodeID << " Kinco::posControlConfirmSP")
//    Drive::posControlConfirmSP();
    return true;
}

bool KincoDrive::initPosControl(motorProfile posControlMotorProfile) {
    spdlog::debug("NodeID {} Initialising Position Control", NodeID);

    sendSDOMessages(generatePosControlConfigSDO(posControlMotorProfile));
    /**
     * \todo Move jointMinMap and jointMaxMap to set additional parameters (bit 5 in 0x6041 makes updates happen immediately)
     *
     */
    return true;
}
//bool KincoDrive::initPosControl() {
//    spdlog::debug("NodeID {} Initialising Position Control", NodeID);
//
//    sendSDOMessages(generatePosControlConfigSDO());
//    return true;
//}
bool KincoDrive::initVelControl(motorProfile velControlMotorProfile) {
    spdlog::debug("NodeID {} Initialising Velocity Control", NodeID);
    /**
     * \todo create velControlMOTORPROFILE and test on exo
     * \todo Tune velocity loop gain index 0x2381 to optimize V control
     *
    */
    sendSDOMessages(generateVelControlConfigSDO(velControlMotorProfile));
    return true;
}
//bool KincoDrive::initVelControl() {
//    spdlog::debug("NodeID {} Initialising Velocity Control", NodeID);
//
//    sendSDOMessages(generateVelControlConfigSDO());
//    return true;
//}
bool KincoDrive::initTorqueControl() {
    spdlog::debug("NodeID {} Initialising Torque Control", NodeID);
    sendSDOMessages(generateTorqueControlConfigSDO());

    return true;
}

bool KincoDrive::resetError(){
    spdlog::debug("NodeID {} reset error", NodeID);
    sendSDOMessages(generateResetErrorSDO());
    return true;
}

bool KincoDrive::initPDOs() {
    spdlog::debug("KincoDrive::initPDOs");

    spdlog::debug("Set up STATUS_WORD TPDO");
    if(sendSDOMessages(generateTPDOConfigSDO({STATUS_WORD}, 1, 0xFF))<0) {
        spdlog::error("Set up STATUS_WORD TPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up ACTUAL_POS and ACTUAL_VEL TPDO");
    if(sendSDOMessages(generateTPDOConfigSDO({ACTUAL_POS, ACTUAL_VEL}, 2, 0x01))<0) {
        spdlog::error("Set up ACTUAL_POS and ACTUAL_VEL TPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up ACTUAL_TOR TPDO");
    if(sendSDOMessages(generateTPDOConfigSDO({ACTUAL_TOR}, 3, 0x01))<0) {
        spdlog::error("Set up ACTUAL_TOR TPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up CONTROL_WORD RPDO");
    if(sendSDOMessages(generateRPDOConfigSDO({CONTROL_WORD}, 1, 0xff))<0) {
        spdlog::error("Set up CONTROL_WORD RPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up TARGET_POS RPDO");
    if(sendSDOMessages(generateRPDOConfigSDO({TARGET_POS}, 2, 0xff))<0) {
        spdlog::error("Set up TARGET_POS RPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up TARGET_VEL RPDO");
    if(sendSDOMessages(generateRPDOConfigSDO({TARGET_VEL}, 3, 0xff))<0) {
        spdlog::error("Set up ARGET_VEL RPDO FAILED on node {}", NodeID);
        return false;
    }

    spdlog::debug("Set up TARGET_TOR RPDO");
    if(sendSDOMessages(generateRPDOConfigSDO({TARGET_TOR}, 4, 0xff, 0x08))<0) {
        //Kinco has a specific word for this with a dedicated subindex
        spdlog::error("Set up TARGET_TOR RPDO FAILED on node {}", NodeID);
        return false;
    }

    return true;
}

std::vector<std::string> KincoDrive::generatePosControlConfigSDO(motorProfile positionProfile) {
    // Define Vector to be returned as part of this method
    std::vector<std::string> CANCommands;
    // Define stringstream for ease of constructing hex strings
    std::stringstream sstream;
    // start drive
    sstream << "[1] " << NodeID << " start";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //Set control word to power up (enable)
    sstream << "[1] " << NodeID << " write 0x6040 0 u16 0x0f";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //enable profile position mode
    sstream << "[1] " << NodeID << " write 0x6060 0 i8 1";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //Set velocity profile
    sstream << "[1] " << NodeID << " write 0x6081 0 i32 " << std::dec << positionProfile.profileVelocity;
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //Set acceleration profile
    sstream << "[1] " << NodeID << " write 0x6083 0 i32 " << std::dec << positionProfile.profileAcceleration;
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //Set deceleration profile
    sstream << "[1] " << NodeID << " write 0x6084 0 i32 " << std::dec << positionProfile.profileDeceleration;
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    //Set instant position mode; important for kinco
    sstream << "[1] " << NodeID << " write 0x6040 0 u16 0x103f";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    return CANCommands;
}

std::vector<std::string> KincoDrive::generateResetErrorSDO() {
    // Define Vector to be returned as part of this method
    std::vector<std::string> CANCommands;
    // Define stringstream for ease of constructing hex strings
    std::stringstream sstream;

    // shutdown
    sstream << "[1] " << NodeID << " write 0x6040 0 u16 0x06";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    // reset fault
    sstream << "[1] " << NodeID << " write 0x6040 0 u16 0x80";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    // reset fault
    sstream << "[1] " << NodeID << " write 0x6040 0 u16 0x06";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    return CANCommands;
}

std::vector<std::string> KincoDrive::readSDOMessage(int address, int datetype) {
    // Define Vector to be returned as part of this method
    std::vector<std::string> CANCommands;
    // Define stringstream for ease of constructing hex strings
    std::stringstream sstream;
    // read message from drive
    sstream.str(std::string());

    switch(datetype){
        case 1:
            sstream << "[1] " << NodeID << " read 0x" << std::hex << address << " 0 u8";
            break;
        case 2:
            sstream << "[1] " << NodeID << " read 0x" << std::hex << address << " 0 u16";
            break;
        case 3:
            sstream << "[1] " << NodeID << " read 0x" << std::hex << address << " 0 i8";
            break;
        case 4:
            sstream << "[1] " << NodeID << " read 0x" << std::hex << address << " 0 i32";
            break;
        default:
            sstream << "[1] " << NodeID << " read 0x" << std::hex << address << " 0 u8";
            break;
    }

    std::cout << sstream.str() << "\n";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    sendSDOMessages(CANCommands);
    return CANCommands;
}

std::vector<std::string> KincoDrive::writeSDOMessage(int address, int value) {
    // Define Vector to be returned as part of this method
    std::vector<std::string> CANCommands;
    // Define stringstream for ease of constructing hex strings
    std::stringstream sstream;
    // read message from drive
    sstream.str(std::string());
    sstream << "[1] " << NodeID << " write 0x" << std::hex << address << " 0 i32 0x" << std::hex << value;
    std::cout << sstream.str() << "\n";
    CANCommands.push_back(sstream.str());
    sstream.str(std::string());

    return CANCommands;
}
