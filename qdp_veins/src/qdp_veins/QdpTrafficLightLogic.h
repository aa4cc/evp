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
#include "veins/modules/world/traci/trafficLight/logics/TraCITrafficLightAbstractLogic.h"
#include "veins/modules/world/traci/trafficLight/TraCITrafficLightInterface.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include "veins/modules/utility/TimerManager.h"
#include "veins/modules/world/traci/trafficLight/TraCITrafficLightProgram.h"
#include "qdp_veins/QdpTrafficLightApplToLogicMsg_m.h"


namespace qdp_veins{

/**
 * Traffic light logic module.
 */
class QDP_VEINS_API QdpTrafficLightLogic : public veins::TraCITrafficLightAbstractLogic
{
public:
    QdpTrafficLightLogic();
    ~QdpTrafficLightLogic();

protected:
    void initialize() override;
    void handleApplMsg(cMessage* msg) override;
    void handleTlIfMsg(veins::TraCITrafficLightMessage* tlMsg) override;
    void handlePossibleSwitch() override;
    void handleSelfMsg(cMessage* msg) override;

private:

    enum class PreemptionRequestSource {
        EMERGENCY,
        TL_LOGIC
    };

    /* interfacing */
    veins::TraCITrafficLightInterface* tlInterfaceAccess;
    veins::TraCICommandInterface* traci;
    veins::TimerManager timerManager {this};  // for scheduling start of preemption

    /* fields needed to control program switching*/
    std::string tl_id;
    veins::TimerManager::TimerHandle start_pref_msg;
    int old_phase_num;              // to save index phase from which we switched to preemption
    std::string old_program_id;     // used to save program id from which we switched to preemption
    bool ev_should_stop_asking;
    bool tl_switched_back;          // preemption is over, EV is away, do not switch to old_phase again


    void startPreemption(const std::vector<int>&);
    void switchProgramBack();
    void scheduleStartPreemptionMsg(PreemptionRequestSource, simtime_t, const std::vector<int>&);

    /* traffic light program helpers */
    bool isPreemptionAllowed(std::string program_id, int phase_id);
    simtime_t getActualPhaseTime();
    std::vector<int> getIndicesOfGreensFromPhase(std::string);
    std::string getDesiredPhase(const std::vector<int>&, int);

    veins::TraCITrafficLightProgram::Logic createPreemptionProgram(std::string, std::string, std::vector<int>);

};

} //namespace qdp_veins
