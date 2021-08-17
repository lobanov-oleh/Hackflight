/*
  Socket code for working with Haskell Copilot version of Hackflight via
  simulator

  Copyright(C) 2021 Simon D.Levy

  MIT License
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "udp_sockets.h"
#include "hackflight.h"

static const char * HOST = "127.0.0.1";
static const uint16_t MOTOR_PORT = 5000;
static const uint16_t TELEMETRY_PORT = 5001;

// Avaiable to Copilot
double receiverThrottle = 0;
double receiverRoll = 0;
double receiverPitch = 0;
double receiverYaw = 0;

double simSensorZ = 0;
double simSensorDz = 0;

// Shared by main() and runMotors()
static udp_socket_t motor_client_socket; 

// Called by Copilot
void runMotors(double m1, double m2, double m3, double m4)
{
    double values[4] = {m1, m2, m3, m4};
    udp_send_data(motor_client_socket, values, 4*sizeof(double));
}

// For debugging
void showVehicleState(double z, double dz)
{
    printf("z: %+3.3f  dz: %+3.3f\n", z, dz);
}

int main (int argc, char *argv[])
{
    udp_client_socket_init(&motor_client_socket, HOST, MOTOR_PORT, 0);

    udp_socket_t telemetry_server_socket = {};
    udp_server_socket_init(&telemetry_server_socket, TELEMETRY_PORT, 0);

    printf("Hit the start button ...\n");
    fflush(stdout);

    while (true) {

        double telemetry_data[17] = {};

        if (udp_receive_data(
                    telemetry_server_socket,
                    telemetry_data,
                    sizeof(telemetry_data))) {

            simSensorZ = telemetry_data[5];
            simSensorDz = telemetry_data[6];

            receiverThrottle = telemetry_data[13];
            receiverRoll = telemetry_data[14];
            receiverPitch = telemetry_data[15];
            receiverYaw = telemetry_data[16];

            udp_set_timeout(telemetry_server_socket, 100);

            // Calls Copilot
            step();
        }

        else {

            exit(0);
        }
    }

    return 0;
}
