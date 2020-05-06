/*
 * ForwarderAppl.h
 *
 *  Created on: Apr 29, 2019
 *      Author: obrusvit
 */


#pragma once

#include "qdp_veins/qdp_veins.h"

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

namespace qdp_veins{


class QDP_VEINS_API ForwarderAppl : public veins::DemoBaseApplLayer
{

protected:
    void onBSM(veins::DemoSafetyMessage* bsm) override;
    void onWSM(veins::BaseFrame1609_4* wsm) override;
    void onWSA(veins::DemoServiceAdvertisment* wsa) override;

};

} //namespace qdp_veins
