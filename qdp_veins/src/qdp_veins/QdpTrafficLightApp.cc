//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "qdp_veins/QdpTrafficLightApp.h"
#include <algorithm>

using namespace qdp_veins;
using namespace veins;

Define_Module(qdp_veins::QdpTrafficLightApp);

void QdpTrafficLightApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0){
        tlInterfaceAccess = TraCITrafficLightInterfaceAccess().get(getParentModule());
        traci = tlInterfaceAccess->getCommandInterface();
        tl_id = tlInterfaceAccess->getExternalId();
        tl_position = tlInterfaceAccess->getPosition();
        did_change = false;
        emveh_is_very_near = false;
        TG = 0.0; // to be changed during computation
        NUM_VEHICLES = 0; // to be changed every second

        // obtaining params from omnetpp.ini
        road_id = par("approachRoadID").stdstringValue();
        const char* tmp_det_ids = par("approachDetIDs").stringValue();
        e2_ids = cStringTokenizer(tmp_det_ids).asVector();
        tl_indices_ev_way = initIndicesEvWay();
    }
    else if (stage == 1){
        auto recurringCallbackUpdateNV = [this](){
            if(!isEvWayCurrentlyGreen()){
                NUM_VEHICLES = getNumOfVehiclesFromDetectors();
                EV << "vehicles: " << NUM_VEHICLES << "\n";
            }
        };
        simtime_t firstStepAt = par("firstStepAt");
        auto recurringTimerSpecUpdateNV = TimerSpecification(recurringCallbackUpdateNV).interval(1).absoluteStart(firstStepAt);
        timerManager.create(recurringTimerSpecUpdateNV, "recurring timer, updateNV");
    }
}

std::vector<int> QdpTrafficLightApp::initIndicesEvWay(){
    // this will get us controlled links which corresponds to an approach to intersection
    // from which the emergency vehicle will come (approach is road_id)
    std::vector<int> ret;
    std::list<std::string> lanes = traci->trafficlight(tl_id).getControlledLanes();
    int idx = 0;
    for (auto& lane : lanes){
        // example: road_id = edge1, then lanes we want are edge1_0, edge1_1 etc.
        if (lane.substr(0, road_id.length()) == road_id && lane[road_id.length()] == '_')
            ret.push_back(idx);
        ++idx;
    }
    return ret;
}

void QdpTrafficLightApp::finish()
{
    DemoBaseApplLayer::finish();
}

void QdpTrafficLightApp::onBSM(DemoSafetyMessage* bsm) {
    // icon to green, visualize TL received the beacon
    findHost()->getDisplayString().setTagArg("i", 1, "green");

    Coord emveh_position = bsm->getSenderPos();
    Coord emveh_speed = bsm->getSenderSpeed();

    if (emveh_is_very_near){
        double distance = traci->getDistance(emveh_position, tl_position, false);
        if (distance >= dist_away)
            instructLogicToStopPreemption();
        return;
    }

    double distance = computeDistanceToTraffiLightDownstream(emveh_position);
    if (distance <= dist_close){
        // do not compute if the EV is very near
        // this prevents calling getDistance(_,_, drivingDist = true) when EV is behind TL
        emveh_is_very_near = true;
        return;
    }

    // compute Arrival Time
    auto TA = calc_TA(distance);
    // get time when last veh is at max speed -> T_L
    auto TL = calc_TL(NUM_VEHICLES);
    // get eXtension Time
    auto TX = calc_TX(NUM_VEHICLES, TL);
    // get actual green time GT (if applicable)
    TG = getNewTG();

    // compute time to start preemption
    auto TP = 0.0;
    if ( ! isEvWayCurrentlyGreen()) {
        TP = TA-(TL+TX) - t_conservative;
    } else {
        TP = TA-std::max(0.0, TL+TX-TG) - t_conservative;
        auto Tph = getActualPhaseTime();
        if(Tph > t_min && TP < t_sw + expDelay(t_sw))
            TP=0.0;  // no time for another red
    }

    EV << "T_P: " << TP << "\n";
    instructLogicToStartPreemptionIn(TP);
}

void QdpTrafficLightApp::handleSelfMsg(cMessage* msg) {
    if(timerManager.handleMessage(msg))
        return;
    DemoBaseApplLayer::handleSelfMsg(msg);
}

void QdpTrafficLightApp::instructLogicToStartPreemptionIn(double timeOffset) {
    QdpTrafficLightApplToLogicMsg* msg = generateApplToLogicMsg(timeOffset, START_PREEMPTION);
    send(msg, "logic$o");
}

void QdpTrafficLightApp::instructLogicToStopPreemption() {
    QdpTrafficLightApplToLogicMsg* msg = generateApplToLogicMsg(0.0, STOP_PREEMPTION);
    send(msg, "logic$o");
}

QdpTrafficLightApplToLogicMsg* QdpTrafficLightApp::generateApplToLogicMsg(simtime_t content, int type) {
    std::string msgName = tl_id + "_MsgToLogic";
    QdpTrafficLightApplToLogicMsg* ret = new QdpTrafficLightApplToLogicMsg(msgName.c_str());
    ret->setTlId(tl_id.c_str());
    ret->setTimeToStartPref(content);
    ret->setApplRequestType(type);
    ret->setIndices(tl_indices_ev_way);
    return ret;
}

bool QdpTrafficLightApp::isEvWayCurrentlyGreen() {
    std::string curr_state = tlInterfaceAccess->getCurrentState();
    for(int i : tl_indices_ev_way)
        if(curr_state[i] != 'g' && curr_state[i] != 'G')
            return false;
    return true;
}

double QdpTrafficLightApp::getNewTG(){
    bool is_green = isEvWayCurrentlyGreen();
    return is_green ? getActualPhaseTime() : 0.0;
}

double QdpTrafficLightApp::getActualPhaseTime(){
    auto remaining_phase_time = tlInterfaceAccess->getRemainingDuration();
    // auto phase_duration = tlInterfaceAccess->getCurrentPhase().duration;  // FIXME why is this producing error?
    auto phase_duration = traci->trafficlight(tl_id).getDefaultCurrentPhaseDuration();
    auto res = phase_duration - remaining_phase_time;
    return res.dbl();
}

int QdpTrafficLightApp::getNumOfVehiclesFromDetectors(){
    // pick the biggest queue from all the detectors that are
    // located in the direction the emergency vehicle is coming from
    int num_vehicles = 0;
    for (auto s : e2_ids)
        num_vehicles = std::max(num_vehicles, traci->laneAreaDetector(s).getJamLengthVehicle());
    return num_vehicles;
}

/*
 * This method taken from:
 * https://stackoverflow.com/questions/50864775/how-to-get-the-distance-or-travel-time-between-two-nodes-e-g-vehicles-in-vei
 */
double QdpTrafficLightApp::computeDistanceToTraffiLightDownstream(veins::Coord emveh_pos){
    EV << "searching distance for: (" << tl_id << ", " << tl_position << " )\n";
    std::list<std::string> lanes = traci->trafficlight(tl_id).getControlledLanes();
    // controlled lanes
    for(auto lane : lanes){
        // which controlled lane in the road_id
        if(traci->lane(lane).getRoadId() == road_id){  // found
            // return last Coord of this lane
            std::list<Coord> laneCoords = traci->lane(lane).getShape();
            double distance = traci->getDistance(emveh_pos, laneCoords.back(), true);
            EV << "computeDistanceToUpstream: distance = " << distance << "\n";
            return distance;
        }
    }
    return -1.0;  // no lanes
}
