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

    void SetChainSynchronize(const std::vector<std::shared_ptr<Digitizer>>& chain);

    void StartSynchronizedRun(const std::vector<std::shared_ptr<Digitizer>>& chain);

    void StopSynchronizedRun(const std::vector<std::shared_ptr<Digitizer>>& chain);

}

#endif