{--
  Haskell Copilot support for Hackflight

  Copyright(C) 2021 Simon D.Levy

  MIT License
--}

{-# LANGUAGE RebindableSyntax #-}
{-# LANGUAGE DataKinds        #-}

module Main where

import Language.Copilot
import Copilot.Compile.C99

import Demands
import VehicleState(initialVehicleState)
import Sensor

receiverDemands :: Stream (Array 4 Double)
receiverDemands  = extern "receiverDemands" Nothing

gyrometerValues :: Stream (Array 3 Double)
gyrometerValues  = extern "gyrometerValues" Nothing

motorValues :: Stream (Array 4 Double)
motorValues = [array [0, 0, 0, 0]] ++ motorValues

spec = do

  let initialDemands = Demands (receiverDemands .!!0)
                               (receiverDemands .!!1)
                               (receiverDemands .!!2)
                               (receiverDemands .!!3)

  let gyrometer = Gyrometer (gyrometerValues .!!0)
                            (gyrometerValues .!!1)
                            (gyrometerValues .!!2)

  let newMotorValues = [array [0.1, 0.2, 0.3, 0.4]] ++ motorValues

  trigger "runMotors" true [arg newMotorValues]

-- Compile the spec
main = reify spec >>= compile "hackflight"
