#ifndef DIGITIZER_DEFS_H
#define DIGITIZER_DEFS_H

#include <CAENDigitizerType.h>
#include <string>
#include <vector>

namespace BoxScore {

    //Unified BoxScore Data structure (similar to CAEN CoMPASS)
    struct BSData
    {
        uint16_t board;
        uint16_t channel;
        uint64_t timestamp;
        uint32_t energy;
        uint32_t energyShort;
        uint32_t flags;
        uint32_t waveSize;
        std::vector<uint16_t> trace1Samples; //For waves: traces are actual data to be saved potentially, digital is result of algorithms
        std::vector<uint16_t> trace2Samples;
        std::vector<uint8_t> digitalTrace1Samples;
        std::vector<uint8_t> digitalTrace2Samples; //Technically DPP supports 4 in docs? But all examples only seem to support 2...
    };

    //These need to be plain enums cause we're gonna cast them to int, so just be careful
    enum DynamicRange
    {
        Volt_2 = 0, //2Volt
        MilliVolt_500 = 1//0.5Volt
    };

    enum PHAVirtualProbe1Options
    {
        PHAVP1_Input = CAEN_DGTZ_DPP_VIRTUALPROBE_Input,
        PHAVP1_Delta = CAEN_DGTZ_DPP_VIRTUALPROBE_Delta,
        PHAVP1_Delta2 = CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2,
        PHAVP1_Trapezoid = CAEN_DGTZ_DPP_VIRTUALPROBE_Trapezoid
    };

    enum PHAVirtualProbe2Options
    {
        PHAVP2_Input = CAEN_DGTZ_DPP_VIRTUALPROBE_Input,
        PHAVP2_Threshold = CAEN_DGTZ_DPP_VIRTUALPROBE_Threshold,
        PHAVP2_Baseline = CAEN_DGTZ_DPP_VIRTUALPROBE_Baseline,
        PHAVP2_TrapezoidReduced = CAEN_DGTZ_DPP_VIRTUALPROBE_TrapezoidReduced,
        PHAVP2_None = CAEN_DGTZ_DPP_VIRTUALPROBE_None
    };

    enum PHADigitalProbe1Options
    {
        PHADP_TriggerWindow = CAEN_DGTZ_DPP_DIGITALPROBE_TRGWin,
        PHADP_Armed = CAEN_DGTZ_DPP_DIGITALPROBE_Armed,
        PHADP_PkRun = CAEN_DGTZ_DPP_DIGITALPROBE_PkRun,
        PHADP_PileUp = CAEN_DGTZ_DPP_DIGITALPROBE_PileUp,
        PHADP_Peaking = CAEN_DGTZ_DPP_DIGITALPROBE_Peaking,
        PHADP_CoincidenceWindow = CAEN_DGTZ_DPP_DIGITALPROBE_CoincWin,
        PHADP_BaselineFreeze = CAEN_DGTZ_DPP_DIGITALPROBE_BLFreeze,
        PHADP_TriggerHoldoff = CAEN_DGTZ_DPP_DIGITALPROBE_TRGHoldoff,
        PHADP_TriggerValue = CAEN_DGTZ_DPP_DIGITALPROBE_TRGVal,
        PHADP_ACQVeto = CAEN_DGTZ_DPP_DIGITALPROBE_ACQVeto,
        PHADP_BFMVeto = CAEN_DGTZ_DPP_DIGITALPROBE_BFMVeto,
        PHADP_ExternalTrig = CAEN_DGTZ_DPP_DIGITALPROBE_ExtTRG,
        PHADP_Busy = CAEN_DGTZ_DPP_DIGITALPROBE_Busy,
        PHADP_PrgVeto = CAEN_DGTZ_DPP_DIGITALPROBE_PrgVeto
    };

    enum PSDVirtualProbe1Options
    {
        PSDVP1_Input = CAEN_DGTZ_DPP_VIRTUALPROBE_Input,
        PSDVP1_CFD = CAEN_DGTZ_DPP_VIRTUALPROBE_CFD
    };

    enum PSDVirtualProbe2Options
    {
        PSDVP2_Baseline = CAEN_DGTZ_DPP_VIRTUALPROBE_Baseline,
        PSDVP2_CFD = CAEN_DGTZ_DPP_VIRTUALPROBE_CFD,
        PSDVP2_None = CAEN_DGTZ_DPP_VIRTUALPROBE_None
    };

    enum PSDDigitalProbe1Options
    {
        PSDDP1_Gate = CAEN_DGTZ_DPP_DIGITALPROBE_Gate,
        PSDDP1_OverThreshold = CAEN_DGTZ_DPP_DIGITALPROBE_OverThr,
        PSDDP1_TriggerOut = CAEN_DGTZ_DPP_DIGITALPROBE_TRGOut,
        PSDDP1_CoincidenceWindow = CAEN_DGTZ_DPP_DIGITALPROBE_CoincWin,
        PSDDP1_PileUp = CAEN_DGTZ_DPP_DIGITALPROBE_PileUp,
        PSDDP1_Coincidence = CAEN_DGTZ_DPP_DIGITALPROBE_Coincidence,
        PSDDP1_Trigger = CAEN_DGTZ_DPP_DIGITALPROBE_Trigger
    };

    enum PSDDigitalProbe2Options
    {
        PSDDP2_GateShort = CAEN_DGTZ_DPP_DIGITALPROBE_GateShort,
        PSDDP2_OverThreshold = CAEN_DGTZ_DPP_DIGITALPROBE_OverThr,
        PSDDP2_TriggerVal = CAEN_DGTZ_DPP_DIGITALPROBE_TRGVal,
        PSDDP2_TriggerHoldoff = CAEN_DGTZ_DPP_DIGITALPROBE_TRGHoldoff,
        PSDDP2_PileUp = CAEN_DGTZ_DPP_DIGITALPROBE_PileUp,
        PSDDP2_Coincidence = CAEN_DGTZ_DPP_DIGITALPROBE_Coincidence,
        PSDDP2_Trigger = CAEN_DGTZ_DPP_DIGITALPROBE_Trigger 
    };

    //Data vital to status/exsistance of digitizer
    struct DigitizerArgs
    {
        CAEN_DGTZ_ConnectionType type = CAEN_DGTZ_ConnectionType::CAEN_DGTZ_OpticalLink; //default (makes sense for FSU)
        int linkNumber = -1;
        int conetNode = -1;
        uint32_t vmeAddress = 0;
        int handle;
        CAEN_DGTZ_BoardModel_t model = CAEN_DGTZ_V1730; //Find way for default?
        CAEN_DGTZ_DPPFirmware_t firmware = CAEN_DGTZ_NotDPPFirmware;
        std::string name = "None";
        int status = CAEN_DGTZ_NotYetImplemented;
    };

    //Parameters that are applied to digitizer as a whole
    struct DigitizerParameters
    {
        uint32_t recordLength = 4096;
        uint32_t channelMask = 0xffff;
        int eventAggr = 0; //Allow CAEN to optimize
        CAEN_DGTZ_AcqMode_t acqMode = CAEN_DGTZ_SW_CONTROLLED;
        CAEN_DGTZ_DPP_AcqMode_t dppAcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;
        CAEN_DGTZ_IOLevel_t IOlevel = CAEN_DGTZ_IOLevel_NIM;
        CAEN_DGTZ_TriggerMode_t triggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
        CAEN_DGTZ_RunSyncMode_t syncMode = CAEN_DGTZ_RUN_SYNC_Disabled;
    };

    //Per channel PHA settings
    struct PHAParameters
    {
        bool isEnabled = true;
        uint32_t preTriggerSamples = 1000;
        float dcOffset = 0.2f;
        CAEN_DGTZ_PulsePolarity_t pulsePolarity = CAEN_DGTZ_PulsePolarityPositive;
        DynamicRange dynamicRange = DynamicRange::Volt_2;
        int decayTimeConst = 25000; //ch
        int trapFlatTop = 496; //ch
        int trapRiseTime = 96; //ch
        int flatTopDelay = 250; //Maybe? caen param ftd
        int triggerFilterSmoothing = 16; // Valid powers of 2 up to 32
        int inputRiseTime = 48; //ch
        int triggerThreshold = 300; //LSB
        int samplesBaseLineMean = 3; //Options: 1->16 samples; 2->64 samples; 3->256 samples; 4->1024 samples; 5->4096 samples; 6->16384 samples
        int samplesPeakMean = 3; //Options: 0-> 1 sample; 1->4 samples; 2->16 samples; 3->64 samples
        int peakHoldOff = 400; //ch
        int baseLineHoldOff = 500; //ns
        int otrej= 0; // n/a
        int triggerHoldOff = 496; //ns
        int riseTimeValidationWindow = 100;// ns
        int riseTimeDiscrimination = 0; // 0 off 1 on
        int digitalProbeGain = 0; //Options: 0->DigitalGain=1; 1->DigitalGain=2 (only with decimation >= 2samples); 2->DigitalGain=4 (only with decimation >= 4samples); 3->DigitalGain=8( only with decimation = 8samples)
        float energyNormalizationFactor = 1.0; //Default 1.0
        int inputDecimation = 0; // decimation (the input signal samples are averaged within this number of samples): 0 ->disabled; 1->2 samples; 2->4 samples; 3->8 samples
    };

    //Per channel PSD settings
    struct PSDParameters
    {
        bool isEnabled = true;
        uint32_t preTriggerSamples = 1000;
        float dcOffset = 0.2f;
        CAEN_DGTZ_PulsePolarity_t pulsePolarity = CAEN_DGTZ_PulsePolarityPositive;
        DynamicRange dynamicRange = DynamicRange::Volt_2;
        int baselineThreshold = 0; //Defined with fixed baseline only (LSB)
        int bltmo = 0;//n/a
        int triggerHoldOff = 496; //Samples
        int triggerThreshold = 300; //LSB   
        int selfTrigger = 1; //0-Disabled 1-Enabled 
        int chargeSensitivity = 1; //Comments only for 1720 & 1751? Check compass 
        int shortGate = 24; //ch 
        int longGate = 100; //ch 2ns for 1730 4ns for 1725
        int preGate = 8; //ch 
        int triggerValidationWindow = 1000; //samples?  
        int samplesBasline = 4; //Again only 1720 and 1751  
        int discrminatorType = CAEN_DGTZ_DPP_DISCR_MODE_CFD;
        int cfdFraction = 0; //0 25% 1 50% 2 75% 3 100%
        int cfdDelay = 4; //ch
        CAEN_DGTZ_DPP_TriggerConfig_t triggerConfig = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
        CAEN_DGTZ_DPP_PUR_t pileUpRejection = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
        int purgap = 100; //Purity Gap LSB
    };

    //Option of dual analog, which types (digital probe 2 is always trigger for PHA)
    struct PHAWaveParameters
    {
        CAEN_DGTZ_DPP_VirtualProbe_t isDual = CAEN_DGTZ_DPP_VIRTUALPROBE_SINGLE; //Default to a single analog trace
        PHAVirtualProbe1Options analogProbe1 = PHAVirtualProbe1Options::PHAVP1_Input; //Main analog trace defaults to input signal;
        PHAVirtualProbe2Options analogProbe2 = PHAVirtualProbe2Options::PHAVP2_None; //Default val; in default config wont be displayed
        PHADigitalProbe1Options digitalProbe1 = PHADigitalProbe1Options::PHADP_TriggerWindow; //Idk guess this is good default
    };

    struct PSDWaveParameters
    {
        CAEN_DGTZ_DPP_VirtualProbe_t isDual = CAEN_DGTZ_DPP_VIRTUALPROBE_SINGLE; //Default to a single analog trace
        PSDVirtualProbe1Options analogProbe1 = PSDVirtualProbe1Options::PSDVP1_Input; //Main trace defaults to input
        PSDVirtualProbe2Options analogProbe2 = PSDVirtualProbe2Options::PSDVP2_None; //Defaults to off
        PSDDigitalProbe1Options digitalProbe1 = PSDDigitalProbe1Options::PSDDP1_Gate; //Defaults to long gate
        PSDDigitalProbe2Options digitalProbe2 = PSDDigitalProbe2Options::PSDDP2_GateShort; //Defaults to short gate
    };
}

#endif