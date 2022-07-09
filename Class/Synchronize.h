#ifndef SYNCHRONIZE_H
#define SYNCHRONIZE_H

#include "Digitizer.h"
#include <memory>

/*
    Here we show how to synchronize in the case of propagating an external start across a digitizer chain
    and then allowing each board to trigger freely. There are other types of synchronization, as shown in the
    CAEN SyncTest examples.
*/

namespace BoxScore {

    enum class SyncStartType
    {
        HardwareControlled,
        SoftwareControlled
    };

    //Add more later, right now only accept SPS standard
    enum class SyncMethod
    {
        SIn_TrigOut
    };

    struct SyncArgs
    {
        SyncStartType startType = SyncStartType::SoftwareControlled;
        SyncMethod syncMethod = SyncMethod::SIn_TrigOut;
    };

    int SetChainSynchronize(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain);

    int StartSynchronizedRun(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain);

    int StopSynchronizedRun(const SyncArgs& args, const std::vector<std::shared_ptr<Digitizer>>& chain);

}

#endif