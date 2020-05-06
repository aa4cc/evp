/*
 * ForwarderAppl.cc
 *
 *  Created on: Apr 29, 2019
 *      Author: obrusvit
 */

#include "qdp_veins/ForwarderAppl.h"
#include "qdp_veins/EmergSafetyMessage_m.h"

using namespace veins;
using namespace qdp_veins;

Define_Module(qdp_veins::ForwarderAppl);

void ForwarderAppl::onBSM(DemoSafetyMessage* bsm) {
    // decrement hopCount and re-send if hopCount >= 0
    findHost()->getDisplayString().setTagArg("i", 1, "green");
    if (EmergSafetyMessage* msg = dynamic_cast<EmergSafetyMessage*>(bsm)){
        msg->setHopCount(msg->getHopCount()-1);
        if (msg->getHopCount() >= 0)
            sendDelayedDown(msg->dup(), uniform(0.01, 0.2));
    }
}

void ForwarderAppl::onWSM(BaseFrame1609_4* wsm) {
}

void ForwarderAppl::onWSA(DemoServiceAdvertisment* wsa) {
}
