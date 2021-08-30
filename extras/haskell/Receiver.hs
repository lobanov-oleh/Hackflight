{--
  Haskell Copilot support for RC receivers

  Copyright(C) 2021 on D.Levy

  MIT License
--}

{-# LANGUAGE RebindableSyntax #-}

module Receiver where

import Language.Copilot
import Copilot.Compile.C99

import Demands

data AxisTrim = AxisTrim {  rollTrim :: Stream Float
                          , pitchTrim :: Stream Float
                          , yawTrime :: Stream Float
                         } deriving (Show)

data ChannelMap = ChannelMap {  throttleChannel :: Stream Float
                              , rollChannel :: Stream Float
                              , pitchChannel :: Stream Float
                              , yawChannel :: Stream Float
                              , aux1Channel :: Stream Float
                              , aux2Channel :: Stream Float
                             } deriving (Show)

data Receiver = Receiver {  throttleMargin :: Stream Float
                          , throttleExpo :: Stream Float
                          , cyclicExpo :: Stream Float
                          , cyclicRate :: Stream Float
                          , auxTheshold :: Stream Float
                          , demandScale :: Stream Float
                          , channelMap :: ChannelMap
                          , axisTrim :: AxisTrim
                         } deriving (Show)

makeReceiverWithTrim :: ChannelMap -> AxisTrim -> Stream Float -> Receiver
makeReceiverWithTrim channelMap axisTrim demandScale =
    Receiver 0.10 -- throttleMargin
             0.20 -- throttleExpo
             0.90 -- cyclicRate
             0.65 -- cyclicExpo
             0.40 -- auxThreshold
             demandScale
             channelMap
             axisTrim

makeReceiver :: ChannelMap -> Stream Float -> Receiver
makeReceiver channelMap demandScale =
  makeReceiverWithTrim channelMap (AxisTrim 0 0 0) demandScale

getDemands :: Receiver -> Demands
getDemands receiver = 

    Demands 0 rollDemand 0 0
    where
      rollDemand = (rollChannel (channelMap receiver))
       

receiverReady ::  Stream Bool
receiverReady = receiverGotNewFrame

-- Externals -------------------------------------------------

chan1 :: Stream Float
chan1  = extern "copilot_receiverChannel1" Nothing

chan2 :: Stream Float
chan2  = extern "copilot_receiverChannel2" Nothing

chan3 :: Stream Float
chan3  = extern "copilot_receiverChannel3" Nothing

chan4 :: Stream Float
chan4  = extern "copilot_receiverChannel4" Nothing

chan5 :: Stream Float
chan5  = extern "copilot_receiverChannel5" Nothing

chan6 :: Stream Float
chan6  = extern "copilot_receiverChannel6" Nothing

chan7 :: Stream Float
chan7  = extern "copilot_receiverChannel7" Nothing

chan8 :: Stream Float
chan8  = extern "copilot_receiverChannel8" Nothing

receiverLostSignal :: Stream Bool
receiverLostSignal  = extern "copilot_receiverLostSignal" Nothing

receiverGotNewFrame :: Stream Bool
receiverGotNewFrame  = extern "copilot_receiverGotNewFrame" Nothing
