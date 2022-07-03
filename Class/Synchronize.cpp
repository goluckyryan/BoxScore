#include "Synchronize.h"
#include "CAENDigitizer.h"

namespace BoxScore {

    void SetChainSynchronize(const std::vector<std::shared_ptr<Digitizer>>& chain)
    {
        int error_code;
        int current_handle;
        uint32_t runStartOnSIN = 0xD;
        uint32_t runDelayAddress = 0x8170;
        uint32_t registerValue = 0;
        //Start the run on the S-IN of the first digitizer in the chain
        error_code = CAEN_DGTZ_WriteRegister(chain[0]->GetDigitizerArgs().handle, CAEN_DGTZ_ACQ_CONTROL_ADD, runStartOnSIN);
        for(size_t i=0; i<chain.size(); i++)
        {
            current_handle = chain[i]->GetDigitizerArgs().handle;
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, 0); //Disable trigger propagation (freely triggering)
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, runDelayAddress, 2*i); //Delay run start to accommidate start propagation
            //Set RunStart to propagate over TrigOut; see SyncTest
            error_code |= CAEN_DGTZ_ReadRegister(current_handle, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, &registerValue);
            registerValue = registerValue & 0xfff0ffff | 0x00010000; //idk unabashedly stolen from SyncTest
            error_code |= CAEN_DGTZ_WriteRegister(current_handle, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, registerValue);
        }
    }

}