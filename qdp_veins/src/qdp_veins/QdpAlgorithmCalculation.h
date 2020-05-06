/*
 * QdpAlgorithmCalculation.h
 *
 *  Created on: May 2, 2020
 *      Author: obrusvit
 */

#ifndef QDP_VEINS_QDPALGORITHMCALCULATION_H_
#define QDP_VEINS_QDPALGORITHMCALCULATION_H_

namespace qdp_veins{

const double desired_speed = 70/3.6;    // desired speed of emergency vehicle [m/s]
const double t_x = 1.22;                // average reaction time of a driver [s]
const double t_a = 5.82;                // average time to accelerate to saturation speed [s]
const double L_hn = 18.7;               // saturation gap between vehicles [m]
const double q_ns = 0.5208;             // vehicle discharge speed [veh/s]
const double t_sw = 11.0;               // minimal time necessary to get back to the current phase if the phase is switched at this very moment [s]
const double t_min = 10.0;              // minimal time of green phase (green on main road cannot be shorter than t_min) [s]
const double t_conservative = 0.0;      // time added to preference start, to make it more conservative [s]

double linApproxQueueDischarge(int num_vehicles, double t);
double calc_TA(double distance, double speed = desired_speed);
double calc_TL(int num_vehicles);
double calc_TX(int num_vehicles, double TL, double speed = desired_speed);
double expDelay(double time);  // expected delay created by additional cycle time

} //namespace qdp_veins


#endif /* QDP_VEINS_QDPALGORITHMCALCULATION_H_ */
