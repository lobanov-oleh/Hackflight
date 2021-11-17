#include <stdio.h>
#include <Dynamics.hpp>

Dynamics::vehicle_params_t vparams = {

    // Estimated
    2.E-06, // d drag cofficient [T=d*w^2]

    // https://www.dji.com/phantom-4/info
    1.380,  // m mass [kg]

    // Estimated
    2,      // Ix [kg*m^2] 
    2,      // Iy [kg*m^2] 
    3,      // Iz [kg*m^2] 
    3.8E-03, // Jr prop inertial [kg*m^2] 
    15000,   // maxrpm
};

Dynamics::fixed_pitch_params_t fpparams = {
    5.E-06, // b thrust coefficient [F=b*w^2]
    0.350   // l arm length [m]
};

int main(int argc, char ** argv)
{
    Dynamics dynamics = Dynamics(vparams, fpparams);

    float motors[4] = {0, 0, 0, 0};

    Dynamics::state_t state = {};

    for (int k=0; k<1000; ++k) {

        float t = k / 1000.;

        printf("%f\n", t);
    }

    return 0;
}
