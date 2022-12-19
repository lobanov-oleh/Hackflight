/*
   Hackflight PID controller support

   Copyright (C) 2022 Simon D. Levy

   MIT License
*/

pub mod newanglepid {

    use crate::datatypes::Demands;
    use crate::datatypes::VehicleState;
    use crate::newpids::newpids;

    pub struct Pid {
        k_p : f32,
        k_i: f32, 
        in_band_prev: bool,
        error_integral: f32,
        altitude_target: f32
    }

    pub fn make(k_p : f32, k_i: f32) -> Pid {

        Pid {
            k_p: k_p,
            k_i: k_i,
            in_band_prev: false,
            error_integral: 0.0,
            altitude_target: 0.0
        }
    }

    impl newpids::PidControllerTrait for Pid {

        fn get_demands_trait(
            &self,
            d_usec: &u32,
            demands: &Demands,
            vstate: &VehicleState,
            reset: &bool) -> Demands {

                Demands { throttle: 0.0, roll: 0.0, pitch: 0.0, yaw: 0.0 }
        }
 
    }

} 
