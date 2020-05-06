/*
 * QdpAlgorithmCalculation.cc
 *
 *  Created on: May 2, 2020
 *      Author: obrusvit
 */
#include "qdp_veins/QdpAlgorithmCalculation.h"

namespace qdp_veins{

double linApproxQueueDischarge(int num_vehicles, double t){
    //constant 1.5 is approximate fit
    //return std::max(0.0, (num_vehs+1.5)-q_ns*t);
    return (num_vehicles+1.5)-q_ns*t;
}

double calc_TA(double distance, double speed){
    return distance / speed; //how long till emergency approaches traffic light
}

double calc_TL(int num_vehicles){
    // Formula:  T_L = num_vehicles*t_x + t_a
    // ternary op prevents zero cars with t_a
    auto TL =  (num_vehicles > 0) ? (num_vehicles * t_x + t_a) : 0;
    return TL;
}

double calc_TX(int num_vehicles, double TL, double speed){
    // get length of queue in time T_L
    auto DX = linApproxQueueDischarge(num_vehicles, TL) * L_hn;
    // get time needed to pass DX
    return DX / speed;
}

double expDelay(double time) {
    auto max_cars = time * q_ns;
    max_cars += 0.5;
    auto max_TL = calc_TL(max_cars);
    auto max_TX = calc_TX(max_cars, max_TL);
    return max_TL + max_TX;
}

} // namespace qdp_veins
