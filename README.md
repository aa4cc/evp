# QDP Veins

Queue discharge-based emergency vehicle traffic signal preemption simulation in Veins. 'QDP' stands for Queue Discharge-based Preemption.

## Content of this repository

This project contains modules to simulate emergency vehicle preemption approach which is based on the model of queue of vehicles (queue discharge) and V2X communication. There are also two SUMO scenarios which demonstrates this approach. See the publication for details: (TODO: add link). 

### OMNeT++ modules:
The following modules are implemented and can be found (NED files as well as C++ implementations) in `qdp_veins/src/qdp_veins`.
- QdpTrafficLightApp: application layer for traffic light controller. The algorithm to compute time to start preemption (T\_P) is implemented here.
- QdpTrafficLightLogic: module dealing with traffic light signal plans. This module executed the preemption phase after request from QdpTrafficLightApp.
- EmergencyAppl: application layer for emergency vehicle. It beacons position and speed every 1s.
- ForwarderAppl: application layer for public transport, RSUs or other ordinary cars to forward beacons sent by emergency vehicle.

### SUMO scenarios:
Files for SUMO can be found under `qdp_veins/examples/`.

#### discharge
Simplistic showcase of our preemption approach. Straight road with traffic light in the middle. RSU is used to extend the reach of beacons from emergency vehicle. 
![GIF Demo](qdp_veins/doc/discharge2.gif)

#### brno_por
Realistic scenario of one major intersection in the city of Brno, the Czech republic. This scenario includes traffic of other vehicles which was determined using induction loop measurements from the exact area. The example contains traffic for the whole weekday.
![GIF Demo](qdp_veins/doc/brno_por2.gif)
TODO video of real experiment from YT.

If you decide to use this repository or parts of it, consider citing (TODO: add link). The experiment conducted in real traffic is documented in the following video: <https://drive.google.com/open?id=1IjWLiKupIz3QcSHucnmZGD9VQ-uWFXWS>.

## How to run simulations from this repository ##

You need a working installation of OMNeT++ and SUMO. The easiest way would be to use Instant Veins (virtual machine with Veins installation, see <http://veins.car2x.org/documentation/instant-veins/>). Pull this repository into the virtual machine and import this repository as a project to OMNeT++ IDE. Then right-click on the `omnetpp.ini` file from selected example and pick 'Run as' -> 'OMNeT++ Simulation'. For more information, please consult Veins, OMNeT++ and SUMO documentations and tutorials.

### Supported program versions ###
The repository was started using Cookiecutter for Veins (see <https://github.com/veins/cookiecutter-veins-project>). 

- Veins 5.0 (see <http://veins.car2x.org/>)
- OMNeT++ 5.5.1 (see <https://omnetpp.org/>)
- SUMO 1.4.0 (see <https://sumo.dlr.de/docs/>), tested with 1.5.0 and it should work also with 1.6.0

### Modifications to scenarios ###
You can change some settings to see different simulation runs:

#### discharge
1) set different `number` of cars in the queue in `qdp_veins/examples/discharge/discharge.rou.xml`
```xml
<flow id="flow0" type="PASSENGER_MOD" route="route0" begin="0" period="0.1" number="13" departSpeed="max"/>
```

#### brno_por
1) change time of `depart` of emergency vehicle to different time of the day. Change must be reflected in `qdp_veins/examples/brno_por/emergency.rou.xml`:
```xml
<!-- 25500s = 7:05 AM -->
<vehicle id="emergency_01" type="EMERGENCY_AMB" depart="25500" departSpeed="max" route="ev_route_west_to_east"/>
```
You should also change the simulation start and end in `qdp_veins/examples/brno_por/omnetpp.ini`. Sensible setting is to start 5 minutes before the departure and end 5 minutes after:
```
*.manager.firstStepAt = 25200s
sim-time-limit = 25800s 
```
2) road of emergency vehicle. There are two roads available. Setting must be in `qdp_veins/examples/brno_por/emergency.rou.xml`: 
```xml
<!-- 25500s = 7:05 AM -->
<!-- <vehicle id="emergency_01" type="EMERGENCY_AMB" depart="25500" departSpeed="max" route="ev_route_west_to_east"/>-->
<vehicle id="emergency_02" type="EMERGENCY_AMB" depart="25500" departSpeed="max" route="ev_route_east_to_west"/>
```
and in `qdp_veins/examples/brno_por/omnetpp.ini`:
```
### For emergency vehicle to approach from the West side:
#*.trafficLight[*].appl.approachRoadID = "gneE41"
#*.trafficLight[*].appl.approachDetIDs = "2_06_e2_west_0 2_06_e2_west_1"  # two detectors

### For emergency vehicle to approach from the East side:
*.trafficLight[*].appl.approachRoadID = "-4463914#2"
*.trafficLight[*].appl.approachDetIDs = "2_06_e2_east_0 2_06_e2_east_1"  # two detectors

```

#### both
you might want to try with different `desired_speed` of emergency vehicle (or v_op in publication). Change in `qdp_veins/src/qdp_veins/QdpAlgorithmCalculation.h`:
```c++
const double desired_speed = 70/3.6;    // desired speed of emergency vehicle [m/s]
```

### Usage of these modules in different scenarios ###
TODO description
