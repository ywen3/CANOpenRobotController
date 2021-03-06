cmake_minimum_required(VERSION 3.3)
project(CORC C CXX)

#To cross-compile for Arm (BeagleBone) use the armhf toolchain:
# cmake -DCMAKE_TOOLCHAIN_FILE=../armhf.cmake ..
#To cross-compile with ROS support prepare a sysroot and pass it to cmake:
# cmake -DCMAKE_SYSROOT=/path/to/sysroot/ -DCMAKE_TOOLCHAIN_FILE=../armhf.cmake ..
# See doc for more details.
################################## USER FLAGS ##################################

## Select the application by setting the state machine to use
#(it should be the class name and have corresponding header naming in a corresponding folder)
# If your application use ROS, don't forget to change the flag bellow to.
#set (STATE_MACHINE_NAME "M3DemoMachine")
#set (STATE_MACHINE_NAME "M3Machining")
#set (STATE_MACHINE_NAME "M3Chai")
#set (STATE_MACHINE_NAME "ExoTestMachine")
#set (STATE_MACHINE_NAME "X2DemoMachine")
set (PLATFORM_NAME "M1")
#set (STATE_MACHINE_NAME "M1TestMachine")
set (STATE_MACHINE_NAME "M1DemoMachine")

# Comment to use actual hardware, uncomment for a nor robot (virtual) app
set(NO_ROBOT OFF)

# ROS Flag. set ON if you want to use ROS. Else, set OFF.
set(USE_ROS OFF)

if(USE_ROS)
	add_definitions(-DUSEROS)
endif()

if(NO_ROBOT AND USE_ROS)
	set(SIM ON)
    add_definitions(-DSIM)
endif()


## Compile as C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(USE_ROS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall " )
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -static-libstdc++ -static-libgcc" )
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)


## Flags (Release is the default)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_definitions(-DSTATE_MACHINE_TYPE=${STATE_MACHINE_NAME})
add_definitions(-DSTATE_MACHINE_INCLUDE="${STATE_MACHINE_NAME}.h")
if(NO_ROBOT)
	add_definitions(-DNOROBOT=1)
endif()


## Get all source and header files (only the target app folder is included)
file(GLOB_RECURSE SOURCES "src/core/*.cpp" "src/core/*.c" "src/hardware/IO/*.cpp" "src/hardware/IO/*.c" "src/hardware/drives/*.cpp" "src/hardware/drives/*.c" "src/hardware/platforms/${PLATFORM_NAME}/*.cpp" "src/hardware/platforms/${PLATFORM_NAME}/*.c" "src/apps/${STATE_MACHINE_NAME}/*.c" "src/apps/${STATE_MACHINE_NAME}/*.cpp")
file(GLOB_RECURSE HEADERS "src/core/*.h" "src/hardware/IO/*.h" "src/hardware/drives/*.h" "src/hardware/platforms/${PLATFORM_NAME}/*.h" "src/apps/${STATE_MACHINE_NAME}/*.h")

## Set every folder containing .h file as include directory
set (INCLUDE_DIRS "")
foreach (_headerFile ${HEADERS})
	get_filename_component(_dir ${_headerFile} PATH)
	list (APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
## Add libraries headers
list (APPEND INCLUDE_DIRS lib/)


## Add ROS dependencies
if(USE_ROS)
	#ROS local compile: use catkin
	if(NOT CMAKE_CROSSCOMPILING)
		message("--catkin--")
		# Required ROS packages
		find_package(catkin REQUIRED COMPONENTS
				roscpp
				rospy
				std_msgs
				sensor_msgs
				geometry_msgs
        )
		if(SIM)
			find_package(catkin REQUIRED COMPONENTS
			controller_manager_msgs
			cob_gazebo_ros_control
			x2_description
			)
		endif()


		catkin_package(
				#  INCLUDE_DIRS include
				#  LIBRARIES x2
				#  CATKIN_DEPENDS roscpp rospy std_msgs
				#  DEPENDS system_lib
		)

		#include CATKIN
		include_directories(${catkin_INCLUDE_DIRS})
		set(ROS_LIBRARIES ${catkin_LIBRARIES})
	#Cross-compile with ROS (w/o catkin)
	else()
		# Required ROS packages
		find_package(roscpp REQUIRED CONFIG ONLY_CMAKE_FIND_ROOT_PATH)
		find_package(std_msgs REQUIRED CONFIG ONLY_CMAKE_FIND_ROOT_PATH)
		find_package(sensor_msgs REQUIRED CONFIG ONLY_CMAKE_FIND_ROOT_PATH)
		find_package(geometry_msgs REQUIRED CONFIG ONLY_CMAKE_FIND_ROOT_PATH)
		# If cross-compiling force search in sysroot
		if(IS_DIRECTORY ${CMAKE_SYSROOT})
			message("-- Using sysroot folder: ${CMAKE_SYSROOT}")
			#Reset include dirs towards sysroot only
			foreach(inc_dir ${roscpp_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})
				string(REGEX REPLACE "^/usr/" "${CMAKE_SYSROOT}usr/" inc_dir_cross ${inc_dir})
				include_directories(${inc_dir_cross})
			endforeach()
			#Reset libraries towards sysroot only
			foreach(lib_path ${roscpp_LIBRARIES} ${std_msgs_LIBRARIES})
				string(REPLACE "/" ";" lib_path_list ${lib_path}) #Breakdown path in list
				list(REVERSE lib_path_list)
				list(GET lib_path_list 0 lib) #Get last element (library name)
				find_library(new_path ${lib} PATHS ${CMAKE_SYSROOT}) #Force to look for it in actual sysroot path
				list(APPEND ROS_LIBRARIES ${new_path}) #Add it
				unset(new_path CACHE) #Clear variable to allow new search
			endforeach()
		else()
			message(FATAL_ERROR "No sysroot (CMAKE_SYSROOT) defined or not an accessible path")
		endif()
	endif()
endif()


## Executable name: {STATEMACHINENAME}_APP
set (APP_NAME ${STATE_MACHINE_NAME}_APP)
if(NO_ROBOT)
	set (APP_NAME ${APP_NAME}_NOROBOT)
endif()
add_executable(${APP_NAME}
				${SOURCES}
		src/apps/M1TestMachine/M1TestMachine.cpp src/apps/M1TestMachine/M1TestMachine.h)


## Includes
target_include_directories(${APP_NAME} PRIVATE ${INCLUDE_DIRS})

## Set required external packages
find_package(Threads REQUIRED)

## Link non-ROS libraries
target_link_libraries(${APP_NAME} ${CMAKE_THREAD_LIBS_INIT})

## Link ROS libraries
if(USE_ROS)
	target_link_libraries(${APP_NAME} ${ROS_LIBRARIES})
endif()

message("-----------------------------------------------\nBuilding application ${APP_NAME}\n-----------------------------------------------")

