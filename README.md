# CP Rail Capstone Project Code
## Purpose
This project was done through the University of Calgary and sponsored by CP Rail with the goal of moving live rail test environments to a lab setting instead. The idea is to electrically simulate rail and train movement. 

To properly imagine this, consider rail as two long conductors in parallel with the wood blocks connecting the two. The train can be represented as a shunt connecting the two long conductors that is moving along the track.

This repository includes the code used to control for the "master" Arduino and the "slave" Arduino. The master Arduino is responsible for the user interface, setting up communications between all the Arduinos, and sending train position data to the "slave" Arduinos. Communication between Arduinos is done with the use of I2C and UART protocols with some Stop and Wait ARQ algorithms implemented. 

## Links
For more information on how the system works, please look at the following three links:

(University of Calgary Capstone Webpage)
https://engineeringdesignfair.ucalgary.ca/electrical/train-simulation-system-for-wayside-equipment-testing/

(Short video explaining full project)
https://www.youtube.com/watch?v=uUO5qHXPe3A&ab_channel=AsadAnjum

(Short video demonstrating user interface and controls)
https://youtu.be/CfxSX29fn40