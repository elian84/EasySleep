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
#include "EasySleep.h"


////////////////////////////////////////////////////////////////////////////////
//

// call back to user
static void (*cpu_sleep_cbk)(bool ) = 0;
static void (*cpu_deepsleep_cbk)(bool ) = 0;


////////////////////////////////////////////////////////////////////////////////
//

typedef enum
{
    BLE_SLEEP_,
    BLE_DEEPSLEEP_
} BLESleepT_;

typedef enum
{
    CPU_NOSLEEP_,
    CPU_SLEEP_,
    CPU_DEEPSLEEP_
} CPUSleepT_;

inline BLESleepT zip(BLESleepT_ ble, CPUSleepT_ cpu)
{
    if ( ble == BLE_SLEEP_ && cpu == CPU_NOSLEEP_ )       return BLE_SLEEP;
    if ( ble == BLE_DEEPSLEEP_ && cpu == CPU_NOSLEEP_ )   return BLE_DEEPSLEEP;
    if ( ble == BLE_SLEEP_ && cpu == CPU_SLEEP_ )         return BLE_SLEEP_CPU_SLEEP;
    if ( ble == BLE_DEEPSLEEP_ && cpu == CPU_SLEEP_ )     return BLE_DEEPSLEEP_CPU_SLEEP;
    if ( ble == BLE_DEEPSLEEP_ && cpu == CPU_DEEPSLEEP_ ) return BLE_DEEPSLEEP_CPU_DEEPSLEEP;

    // impossible combination; set best alternative
    if ( ble == BLE_SLEEP_ && cpu == CPU_DEEPSLEEP_ )     return BLE_SLEEP_CPU_SLEEP;
}

inline void unzip(BLESleepT sleep, BLESleepT_* ble, CPUSleepT_* cpu)
{
    if ( sleep == BLE_SLEEP )                     { *ble = BLE_SLEEP_;      *cpu = CPU_NOSLEEP_;   return; }
    if ( sleep == BLE_DEEPSLEEP )                 { *ble = BLE_DEEPSLEEP_;  *cpu = CPU_NOSLEEP_;   return; }
    if ( sleep == BLE_SLEEP_CPU_SLEEP )           { *ble = BLE_SLEEP_;      *cpu = CPU_SLEEP_;     return; }
    if ( sleep == BLE_DEEPSLEEP_CPU_SLEEP )       { *ble = BLE_DEEPSLEEP_;  *cpu = CPU_SLEEP_;     return; }
    if ( sleep == BLE_DEEPSLEEP_CPU_DEEPSLEEP )   { *ble = BLE_DEEPSLEEP_;  *cpu = CPU_DEEPSLEEP_; return; }
}


////////////////////////////////////////////////////////////////////////////////
//


void sleep_ble(BLESleepT request)
{

    ////////////////////////////////////////////////////////////////////////////////
    // references
    //
    //  - DfLPaEBLfBA         : "Designing for Low Power and Estimating Battery Life for BLE Applications"
    //  - CyBle_EnterLPM Doc  : API documentation for CyBle_EnterLPM()
    //  - CyBle_GetBleSsState : API documentation for CyBle_GetBleSsState()
    //

    ////////////////////////////////////////////////////////////////////////////////
    // CyBle_GetBleSsState():
    //
    //    CYBLE_BLESS_STATE_ACTIVE 
    //    CYBLE_BLESS_STATE_EVENT_CLOSE
    //    CYBLE_BLESS_STATE_SLEEP
    //    CYBLE_BLESS_STATE_ECO_ON
    //    CYBLE_BLESS_STATE_ECO_STABLE
    //    CYBLE_BLESS_STATE_DEEPSLEEP
    //    CYBLE_BLESS_STATE_HIBERNATE
    //    CYBLE_BLESS_STATE_INVALID
    //
    // CYBLE_LP_MODE_T:
    //    CYBLE_BLESS_ACTIVE 
    //    CYBLE_BLESS_SLEEP
    //    CYBLE_BLESS_DEEPSLEEP
    //    CYBLE_BLESS_HIBERNATE
    //    CYBLE_BLESS_INVALID
    //

    BLESleepT_ ble_req; CPUSleepT_ cpu_req;
    unzip( request, &ble_req, &cpu_req );

    CYBLE_LP_MODE_T ble_sleep_state;
    CYBLE_BLESS_STATE_T ble_state; 

    ////////////////////////////////////////////////////////////////////////////////
    // try to sleep BLE
    //

    ble_sleep_state = CYBLE_BLESS_ACTIVE;
    ble_state = CyBle_GetBleSsState();

    // see CyBle_GetBleSsState
    // added 14. july: _CLOSE, since doc CyBle_GetBleSsState is not very clear ("after")
    if ( ble_state == CYBLE_BLESS_STATE_ACTIVE      ||
         ble_state == CYBLE_BLESS_STATE_EVENT_CLOSE ||
         ble_state == CYBLE_BLESS_STATE_SLEEP       ||
         ble_state == CYBLE_BLESS_STATE_ECO_ON        )
    {

        // if request does not need BLE fast wakeup, try to put BLE into DeepSleep
        if ( ble_req == BLE_DEEPSLEEP_ )
        {
            ble_sleep_state = CyBle_EnterLPM( CYBLE_BLESS_DEEPSLEEP ); 
            // (when BLE wakes up, BLESS transitions to ECO_ON and ECO_STABLE before ACTIVE)

        }
        else
        {
            ble_sleep_state = CyBle_EnterLPM( CYBLE_BLESS_SLEEP ); 
        }

    }
    // see CyBle_GetBleSsState
    if ( ble_state == CYBLE_BLESS_STATE_ECO_STABLE )
    {
        ble_sleep_state = CyBle_EnterLPM( CYBLE_BLESS_SLEEP ); 
    }


    ////////////////////////////////////////////////////////////////////////////////
    // try to sleep CPU
    //

    if ( cpu_req == CPU_SLEEP_ || cpu_req == CPU_DEEPSLEEP_ )
    {
        // DfLPaEBLfBA 2.3 table 3 gives supported sleep modes
        // CyBle_GetBleSsState gives supported sleep modes

        // disabled interrupts during CPU sleep is required (otherwise BLE
        // can wakeup before sleep causing a non-wakeupable state (?)), see
        // http://www.cypress.com/comment/335891#comment-335891
        uint32_t sr = CyEnterCriticalSection();

        // we ignore 'ble_sleep_state' and instead use current BLE state
        ble_state = CyBle_GetBleSsState();

        // can we put CPU into DeepSleep?
        // NOTE: CyBle_GetBleSsState says CPU can be put to DeepSleep upon
        //       ECO_ON too, but that does not make sense since BLE is waking
        //       up then
        // added 14. july: added _HIBERNATE (doc CyBle_GetBleSsState)
        if ( ble_state == CYBLE_BLESS_STATE_DEEPSLEEP ||
             //ble_state == CYBLE_BLESS_STATE_ECO_ON // cf. NOTE
             ble_state == CYBLE_BLESS_STATE_HIBERNATE )
        {
            // do request allow CPU to be put in DeepSleep mode?
            if ( cpu_req == CPU_DEEPSLEEP_ )
            {
                // give user a chance to put components to sleep
                if ( cpu_deepsleep_cbk ) cpu_deepsleep_cbk( true );

                CySysPmDeepSleep();

                // (CPU is now awake. wake up sources: LP, Comparator, GPIO, 
                // CTBm, BLESS, WDT, SCB; see: DfLPaEBLfBA 2.2)
 
                if ( cpu_deepsleep_cbk ) cpu_deepsleep_cbk( false );

            }
            else
            {
                // give user a chance to put components to sleep
                if ( cpu_sleep_cbk ) cpu_sleep_cbk( true );
                
                CySysPmSleep();

                // (CPU is now awake. wake up sources: any IRQ; see: DfLPaEBLfBA 2.2)

                if ( cpu_sleep_cbk ) cpu_sleep_cbk( false );
            }
        }

        // see DfLPaEBLfBA 2.4
        // see CyBle_GetBleSsState
        if ( ble_state == CYBLE_BLESS_STATE_ECO_ON      ||
             ble_state == CYBLE_BLESS_STATE_ECO_STABLE     )  
        {
            // give user a chance to put components to sleep
            if ( cpu_sleep_cbk ) cpu_sleep_cbk( true );
                
            CySysPmSleep();

            // (CPU is now awake. wake up sources: any IRQ, see: DfLPaEBLfBA 2.2)

            if ( cpu_sleep_cbk ) cpu_sleep_cbk( false );
        }

        // see DfLPaEBLfBA 2.4
        // see CyBle_GetBleSsState
        if ( ble_state == CYBLE_BLESS_STATE_SLEEP   ||
             ble_state == CYBLE_BLESS_STATE_ACTIVE    )
        {
            
            // give user a chance to put components to sleep
            if ( cpu_sleep_cbk ) cpu_sleep_cbk( true );
                
            // set ECO frequency: 3 MHz, and set ECO as CPU clock (see DfLPaEBLfBA, 2.4)
            CySysClkWriteEcoDiv( CY_SYS_CLK_ECO_DIV8 );         // NOTE: ECO must not be CPU clock for this call
            CySysClkWriteHfclkDirect( CY_SYS_CLK_HFCLK_ECO );
            CySysClkImoStop();

            CySysPmSleep();

            // (CPU is now awake. wake up sources: any IRQ, see: DfLPaEBLfBA 2.2)

            CySysClkImoStart();
            CySysClkWriteHfclkDirect( CY_SYS_CLK_HFCLK_IMO );
            CySysClkWriteEcoDiv( CY_SYS_CLK_ECO_DIV1 );         // NOTE: ECO must not be CPU clock for this call
                                                                // FIXME: is ECO 24 MHz necessary?

            if ( cpu_sleep_cbk ) cpu_sleep_cbk( false );
        }
       

        // set back interrupts
        CyExitCriticalSection( sr );
    }

}


void wakeup_ble()
{
    CyBle_ExitLPM();
}


void cpu_sleep_callbacks(void (*sleep)(bool ), void (*deepsleep)(bool ))
{
    cpu_sleep_cbk = sleep;
    cpu_deepsleep_cbk = deepsleep;
}

