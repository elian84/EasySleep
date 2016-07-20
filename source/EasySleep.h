// The MIT License (MIT)
// 
// Copyright (c) 2016 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef PSOC4BLE_EASYSLEEP_H
#define PSOC4BLE_EASYSLEEP_H
#include <project.h>
#include <stdint.h>
#include <stdbool.h>


////////////////////////////////////////////////////////////////////////////////////////
//                                                                                    //
// EasySleep (PSoC4BLE)                                                               //
//                                                                                    //
// "At least BLE should be put to sleep at end of each CyBle_ProcessEvents() frame"   //
//    - old chinese proverb                                                           //
//                                                                                    //
// Created by speedycat                                                               //
//                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// references
//
//  - DfLPaEBLfBA         : "Designing for Low Power and Estimating Battery Life for BLE Applications"
//  - CyBle_EnterLPM Doc  : API documentation for CyBle_EnterLPM()
//  - CyBle_GetBleSsState : API documentation for CyBle_GetBleSsState()
//


// wakeup times taken from DfLPaEBLfA 2.2 table 2
typedef enum
{
    BLE_SLEEP,                    // try to sleep BLE, but not CPU.
                                  // BLE Sleep or Active
                                  // BLE wakeup time <= X s (fast) // FIXME

    BLE_DEEPSLEEP,                // try to sleep BLE, but not CPU.
                                  // BLE DeepSleep or Sleep or Active
                                  // BLE wakeup time <= XX s // FIXME 0.002

    BLE_SLEEP_CPU_SLEEP,          // try to sleep BLE, try to sleep CPU.
                                  // BLE Sleep or Active
                                  // CPU Sleep or Active
                                  // BLE wakeup time <= X s (fast) // FIXME
                                  // CPU wakeup time <= 0 s (fast (??)) 

    BLE_DEEPSLEEP_CPU_SLEEP,      // try to sleep BLE, try to sleep CPU. 
                                  // BLE DeepSleep or Sleep or Active
                                  // BLE wakeup time <= XX s // FIXME 0.002
                                  // CPU wakeup time <= 0 s (fast (??)) // FIXME

    BLE_DEEPSLEEP_CPU_DEEPSLEEP   // try to sleep BLE, try to sleep CPU. 
                                  // BLE DeepSleep or Sleep or Active
                                  // CPU DeepSleep or Sleep or Active
                                  // BLE wakeup time <= XX s // FIXME 0.002
                                  // CPU wakeup time <= 0.000025 s

} BLESleepT;



// try to sleep BLE (DeepSleep or Sleep), and also CPU if requested
// CPU may actually sleep but not BLE. 
// the actual blocking time will be a little longer than the value given above.
//
void sleep_ble(BLESleepT );


// wait for BLE to wake up
//
void wakeup_ble();


// set callbacks for CPU sleep/wakup, 
// for example put components other than BLE to sleep.
// these callbacks are _not_ allowed to enable interrupts!
//
void cpu_sleep_callbacks(void (*sleep)(bool ), void (*deepsleep)(bool ));


#endif


