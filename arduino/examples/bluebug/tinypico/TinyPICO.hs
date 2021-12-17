{--
  TinyPICO Support for Bluebug flight controller

  Copyright(C) 2021 on D.Levy

  MIT License
--}

{-# LANGUAGE RebindableSyntax #-}

module TinyPICO where

import Language.Copilot
import Copilot.Compile.C99

import Time

-- Comms
import Bluetooth
import Serial
import Parser

-- Misc
import Utils

------------------------------------------------------------

getChannel :: SWord8 -> SWord8 -> SWord8 -> SWord16 -> SWord16

getChannel byte payindex tgtindex chanval' = chanval where
  chanval = if payindex == k then cast byte
            else if payindex == (k+1) then chanval' .|. ((cast byte) .<<. s8)
            else chanval'
                where k = tgtindex * 2 - 1
                      s8 = 8 :: SWord8

------------------------------------------------------------

spec = do

  -- Get flags for startup, loop
  let (running, starting) = runstate

  -- Shorthand
  let avail = stream_bluetoothAvailable
  let byte = stream_bluetoothByte

  -- Run the serial comms parser
  let (msgtype, _, payindex, checked) = parse avail byte

  -- Get channel values
  let chan1 = getChannel byte payindex 1 chan1' where chan1' = [0] ++ chan1 
  let chan2 = getChannel byte payindex 2 chan2' where chan2' = [0] ++ chan2 
  let chan3 = getChannel byte payindex 3 chan3' where chan3' = [0] ++ chan3 
  let chan4 = getChannel byte payindex 4 chan4' where chan4' = [0] ++ chan4 
  let chan5 = getChannel byte payindex 5 chan5' where chan5' = [0] ++ chan5 
  let chan6 = getChannel byte payindex 6 chan6' where chan6' = [0] ++ chan6 

  -- Do some stuff at startup
  trigger "stream_startSerial" starting []
  trigger "stream_startBluetooth" starting []

  -- Do some other stuff in loop
  -- trigger "stream_updateTime" running []
  trigger "stream_bluetoothUpdate" running []
  trigger "stream_bluetoothRead" (running && stream_bluetoothAvailable) []

  trigger "stream_debug" checked [  arg chan1
                                  , arg chan2
                                  , arg chan3
                                  , arg chan4
                                  , arg chan5
                                  , arg chan6]

-- Compile the spec
main = reify spec >>= compile "hackflight"

