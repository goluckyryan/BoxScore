#include "Digitizer.h"
#include "CAENDigitizer.h"

//Mask a specific bit
#define BIT(x) (1<<x)

namespace BoxScore {

    /////////////////////// Open Function ///////////////////////
    Digitizer* OpenDigitizer(DigitizerArgs& args)
    {
        int code = CAEN_DGTZ_OpenDigitizer(args.type, args.linkNumber, args.conetNode, args.vmeAddress, &args.handle);
        if(code != CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success)
        {
            //report error
            return nullptr;
        }
        CAEN_DGTZ_BoardInfo_t info;
        CAEN_DGTZ_DPPFirmware_t firmware;
        code |= CAEN_DGTZ_GetInfo(args.handle, &info);
        code |= CAEN_DGTZ_GetDPPFirmwareType(args.handle, &firmware);
        if(code != CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success)
        {
            //report errors
            return nullptr;
        }

        switch(firmware)
        {
            case CAEN_DGTZ_DPPFirmware_PHA: return new DigitizerPHA(args, info, code);
            case CAEN_DGTZ_DPPFirmware_PSD: return new DigitizerPSD(args, info, code);
            case CAEN_DGTZ_DPPFirmware_DAW: return nullptr;
            case CAEN_DGTZ_DPPFirmware_CI: return nullptr;
	        case CAEN_DGTZ_DPPFirmware_ZLE: return nullptr;
	        case CAEN_DGTZ_DPPFirmware_QDC: return nullptr;
        }

        return nullptr;
    }
    /////////////////////// Open Function ///////////////////////


    /////////////////////// DigitizerPHA ///////////////////////
    DigitizerPHA::DigitizerPHA(const DigitizerArgs& args, const CAEN_DGTZ_BoardInfo_t& info, int ec) :
        Digitizer(), m_eventData(nullptr), m_eventSize(nullptr), m_waveData(nullptr)
    {
        Init(args, info, ec);
    }

    DigitizerPHA::~DigitizerPHA() { Close(); }

    void DigitizerPHA::Init(const DigitizerArgs& args, const CAEN_DGTZ_BoardInfo_t& info, int ec)
    {
        m_args = args;
        m_internalData = info;

        m_args.status = ec;
        if(info.Model == 1730)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1730;
        else if(info.Model == 1725)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1725;
        else if(info.Model == 1740)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1740;
        else
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_DT5720;
        m_args.name = info.ModelName + std::to_string(info.SerialNumber);
        m_args.firmware = CAEN_DGTZ_DPPFirmware_PHA;

        m_channelParams.resize(info.Channels);
        m_outputData.resize(info.Channels);
        for(auto& hit : m_outputData)
            hit.board = m_args.handle;

        m_eventSize = new uint32_t[info.Channels];
        m_eventData = new CAEN_DGTZ_DPP_PHA_Event_t*[info.Channels];

        LoadDigitizerParameters();
        LoadChannelParameters();
        //Must load default parameters here to generate a buffer 
        AllocateMemory(); //More specifically: CAEN memory

        m_isConnected = true;
    }

    void DigitizerPHA::Close()
    {
        StopAquisition(); //Stop aquisition if needed

        m_args.status |= CAEN_DGTZ_CloseDigitizer(m_args.handle);
        m_args.status |= CAEN_DGTZ_FreeReadoutBuffer(&m_lowBuffer);
        m_args.status |= CAEN_DGTZ_FreeDPPEvents(m_args.handle, (void**)(m_eventData));
        m_args.status |= CAEN_DGTZ_FreeDPPWaveforms(m_args.handle, (void*)(m_waveData));

        delete[] m_eventSize;

        m_lowBuffer = nullptr;
        m_eventData = nullptr;
        m_eventSize = nullptr;
        m_waveData = nullptr;

        m_isConnected = false;
    }

    void DigitizerPHA::SetDigitizerParameters(const DigitizerParameters& params)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_digitizerParams = params;
        LoadDigitizerParameters();
    }

    void DigitizerPHA::SetChannelParameters(const PHAParameters& params, int channel)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_channelParams[channel] = params;
        LoadChannelParameters();
    }

    void DigitizerPHA::SetWaveformParameters(const PHAWaveParameters& params)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_waveParams = params;
        LoadWaveformParameters();
    }

    //This cannot possibly be the correct method for cases where digitizers are chained.
    //July 2022: Is not. See CAEN SyncTest for example
    void DigitizerPHA::StartAquisition()
    {
        if(!m_isActive && m_isConnected)
        {
            m_args.status |= CAEN_DGTZ_SWStartAcquisition(m_args.handle);
            m_isActive = true;
        }
    }

    //This cannot possibly be the correct method for cases where digitizers are chained.
    //July 2022: Is not. See CAEN SyncTest for example
    void DigitizerPHA::StopAquisition()
    {
        if(m_isActive && m_isConnected)
        {
            m_args.status |= CAEN_DGTZ_SWStopAcquisition(m_args.handle);
            m_isActive = false;
        }
    }

    //Set digitizer wide parameters
    void DigitizerPHA::LoadDigitizerParameters()
    {
        m_args.status |= CAEN_DGTZ_SetAcquisitionMode(m_args.handle, m_digitizerParams.acqMode);
        m_args.status |= CAEN_DGTZ_SetDPPAcquisitionMode(m_args.handle, m_digitizerParams.dppAcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
        m_args.status |= CAEN_DGTZ_SetRecordLength(m_args.handle, m_digitizerParams.recordLength);
        m_args.status |= CAEN_DGTZ_SetIOLevel(m_args.handle, m_digitizerParams.IOlevel);

        m_args.status |= CAEN_DGTZ_SetExtTriggerInputMode(m_args.handle, m_digitizerParams.triggerMode);
        m_args.status |= CAEN_DGTZ_SetDPPEventAggregation(m_args.handle, m_digitizerParams.eventAggr, 0);
        m_args.status |= CAEN_DGTZ_SetRunSynchronizationMode(m_args.handle, m_digitizerParams.syncMode);
        m_args.status |= CAEN_DGTZ_SetChannelEnableMask(m_args.handle, m_digitizerParams.channelMask);
    }

    //Set per channel data
    void DigitizerPHA::LoadChannelParameters()
    {
        m_digitizerParams.channelMask = 0;
        for(size_t i=0; i<m_channelParams.size(); i++)
        {
            m_args.status |= CAEN_DGTZ_SetChannelDCOffset(m_args.handle, i, uint32_t(0xffff * m_channelParams[i].dcOffset)); //Max range is 0xffff
            m_args.status |= CAEN_DGTZ_SetDPPPreTriggerSize(m_args.handle, i, m_channelParams[i].preTriggerSamples);
            m_args.status |= CAEN_DGTZ_SetChannelPulsePolarity(m_args.handle, i, m_channelParams[i].pulsePolarity);
            m_args.status |= CAEN_DGTZ_WriteRegister(m_args.handle, 0x1028 +  (i<<8), m_channelParams[i].dynamicRange);
            
            if(m_channelParams[i].isEnabled)
            {
                m_digitizerParams.channelMask |= (BIT(i)); //flip channel bit to 1 scince its enabled
            }
            
            //Write data to garbage CAEN style structs

            m_caenParams.M[i] = m_channelParams[i].decayTimeConst;
            m_caenParams.m[i] = m_channelParams[i].trapFlatTop;
            m_caenParams.k[i] = m_channelParams[i].trapRiseTime;
            m_caenParams.ftd[i] = m_channelParams[i].flatTopDelay;
            m_caenParams.a[i] = m_channelParams[i].triggerFilterSmoothing;
            m_caenParams.b[i] = m_channelParams[i].inputRiseTime;
            m_caenParams.thr[i] = m_channelParams[i].triggerThreshold;
            m_caenParams.nsbl[i] = m_channelParams[i].samplesBaseLineMean;
            m_caenParams.nspk[i] = m_channelParams[i].samplesPeakMean;
            m_caenParams.pkho[i] = m_channelParams[i].peakHoldOff;
            m_caenParams.blho[i] = m_channelParams[i].baseLineHoldOff;
            m_caenParams.otrej[i] = m_channelParams[i].otrej;
            m_caenParams.trgho[i] = m_channelParams[i].triggerHoldOff;
            m_caenParams.twwdt[i] = m_channelParams[i].riseTimeValidationWindow;
            m_caenParams.trgwin[i] = m_channelParams[i].riseTimeDiscrimination;
            m_caenParams.dgain[i] = m_channelParams[i].digitalProbeGain;
            m_caenParams.enf[i] = m_channelParams[i].energyNormalizationFactor;
            m_caenParams.decimation[i] = m_channelParams[i].inputDecimation;
            //So far as I can tell these are not used
	        m_caenParams.enskim[i] = 0;
	        m_caenParams.eskimlld[i] = 0;
            m_caenParams.eskimuld[i] = 0;
	        m_caenParams.blrclip[i] = 0;
            m_caenParams.dcomp[i] = 0;
            m_caenParams.trapbsl[i] = 0;
        }

        m_args.status |= CAEN_DGTZ_SetDPPParameters(m_args.handle, m_digitizerParams.channelMask, &m_caenParams);
    }

    void DigitizerPHA::LoadWaveformParameters()
    {
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, ANALOG_TRACE_1, m_waveParams.analogProbe1);
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, ANALOG_TRACE_2, m_waveParams.analogProbe2);
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, DIGITAL_TRACE_1, m_waveParams.digitalProbe1);
    }

    void DigitizerPHA::AllocateMemory()
    {
        m_args.status |= CAEN_DGTZ_MallocReadoutBuffer(m_args.handle, &m_lowBuffer, &m_lowBufferSize);
        m_args.status |= CAEN_DGTZ_MallocDPPEvents(m_args.handle, (void**)(&m_eventData), m_eventSize); //void casts are soooo bad .... but required by CAEN API
        m_args.status |= CAEN_DGTZ_MallocDPPWaveforms(m_args.handle, (void**)(&m_waveData), &m_waveSize);
    }

    const std::vector<BSData>& DigitizerPHA::ReadData()
    {
        if(!m_isActive || !m_isConnected)
            return m_outputData;

        m_args.status |= CAEN_DGTZ_ReadData(m_args.handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, m_lowBuffer, &m_lowBufferSize);
        m_args.status |= CAEN_DGTZ_GetDPPEvents(m_args.handle, m_lowBuffer, m_lowBufferSize, (void**)(&m_eventData), m_eventSize);

        for(int i=0; i<m_internalData.Channels; i++)
        {
            m_outputData[i].channel = i;
            for(int j=0; j<m_eventSize[i]; j++)
            {
                m_outputData[i].energy = m_eventData[i][j].Energy;
                m_outputData[i].timestamp = m_eventData[i][j].TimeTag;
                m_outputData[i].flags = m_eventData[i][j].Extras;

                if(m_digitizerParams.dppAcqMode != CAEN_DGTZ_DPP_ACQ_MODE_List)
                {
                    CAEN_DGTZ_DecodeDPPWaveforms(m_args.handle, (void*)&(m_eventData[i][j]), m_waveData);
                    m_outputData[i].waveSize = m_waveData->Ns;
                    m_waveSize = m_waveData->Ns;
                    if(m_outputData[i].waveSize != 0)
                    {
                        //Copy the data to our vectors PHA supports 2 analog traces and 2 digital traces
                        m_outputData[i].trace1Samples.assign(m_waveData->Trace1, m_waveData->Trace1 + m_waveSize);
                        m_outputData[i].trace2Samples.assign(m_waveData->Trace2, m_waveData->Trace2 + m_waveSize); //This is all zero if in single analog trace mode
                        m_outputData[i].digitalTrace1Samples.assign(m_waveData->DTrace1, m_waveData->DTrace1 + m_waveSize);
                        m_outputData[i].digitalTrace2Samples.assign(m_waveData->DTrace2, m_waveData->DTrace2 + m_waveSize);
                    }
                }
            }
        }
        return m_outputData;
    }
    /////////////////////// DigitizerPHA ///////////////////////

    /////////////////////// DigitizerPSD ///////////////////////
    DigitizerPSD::DigitizerPSD(const DigitizerArgs& args, const CAEN_DGTZ_BoardInfo_t& info, int code) :
        Digitizer(), m_eventData(nullptr), m_eventSize(nullptr), m_waveData(nullptr)
    {
        Init(args, info, code);
    }

    DigitizerPSD::~DigitizerPSD() { Close(); }

    void DigitizerPSD::Init(const DigitizerArgs& args, const CAEN_DGTZ_BoardInfo_t& info, int ec)
    {
        m_args = args;
        m_internalData = info;

        m_args.status = ec;
        if(info.Model == 1730)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1730;
        else if(info.Model == 1725)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1725;
        else if(info.Model == 1740)
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_V1740;
        else
            m_args.model = CAEN_DGTZ_BoardModel_t::CAEN_DGTZ_DT5720;
        m_args.name = info.ModelName + std::to_string(info.SerialNumber);
        m_args.firmware = CAEN_DGTZ_DPPFirmware_PSD;

        m_channelParams.resize(info.Channels);
        m_outputData.resize(info.Channels);
        for(auto& hit : m_outputData)
            hit.board = m_args.handle;

        m_eventSize = new uint32_t[info.Channels];
        m_eventData = new CAEN_DGTZ_DPP_PSD_Event_t*[info.Channels];

        LoadDigitizerParameters();
        LoadChannelParameters();
        //Must load default parameters here to generate a buffer 
        AllocateMemory(); //More specifically: CAEN memory

        m_isConnected = true;
    }

    void DigitizerPSD::Close()
    {
        StopAquisition(); //Stop aquisition if needed

        m_args.status |= CAEN_DGTZ_CloseDigitizer(m_args.handle);
        m_args.status |= CAEN_DGTZ_FreeReadoutBuffer(&m_lowBuffer);
        m_args.status |= CAEN_DGTZ_FreeDPPEvents(m_args.handle, (void**)(m_eventData));
        m_args.status |= CAEN_DGTZ_FreeDPPWaveforms(m_args.handle, (void*)(m_waveData));

        delete[] m_eventSize;

        m_lowBuffer = nullptr;
        m_eventData = nullptr;
        m_eventSize = nullptr;
        m_waveData = nullptr;

        m_isConnected = false;
    }

    void DigitizerPSD::SetDigitizerParameters(const DigitizerParameters& params)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_digitizerParams = params;
        LoadDigitizerParameters();
    }

    void DigitizerPSD::SetChannelParameters(const PSDParameters& params, int channel)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_channelParams[channel] = params;
        LoadChannelParameters();
    }

    void DigitizerPSD::SetWaveformParameters(const PSDWaveParameters& params)
    {
        if(!m_isConnected)
            return;

        if(m_isActive)
        {
            StopAquisition();
        }
        m_waveParams = params;
        LoadWaveformParameters();
    }

    //This cannot possibly be the correct method for cases where digitizers are chained.
    //July 2022: Is not. See CAEN SyncTest for example
    void DigitizerPSD::StartAquisition()
    {
        if(!m_isActive && m_isConnected)
        {
            m_args.status |= CAEN_DGTZ_SWStartAcquisition(m_args.handle);
            m_isActive = true;
        }
    }

    //This cannot possibly be the correct method for cases where digitizers are chained.
    //July 2022: Is not. See CAEN SyncTest for example
    void DigitizerPSD::StopAquisition()
    {
        if(m_isActive && m_isConnected)
        {
            m_args.status |= CAEN_DGTZ_SWStopAcquisition(m_args.handle);
            m_isActive = false;
        }
    }

    //Set digitizer wide parameters
    void DigitizerPSD::LoadDigitizerParameters()
    {
        m_args.status |= CAEN_DGTZ_SetAcquisitionMode(m_args.handle, m_digitizerParams.acqMode);
        m_args.status |= CAEN_DGTZ_SetDPPAcquisitionMode(m_args.handle, m_digitizerParams.dppAcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime); //why would you ever not want one of these?
        m_args.status |= CAEN_DGTZ_SetRecordLength(m_args.handle, m_digitizerParams.recordLength);
        m_args.status |= CAEN_DGTZ_SetIOLevel(m_args.handle, m_digitizerParams.IOlevel);

        m_args.status |= CAEN_DGTZ_SetExtTriggerInputMode(m_args.handle, m_digitizerParams.triggerMode);
        m_args.status |= CAEN_DGTZ_SetDPPEventAggregation(m_args.handle, m_digitizerParams.eventAggr, 0);
        m_args.status |= CAEN_DGTZ_SetRunSynchronizationMode(m_args.handle, m_digitizerParams.syncMode);
        m_args.status |= CAEN_DGTZ_SetChannelEnableMask(m_args.handle, m_digitizerParams.channelMask);
    }

    //Set per channel data
    void DigitizerPSD::LoadChannelParameters()
    {
        m_digitizerParams.channelMask = 0;
        for(size_t i=0; i<m_channelParams.size(); i++)
        {
            m_args.status |= CAEN_DGTZ_SetChannelDCOffset(m_args.handle, i, uint32_t(0xffff * m_channelParams[i].dcOffset)); //Max range is 0xffff
            m_args.status |= CAEN_DGTZ_SetDPPPreTriggerSize(m_args.handle, i, m_channelParams[i].preTriggerSamples);
            m_args.status |= CAEN_DGTZ_SetChannelPulsePolarity(m_args.handle, i, m_channelParams[i].pulsePolarity);
            m_args.status |= CAEN_DGTZ_WriteRegister(m_args.handle, 0x1028 +  (i<<8), m_channelParams[i].dynamicRange);
            
            if(m_channelParams[i].isEnabled)
            {
                m_digitizerParams.channelMask |= (BIT(i)); //flip channel bit to 1 scince its enabled
            }
            
            //Write data to garbage CAEN style structs
            m_caenParams.thr[i] = m_channelParams[i].triggerThreshold;
            m_caenParams.selft[i] = m_channelParams[i].selfTrigger;
            m_caenParams.csens[i] = m_channelParams[i].chargeSensitivity;
            m_caenParams.sgate[i] = m_channelParams[i].shortGate;
            m_caenParams.lgate[i] = m_channelParams[i].longGate;
            m_caenParams.pgate[i] = m_channelParams[i].preGate;
            m_caenParams.tvaw[i] = m_channelParams[i].triggerValidationWindow;
            m_caenParams.nsbl[i] = m_channelParams[i].samplesBasline;
            m_caenParams.discr[i] = m_channelParams[i].discrminatorType;
            m_caenParams.cfdf[i] = m_channelParams[i].cfdFraction;
            m_caenParams.cfdd[i] = m_channelParams[i].cfdDelay;
            m_caenParams.trgc[i] = m_channelParams[i].triggerConfig; //This seems to be the same as selft? Identical parameters?
        }
        //These are like global digitizer params but PSD specific. Here we treat first channel as "global" setting (similar to CoMPASS)
        m_caenParams.blthr = m_channelParams[0].baselineThreshold;
        m_caenParams.bltmo = m_channelParams[0].bltmo;
        m_caenParams.trgho = m_channelParams[0].triggerHoldOff;
        m_caenParams.purh = m_channelParams[0].pileUpRejection;
        m_caenParams.purgap = m_channelParams[0].purgap;

        m_args.status |= CAEN_DGTZ_SetDPPParameters(m_args.handle, m_digitizerParams.channelMask, &m_caenParams);
    }

    void DigitizerPSD::LoadWaveformParameters()
    {
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, ANALOG_TRACE_1, m_waveParams.analogProbe1);
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, ANALOG_TRACE_2, m_waveParams.analogProbe2);
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, DIGITAL_TRACE_1, m_waveParams.digitalProbe1);
        m_args.status |= CAEN_DGTZ_SetDPP_VirtualProbe(m_args.handle, DIGITAL_TRACE_2, m_waveParams.digitalProbe2);
    }

    void DigitizerPSD::AllocateMemory()
    {
        m_args.status |= CAEN_DGTZ_MallocReadoutBuffer(m_args.handle, &m_lowBuffer, &m_lowBufferSize);
        m_args.status |= CAEN_DGTZ_MallocDPPEvents(m_args.handle, (void**)(&m_eventData), m_eventSize); //void casts are soooo bad .... but required by CAEN API
        m_args.status |= CAEN_DGTZ_MallocDPPWaveforms(m_args.handle, (void**)(&m_waveData), &m_waveSize);
    }

    const std::vector<BSData>& DigitizerPSD::ReadData()
    {
        if(!m_isActive || !m_isConnected)
            return m_outputData;

        m_args.status |= CAEN_DGTZ_ReadData(m_args.handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, m_lowBuffer, &m_lowBufferSize);
        m_args.status |= CAEN_DGTZ_GetDPPEvents(m_args.handle, m_lowBuffer, m_lowBufferSize, (void**)(&m_eventData), m_eventSize);

        for(int i=0; i<m_internalData.Channels; i++)
        {
            m_outputData[i].channel = i;
            for(int j=0; j<m_eventSize[i]; j++)
            {
                m_outputData[i].energy = m_eventData[i][j].ChargeLong;
                m_outputData[i].energyShort = m_eventData[i][j].ChargeShort;
                m_outputData[i].timestamp = m_eventData[i][j].TimeTag;
                m_outputData[i].flags = m_eventData[i][j].Extras;

                if(m_digitizerParams.dppAcqMode != CAEN_DGTZ_DPP_ACQ_MODE_List)
                {
                    CAEN_DGTZ_DecodeDPPWaveforms(m_args.handle, (void*)&(m_eventData[i][j]), m_waveData);
                    m_outputData[i].waveSize = m_waveData->Ns;
                    m_waveSize = m_waveData->Ns;
                    if(m_outputData[i].waveSize != 0)
                    {
                        //Copy the data to our vectors PHA supports 2 analog traces and 2 digital traces
                        m_outputData[i].trace1Samples.assign(m_waveData->Trace1, m_waveData->Trace1 + m_waveSize);
                        m_outputData[i].trace2Samples.assign(m_waveData->Trace2, m_waveData->Trace2 + m_waveSize); //This is all zero if in single analog trace mode
                        m_outputData[i].digitalTrace1Samples.assign(m_waveData->DTrace1, m_waveData->DTrace1 + m_waveSize);
                        m_outputData[i].digitalTrace2Samples.assign(m_waveData->DTrace2, m_waveData->DTrace2 + m_waveSize);
                    }
                }
            }
        }
        return m_outputData;
    }
}