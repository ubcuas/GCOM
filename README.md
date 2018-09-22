# GCOM
<b>G</b>round <b>Com</b>mand Software

## Overview

This is a repository for UBC Unmanned Aircraft Systems' ground control station software. The software will act as a form of master server and has the following features:

*	Control the ground antenna tracker
*	Download and tag images off of the drone
*	Send/receive data and commands to and from the drone
*	Stitch incoming images into orthphotos
*	Perform detection algorithms on images off of drone
*	Perform analytics on retrieved images
*	Keep a reliable/recoverable communication with the drone

To keep the program as streamlined and optimized as possible, the software uses a modular design, with a solid and well-documented framework to hold everything together. This allows for the deployment of numerous versions of GCOM, one for each of our drones, where each version will have different modules enabled.

### Prerequisities

* [Qt](https://www.qt.io/ "Qt's Homepage")
*	[MAVLink](https://github.com/mavlink/mavlink "MAVLink Git Repo")

## Modules

Module | Designation
--- | :---:
1 | MAVLink Relay
2 | Ground Antenna Control
3 | Image Fetch and Tag
4 | Drone Command & Control
5 | Collision Avoidance
6 | GCOM Controller

### MAVLink Relay

The MAVLink Relay relays MAVLink messages from [Mission Planner](http://ardupilot.org/planner/ "Mission Planner Homepage") to other modules within GCOM. Connection to Mission Planner or any other MAVLink source is done via TCP protocol, continously receiving messages through the socket. These messages are then decoded using MAVLink libraries.

Two message types are filtered out and relayed:

*	`MAVLINK_MSG_ID_GLOBAL_POSITION_INT`
*	`MAVLINK_MSG_ID_CAMERA_FEEDBACK`

The former is used by the antenna tracker module, and the latter is used by the image processing modules. These messages are relayed via shared pointer signals which can be connected to [slots](http://doc.qt.io/qt-5/signalsandslots.html "Qt Signals & Slots") within the GCOM system.

> Note that the mavrelayCamInfo is triggered **twice** for each photo taken. Once when pressed, and once when released.

### Ground Antenna Controller

The antenna tracker currently uses an Arduino Uno as a data acquisition module, specifically to gather GPS and altitude information of the antenna tracker. GCOM also constantly receives telemetry packets from Mission Planner. Using this data it is possible to calculate the required movements for the antenna tracker so that it is constantly facing the drone. The software then uses a Zaber controller to move the antenna tracker accordingly.

### Image Fetch & Tag

The image fetcher and tagger saves incoming images retrieved from the drone to disc and tags them with GPS coordinates based on telemetry data received from Mission Planner.

### Drone Command & Control

The Drone Command and Control is GCOM's primary means of communication with the drone's on-board Raspberry Pi. It handles sending commands to the Pi in our own message format. This module also handles receiving responses from the Pi and signaling and passing on responses to the appropriate listeners of these responses.

### Collision Avoidance

Currently in progress.

### GCOM Controller

The GCOM Controller refers to the GUI for GCOM, besides providing a user-friendly way to interact with all the various modules. The controller is responsible for the instantiation of said modules and managing their dependencies. The core principle of the GCOM Controller is that there exists two modules which act as the basis for everything else:

*	MAVLink Relay
*	Drone Command & Control

Upon creation, the controller creates the MAVLink relay, as well as the DCNC. Users of GCOM are then presented with two possible actions.
