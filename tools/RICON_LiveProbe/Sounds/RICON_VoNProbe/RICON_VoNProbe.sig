AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "TransmissionQuality"
   tl -351.778 176.889
   children {
    6 7 14
   }
   value 1
  }
  IOPItemInputClass {
   id 18
   name "RICON_RadioOnlyProbeGain"
   tl -350 720
   children {
    19
   }
   value 1
  }
  IOPInputValueClass {
   id 9
   name "Vol Min [dB]"
   tl -346.364 378.182
   children {
    10
   }
   value -60
  }
  IOPInputValueClass {
   id 16
   name "Vol Min [dB]"
   tl -345.795 550
   children {
    17
   }
   value -12
  }
 }
 Ops {
  IOPItemOpInterpolateClass {
   id 6
   name "Interpolate 1"
   tl 36 95
   children {
    5
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "Y min" 0.85
   "Y max" 0.2
  }
  IOPItemOpInterpolateClass {
   id 7
   name "Interpolate 1"
   tl 39.111 282
   children {
    8
   }
   inputs {
    ConnectionClass "10:4" {
     id 10
     port 4
    }
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "X max" 0.8
   "Y min" 1
   "Fade In Type" "Power of 1/3"
   "Fade Out Type" "Power of 1/3"
  }
  SignalOpDb2GainClass {
   id 10
   name "Db2Gain 10"
   tl -157.273 378.182
   children {
    7
   }
   inputs {
    ConnectionClass "9:0" {
     id 9
     port 0
    }
   }
  }
  IOPItemOpInterpolateClass {
   id 14
   name "Interpolate 14"
   tl 41.528 477.639
   children {
    19
   }
   inputs {
    ConnectionClass "17:3" {
     id 17
     port 3
    }
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "Y min" 1
  }
  SignalOpDb2GainClass {
   id 17
   name "Db2Gain 10"
   tl -155.593 548.75
   children {
    14
   }
   inputs {
    ConnectionClass "16:0" {
     id 16
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 19
   name "RICON Radio Probe Multiply"
   tl 224 640
   children {
    12
   }
   inputs {
    ConnectionClass "14:0" {
     id 14
     port 0
    }
    ConnectionClass "18:0" {
     id 18
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 5
   name "Quality_W"
   tl 227 95
   input 6
  }
  IOPItemOutputClass {
   id 8
   name "Noise_V"
   tl 227 281
   input 7
  }
  IOPItemOutputClass {
   id 12
   name "Radio_V"
   tl 227 477.287
   input 19
  }
 }
 Input_Order {
  ItemDetailListItemClass TransmissionQuality {
   Name "TransmissionQuality"
   Id 1
  }
  ItemDetailListItemClass RICON_RadioOnlyProbeGain {
   Name "RICON_RadioOnlyProbeGain"
   Id 18
  }
 }
 Output_Order {
  ItemDetailListItemClass Quality_W {
   Name "Quality_W"
   Id 5
  }
  ItemDetailListItemClass Noise_V {
   Name "Noise_V"
   Id 8
  }
  ItemDetailListItemClass Radio_V {
   Name "Radio_V"
   Id 12
  }
 }
}