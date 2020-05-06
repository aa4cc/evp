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
#pragma once

#include "qdp_veins/qdp_veins.h"
#include <algorithm>
#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/world/traci/trafficLight/TraCITrafficLightInterface.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include "veins/modules/utility/TimerManager.h"
#include "qdp_veins/QdpTrafficLightApplToLogicMsg_m.h"
#include "qdp_veins/QdpAlgorithmCalculation.h"

namespace qdp_veins{

/**
 * Traffic light appl module.
 */
class QDP_VEINS_API QdpTrafficLightApp : public veins::DemoBaseApplLayer
{
public:
    void initialize(int stage) override;
    void finish() override;

protected:
    //Note: onWSM, onWSA not implemented
    /** @brief this function is called upon receiving a BasicSafetyMessage, also referred to as a beacon  */
    void onBSM(veins::DemoSafetyMessage* bsm) override;
    void handleSelfMsg(cMessage* msg) override;

private:
    /* Connection to QdpTrafficLightLogic */
    void instructLogicToStartPreemptionIn(double);
    void instructLogicToStopPreemption();
    QdpTrafficLightApplToLogicMsg* generateApplToLogicMsg(simtime_t, int);

    /* helper methods */
    bool isEvWayCurrentlyGreen();
    double getNewTG();
    double getActualPhaseTime();
    int getNumOfVehiclesFromDetectors();
    double computeDistanceToTraffiLightDownstream(veins::Coord);
    std::vector<int> initIndicesEvWay();

    /* interfacing */
    veins::TraCITrafficLightInterface* tlInterfaceAccess;
    veins::TraCICommandInterface* traci;
    veins::TimerManager timerManager {this};  // for updating the NumVehicles periodically

    /* values controlling App decision */
    std::string tl_id;
    std::string road_id;
    std::vector<std::string> e2_ids;
    veins::Coord tl_position;
    bool did_change;
    bool emveh_is_very_near;
    std::vector<int> tl_indices_ev_way;

    /* constant values controlling App decision */
    const double dist_close = 25.0;         // downstream distance to TL after which EV requests would not be processed [m]
    const double dist_away = 40.0;          // upstream distance from TL after which preemption stops [m]

    /* algorithm parameters - updated periodically */
    double TG;                              // GreenTime, value to be added to computed timeToStartPreemption [s]
    int NUM_VEHICLES;                       // max num of vehicles on given indices of given edge_id [-]

};
} // namespace qdp_veins
