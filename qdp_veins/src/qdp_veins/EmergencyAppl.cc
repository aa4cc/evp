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

#include "qdp_veins/EmergencyAppl.h"
#include "qdp_veins/EmergSafetyMessage_m.h"

using namespace veins;
using namespace qdp_veins;
Define_Module(qdp_veins::EmergencyAppl);


void EmergencyAppl::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        // Initializing members and pointers of your application goes here
        EV << "Initializing " << par("appName").stringValue() << std::endl;
        mobility = TraCIMobilityAccess().get(getParentModule());
        traci = mobility->getCommandInterface();
    }
    else if (stage == 1) {
        // Initializing members that require initialized other modules goes here
    }
}

void EmergencyAppl::finish()
{
    DemoBaseApplLayer::finish();
    // statistics recording goes here
}

void EmergencyAppl::onBSM(DemoSafetyMessage* bsm)
{
    // Your application has received a beacon message from another car or RSU
    // code for handling the message goes here

    EV << "EmergencyAppl onBSM\n";
}

void EmergencyAppl::onWSM(BaseFrame1609_4* wsm)
{
    // Your application has received a data message from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples

    EV << "EmergencyAppl onWSM \n";

}

void EmergencyAppl::onWSA(DemoServiceAdvertisment* wsa)
{
    // Your application has received a service advertisement from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples
    EV << "EmergencyAppl onWSA\n";
}

void EmergencyAppl::handleSelfMsg(cMessage* msg)
{
    // this method is for self messages (mostly timers)
    // it is important to call the DemoBaseApplLayer function for BSM and WSM transmission
    if (msg->getKind() == SEND_BEACON_EVT){
        // replace SEND_BEACON_EVT case to use EmergSafetyMessage
        EmergSafetyMessage* esm = new EmergSafetyMessage();
        populateWSM(esm);
        sendDown(esm);
        scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void EmergencyAppl::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    // the vehicle has moved. Code that reacts to new positions goes here.
    // member variables such as currentPosition and currentSpeed are updated in the parent class
}
