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

#include "qdp_veins/QdpTrafficLightLogic.h"

using namespace veins;
using namespace qdp_veins;

Define_Module(qdp_veins::QdpTrafficLightLogic);

QdpTrafficLightLogic::QdpTrafficLightLogic()
    : TraCITrafficLightAbstractLogic()
{
}

QdpTrafficLightLogic::~QdpTrafficLightLogic()
{
}

void QdpTrafficLightLogic::initialize() {
    TraCITrafficLightAbstractLogic::initialize();
    tlInterfaceAccess = TraCITrafficLightInterfaceAccess().get(getParentModule());
    traci = tlInterfaceAccess->getCommandInterface();
    tl_id = tlInterfaceAccess->getExternalId();

    old_phase_num = 0;
    old_program_id = "";
    ev_should_stop_asking = false;
    tl_switched_back = false;
}


void QdpTrafficLightLogic::handleApplMsg(cMessage* msg) {
    auto* applMsg = check_and_cast<QdpTrafficLightApplToLogicMsg*>(msg);
    auto tl_id = applMsg->getTlId();
    auto msg_type = applMsg->getApplRequestType();
    switch (msg_type) {
        case START_PREEMPTION: {
            auto TP = std::max(0.0, applMsg->getTimeToStartPref().dbl());
            auto indices = applMsg->getIndices();
            if (!ev_should_stop_asking)
                scheduleStartPreemptionMsg(PreemptionRequestSource::EMERGENCY, TP, indices);
            break;
        }
        case STOP_PREEMPTION: {
            switchProgramBack();
            break;
        }
        default: {
            EV_WARN << "QdpTrafficLightLogic: LOGIC: Error, got message from APPL of unknown type";
        }
    }
    delete msg;
}

void QdpTrafficLightLogic::handleTlIfMsg(TraCITrafficLightMessage* tlMsg) {
    delete tlMsg;
}

void QdpTrafficLightLogic::handlePossibleSwitch() {
}

void QdpTrafficLightLogic::handleSelfMsg(cMessage* msg) {
    if (timerManager.handleMessage(msg))
        return;
    TraCITrafficLightAbstractLogic::handleSelfMsg(msg);
}

void QdpTrafficLightLogic::startPreemption(const std::vector<int>& ev_indices) {
    // obtain data from TraCI
    int phase_id = tlInterfaceAccess->getCurrentPhaseId();
    std::string program_id = tlInterfaceAccess->getCurrentLogicId();
    std::string current_phase = tlInterfaceAccess->getCurrentState();

    if( ! isPreemptionAllowed(program_id, phase_id)){
        // let logic ask again in a second
        scheduleStartPreemptionMsg(PreemptionRequestSource::TL_LOGIC, 1.0, ev_indices);
        return;
    }

    //create program that will get us to desired (preference) phase
    auto desired_phase = getDesiredPhase(ev_indices, current_phase.length());
    auto new_program = createPreemptionProgram(current_phase, desired_phase, ev_indices);

    //set this new program as used
    traci->trafficlight(tl_id).setProgramDefinition(new_program, 10); //TODO what is 10
    traci->trafficlight(tl_id).setPhaseIndex(0);

    //save old values
    old_program_id = program_id;
    old_phase_num = phase_id;
}

void QdpTrafficLightLogic::switchProgramBack(){
    if (!tl_switched_back){
        tl_switched_back = true;
        tlInterfaceAccess->setCurrentLogicById(old_program_id);
        tlInterfaceAccess->setCurrentPhaseByNr(old_phase_num);
    }
}

void QdpTrafficLightLogic::scheduleStartPreemptionMsg(PreemptionRequestSource kind, simtime_t TP, const std::vector<int>& indices){
    timerManager.cancel(start_pref_msg);
    auto timerToStartPreemption = [this, kind, indices](){
        if (kind == PreemptionRequestSource::EMERGENCY){
            this->ev_should_stop_asking = true;
            // NOTE: log start of preemption request here potentially (distance, time,..)
        }
        startPreemption(indices);
    };
    auto oneOffSpecification = TimerSpecification(timerToStartPreemption).oneshotIn(TP);
    start_pref_msg = timerManager.create(oneOffSpecification, "one off timer to start preemption");
}

bool QdpTrafficLightLogic::isPreemptionAllowed(std::string program_id, int phase_id){
    auto phase_time = getActualPhaseTime();
    if (phase_time >= 5){
        return true;
    }

    // find which indices (lanes) are now in green state
    std::vector<int> ind_green = getIndicesOfGreensFromPhase(tlInterfaceAccess->getCurrentPhase().state);

    // iterate over past phases to find out whether current greens are green for at least 5s
    auto total_green_time = phase_time;
    while (total_green_time <= 5.0){
        --phase_id;
        if (phase_id == -1)
            phase_id = tlInterfaceAccess->getCurrentLogic().phases.size()-1;
        //iterate over previous state and look for 'g' or 'G' on ind_green indices
        auto prev_phase = tlInterfaceAccess->getCurrentLogic().phases[phase_id];
        for (int ind = 0; ind < prev_phase.state.length(); ind++)
            if (std::find(ind_green.begin(), ind_green.end(), ind) != ind_green.end())
                if (prev_phase.state[ind] != 'g' and prev_phase.state[ind] != 'G')
                    return false;
        total_green_time += prev_phase.duration;
    }
    // we got past the while loop, that means green was at least 5s
    return true;
}

simtime_t QdpTrafficLightLogic::getActualPhaseTime() {
    auto remaining_phase_time = tlInterfaceAccess->getRemainingDuration();
    auto phase_duration = tlInterfaceAccess->getCurrentPhase().duration;
    return phase_duration - remaining_phase_time;
}

std::vector<int> QdpTrafficLightLogic::getIndicesOfGreensFromPhase(std::string phase){
    std::vector<int> ret;
    int i = 0;
    for(auto& s : phase){
        if (s=='g' || s=='G')
            ret.push_back(i);
        ++i;
    }
    return ret;
}

TraCITrafficLightProgram::Logic QdpTrafficLightLogic::createPreemptionProgram(
        std::string current_phase, std::string desired_phase,
        std::vector<int> indices)
{
    using Phase = TraCITrafficLightProgram::Phase;
    using Logic = TraCITrafficLightProgram::Logic;

    Phase phase_final{1000.0, desired_phase, 1000.0, 1000.0, {1}, "phase_final"};

    // define transition phase
    int phase_str_len = current_phase.length();
    std::string transition_phase(phase_str_len, 'r');

    // iterate first time over i not in indices
    for (int i = 0; i < phase_str_len; i++){
        if(std::find(indices.begin(), indices.end(), i) == indices.end()){
            if(current_phase[i] == 'g' || current_phase[i] == 'G'){
                transition_phase[i] = 'y';
            } else {
                transition_phase[i] = 'r';
            }
        }
    }
    // iterate second time over i in indices
    for (int i = 0; i < phase_str_len; i++){
        if(std::find(indices.begin(), indices.end(), i) != indices.end()){
            if(current_phase[i] == 'g' || current_phase[i] == 'G'){
                transition_phase[i] = 'G';
            } else {
                if (std::find(transition_phase.begin(), transition_phase.end(), 'y') != transition_phase.end()){
                    transition_phase[i] = 'r';
                } else{
                    transition_phase[i] = 'G';
                }
            }
        }
    }

    std::vector<Phase> phases;
    if (transition_phase != desired_phase){
        Phase phase_transition{3.0, transition_phase, 3.0, 3.0, {1}, "phase_trans"};
        phases.push_back(phase_transition);
    }
    phases.push_back(phase_final);

    //create final program
    Logic new_program{tl_id+"_preemption", 0, phases, 0, 0};
    return new_program;
}

std::string QdpTrafficLightLogic::getDesiredPhase(const std::vector<int>& indices, int len){
    std::string ret(len, 'r');
    for(auto i : indices){
        ret[i] = 'G';
    }
    return ret;
}
