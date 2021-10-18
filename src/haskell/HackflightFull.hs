{--
  Hackflight core algorithm

  Copyright(C) 2021 on D.Levy

  MIT License
--}

{-# LANGUAGE RebindableSyntax #-}

module HackflightFull where

import Language.Copilot

import Prelude hiding((!!), (||), (++), (<), (>), (&&), (==), div, mod, not)

import Receiver
import State
import Sensor
import Demands
import PidController
import Safety
import Time
import Mixer
import Utils

hackflight :: Receiver -> [Sensor] -> [PidFun]
  -> (State, SBool, SBool, Demands, Motors, SBool)

hackflight receiver sensors pidfuns = (state, mrunning, mzero , pdemands, motors, led)

  where

    -- Get receiver demands from external C functions
    rdemands = getDemands receiver

    -- Get the vehicle state by composing the sensor functions over the current state
    state = compose sensors (state' state)

    -- Periodically get the demands by composing the PID controllers over the receiver
    -- demands
    (_, _, pdemands) = compose pidfuns (state, timerReady 300, rdemands)

    -- Check safety (arming / failsafe)
    (armed, failsafe, mrunning, mzero) = safety rdemands state

    -- Run mixer on demands to get motor values
    motors = mix pdemands

    -- Blink LED during first couple of seconds; keep it solid when armed
    led = if micros < 2000000 then (mod (div micros 50000) 2 == 0) else armed
