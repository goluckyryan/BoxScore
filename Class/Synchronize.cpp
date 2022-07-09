#include "Synchronize.h"
#include "CAENDigitizer.h"

/*
    Logic (approximate):
    Set the digitizers to propogate synchronization, and all 
*/

namespace BoxScore {

    static double GetSamplingPeriod(CAEN_DGTZ_BoardModel_t model)
    {
        switch(model)
        {
            case CAEN_DGTZ_V1730: return 2.0; //ns -> 500 MSamples/s
            case CAEN_DGTZ_V1725: return 4.0; //ns -> 250 MSamples/s
        }

        return 4.0; //Idk?
    }

    int SetChainSynchronize(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain)
    {
        static uint32_t runStartOnSIN = 0xD;
        static uint32_t runDelayAddress = 0x8170;

        if(args.syncMethod != SyncMethod::SIn_TrigOut)
        {
            //report unsupported method
            return 0;
        }
        int error_code;
        int current_handle;

        std::vector<double> samplingPeriods;
        double delay = 0.0;
        for(auto& digi : chain)
        {
            samplingPeriods.push_back(GetSamplingPeriod(digi->GetDigitizerArgs().model));
            delay += GetSamplingPeriod(digi->GetDigitizerArgs().model);
        }
        
        uint32_t registerValue = 0;
        //Set all digitizers except the master to start on S-IN level propogated value
        for(size_t i=1; i<chain.size(); i++)
            error_code = CAEN_DGTZ_WriteRegister(chain[i]->GetDigitizerArgs().handle, CAEN_DGTZ_ACQ_CONTROL_ADD, runStartOnSIN);
        for(size_t i=0; i<chain.size(); i++)
        {
            current_handle = chain[i]->GetDigitizerArgs().handle;
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, 0); //Disable trigger propagation (freely triggering)
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, runDelayAddress, delay); //Delay run start to accommidate start propagation
            //Set RunStart to propagate over TrigOut; see SyncTest
            error_code |= CAEN_DGTZ_ReadRegister(current_handle, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, &registerValue);
            registerValue = registerValue & 0xfff0ffff | 0x00010000; //idk unabashedly stolen from SyncTest
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, registerValue);

            //Shorten the delay to move to next board
            delay -= samplingPeriods[i];
        }

        return error_code;
    }

    int StartSynchronizedRun(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain)
    {
        static uint32_t softwareTrigger = 0x4;
        static uint32_t hardwareLevelTrigger = 0x5;

        if(args.syncMethod != SyncMethod::SIn_TrigOut)
        {
            //report unsupported method
            return 0;
        }

        int error_code = 0;
        switch(args.startType)
        {
            case SyncStartType::HardwareControlled:
            {
                error_code |= CAEN_DGTZ_WriteRegister(chain[0]->GetDigitizerArgs().handle, CAEN_DGTZ_ACQ_CONTROL_ADD, softwareTrigger);
                break;
            }
            case SyncStartType::SoftwareControlled:
            {
                error_code |= CAEN_DGTZ_WriteRegister(chain[0]->GetDigitizerArgs().handle, CAEN_DGTZ_ACQ_CONTROL_ADD, hardwareLevelTrigger);
                break;
            }
        }

        return error_code;
    }

    int StopSynchronizedRun(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain)
    {
        if(args.syncMethod != SyncMethod::SIn_TrigOut)
        {
            //report unsupported method
            return 0;
        }

        return CAEN_DGTZ_WriteRegister(chain[0]->GetDigitizerArgs().handle, CAEN_DGTZ_ACQ_CONTROL_ADD, 0); //Seems pretty universal, for all methods 0 indicates stop
    }
}