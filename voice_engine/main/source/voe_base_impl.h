/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VOICE_ENGINE_VOE_BASE_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_BASE_IMPL_H

#include "voe_base.h"

#include "module_common_types.h"
#include "shared_data.h"

namespace cloopenwebrtc
{

class ProcessThread;

class VoEBaseImpl: public VoEBase,
                   public AudioTransport,
                   public AudioDeviceObserver
{
public:
    virtual int RegisterVoiceEngineObserver(VoiceEngineObserver& observer);

    virtual int DeRegisterVoiceEngineObserver();

    virtual int Init(AudioDeviceModule* external_adm = NULL,
                     AudioProcessing* audioproc = NULL);
    virtual AudioProcessing* audio_processing() {
      return _shared->audio_processing();
    }

    virtual int Terminate();

    virtual int CreateChannel();
    virtual int CreateChannel(const Config& config);

    virtual int DeleteChannel(int channel);

    virtual int StartReceive(int channel);

    virtual int StartPlayout(int channel);

    virtual int StartSend(int channel);

    virtual int StartRecord();

    virtual int StopReceive(int channel);

    virtual int StopPlayout(int channel);

    virtual int StopSend(int channel);

    virtual int StopRecord();

    virtual int GetVersion(char version[1024]);

    virtual int LastError();

    virtual AudioTransport* audio_transport() { return this; }

    // AudioTransport
    virtual int32_t
        RecordedDataIsAvailable(const void* audioSamples,
                                uint32_t nSamples,
                                uint8_t nBytesPerSample,
                                uint8_t nChannels,
                                uint32_t samplesPerSec,
                                uint32_t totalDelayMS,
                                int32_t clockDrift,
                                uint32_t micLevel,
                                bool keyPressed,
                                uint32_t& newMicLevel);

    virtual int32_t NeedMorePlayData(uint32_t nSamples,
                                     uint8_t nBytesPerSample,
                                     uint8_t nChannels,
                                     uint32_t samplesPerSec,
                                     void* audioSamples,
                                     uint32_t& nSamplesOut,
                                     int64_t* elapsed_time_ms,
                                     int64_t* ntp_time_ms);

    virtual int OnDataAvailable(const int voe_channels[],
                                int number_of_voe_channels,
                                const int16_t* audio_data,
                                int sample_rate,
                                int number_of_channels,
                                int number_of_frames,
                                int audio_delay_milliseconds,
                                int volume,
                                bool key_pressed,
                                bool need_audio_processing);

    virtual void OnData(int voe_channel, const void* audio_data,
                        int bits_per_sample, int sample_rate,
                        int number_of_channels, int number_of_frames);

    virtual void PushCaptureData(int voe_channel, const void* audio_data,
                                 int bits_per_sample, int sample_rate,
                                 int number_of_channels, int number_of_frames);

    virtual void PullRenderData(int bits_per_sample, int sample_rate,
                                int number_of_channels, int number_of_frames,
                                void* audio_data,
                                int64_t* elapsed_time_ms,
                                int64_t* ntp_time_ms);

    // AudioDeviceObserver
    virtual void OnErrorIsReported(ErrorCode error);
    virtual void OnWarningIsReported(WarningCode warning);



    virtual int SetDtmfCb(int channelid, onReceivingDtmf dtmf_cb);
    virtual int SetMediaTimeoutCb(int channelid, onMediaPacketTimeout media_timeout_cb);
    virtual int SetStunCb(int channelid, onStunPacket stun_cb);
    virtual int SetAudioDataCb(int channelid, onAudioData audio_data_cb);
    virtual bool GetRecordingIsInitialized();
protected:
    VoEBaseImpl(voe::SharedData* shared);
    virtual ~VoEBaseImpl();

private:
    int32_t StartPlayout();
    int32_t StopPlayout();
    //int32_t StartSend();
    //int32_t StopSend();
    int32_t TerminateInternal();

    // Helper function to process the recorded data with AudioProcessing Module,
    // demultiplex the data to specific voe channels, encode and send to the
    // network. When |number_of_VoE_channels| is 0, it will demultiplex the
    // data to all the existing VoE channels.
    // It returns new AGC microphone volume or 0 if no volume changes
    // should be done.
    int ProcessRecordedDataWithAPM(const int voe_channels[],
                                   int number_of_voe_channels,
                                   const void* audio_data,
                                   uint32_t sample_rate,
                                   uint8_t number_of_channels,
                                   uint32_t number_of_frames,
                                   uint32_t audio_delay_milliseconds,
                                   int32_t clock_drift,
                                   uint32_t volume,
                                   bool key_pressed);

    void GetPlayoutData(int sample_rate, int number_of_channels,
                        int number_of_frames, bool feed_data_to_apm,
                        void* audio_data,
                        int64_t* elapsed_time_ms,
                        int64_t* ntp_time_ms);

    int32_t AddVoEVersion(char* str) const;

    // Initialize channel by setting Engine Information then initializing
    // channel.
    int InitializeChannel(voe::ChannelOwner* channel_owner);
#ifdef WEBRTC_EXTERNAL_TRANSPORT
    int32_t AddExternalTransportBuild(char* str) const;
#endif
    VoiceEngineObserver* _voiceEngineObserverPtr;
    CriticalSectionWrapper& _callbackCritSect;

    bool _voiceEngineObserver;
    AudioFrame _audioFrame;
    voe::SharedData* _shared;

//---begin
private:
	WebRtc_UWord32 _oldMicLevel;
	WebRtc_UWord32 _oldVoEMicLevel;

	bool _enlargeIncomingGainFlag;
	double _enlargeIncomingGainFactor;
	bool _enlargeOutgoingGainFlag;
	double _enlargeOutgoingGainFactor;

public:
	virtual WebRtc_Word32 SendRaw(int channel,
		const WebRtc_Word8* data,
		WebRtc_UWord32 length,
		WebRtc_Word32 isRTCP,
		WebRtc_UWord16 portnr = 0,
		const char* ip = NULL);

	//    Sean add begin 20131119 noise suppression
	virtual int NoiseSuppression(const void* audioSamples,
		WebRtc_Word16 *out,
		const WebRtc_UWord32 nSamples = 320,
		const WebRtc_UWord8 nBytesPerSample = 2,
		const WebRtc_UWord8 nChannels = 1,
		const WebRtc_UWord32 samplesPerSec =8000,
		const WebRtc_UWord32 totalDelayMS = 0,
		const WebRtc_Word32 clockDrift = 0,
		const WebRtc_UWord32 currentMicLevel = 0,
		const WebRtc_UWord32 mixingFrequency = 16000);
	//    Sean add end 20131119 noise suppression
	virtual int setProcessData(int channel, bool flag, bool originalFlag = false);
	virtual int pause(int channel, bool mute);
	//sean add begin 20141224 set network type
	virtual int SetNetworkType(int channelid, bool isWifi);
	//sean add end 20141224 set network type

	virtual int SetLocalReceiver(int channel, int port,
		int RTCPport = kVoEDefault,
		const char ipAddr[64] = NULL,
		const char multiCastAddr[64] = NULL);
		virtual int SetSocks5SendData(int charnnel_id, unsigned char *data, int length, bool isRTCP);
	virtual int SetSendDestination(int channel, int rtp_port, const char *rtp_ipaddr, int sourcePort, int rtcp_port, const char *rtcp_ipaddr);

	// Gets the destination port and address for a specified |channel| number.
	virtual int GetSendDestination(int channel, int& port, char ipAddr[64],
		int& sourcePort, int& RTCPport);

	// Gets the local receiver port and address for a specified
	// |channel| number.
	virtual int GetLocalReceiver(int channel, int& port, int& RTCPport,
		char ipAddr[64]);

	//sean add begin 20140422 SetAudioGain
	virtual int setEnlargeAudioFlagOutgoing(bool flag, double factor);
	virtual int setEnlargeAudioFlagIncoming(bool flag, double factor);
	//sean add end 20140422 SetAudioGain
	//sean add begin 20140626 init and release audio device
	virtual int RegisterAudioDevice();
	virtual int DeRegisterAudioDevice();
	//sean add end 20140626 init and release audio device

//	virtual int RegisterServiceCoreCallBack(int channel, ServiceCoreCallBack *messageCallBack, const char* call_id, int firewall_policy);
    virtual int SetFecStatus(int channel, bool enable);
    virtual int SetLoss(int channel, int loss);
	virtual void* GetChannel(int channelid);

//---end
};

}  // namespace webrtc

#endif  // WEBRTC_VOICE_ENGINE_VOE_BASE_IMPL_H
