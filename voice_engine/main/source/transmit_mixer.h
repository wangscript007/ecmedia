/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H
#define WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H

#include "push_resampler.h"
#include "common_types.h"
#include "typing_detection.h"
#include "module_common_types.h"
#include "file_player.h"
#include "file_recorder.h"
#include "scoped_ptr.h"
#include "voe_base.h"
#include "level_indicator.h"
#include "monitor_module.h"
#include "voice_engine_defines.h"

#include "wavfile.h"

#include "resampler.h"

namespace cloopenwebrtc {

class AudioProcessing;
class ProcessThread;
class VoEExternalMedia;
class VoEMediaProcess;

namespace voe {

class ChannelManager;
class MixedAudio;
class Statistics;

class TransmitMixer : public MonitorObserver,
                      public FileCallback {
public:
    static int32_t Create(TransmitMixer*& mixer, uint32_t instanceId);

    static void Destroy(TransmitMixer*& mixer);

    int32_t SetEngineInformation(ProcessThread& processThread,
                                 Statistics& engineStatistics,
                                 ChannelManager& channelManager);

    int32_t SetAudioProcessingModule(
        AudioProcessing* audioProcessingModule);

    int32_t PrepareDemux(const void* audioSamples,
                         uint32_t nSamples,
                         uint8_t  nChannels,
                         uint32_t samplesPerSec,
                         uint16_t totalDelayMS,
                         int32_t  clockDrift,
                         uint16_t currentMicLevel,
                         bool keyPressed);


    int32_t DemuxAndMix();
    // Used by the Chrome to pass the recording data to the specific VoE
    // channels for demux.
    void DemuxAndMix(const int voe_channels[], int number_of_voe_channels);

    int32_t EncodeAndSend();
    // Used by the Chrome to pass the recording data to the specific VoE
    // channels for encoding and sending to the network.
    void EncodeAndSend(const int voe_channels[], int number_of_voe_channels);

    // Must be called on the same thread as PrepareDemux().
    uint32_t CaptureLevel() const;

    int32_t StopSend();

    // VoEDtmf
    void UpdateMuteMicrophoneTime(uint32_t lengthMs);

    // VoEExternalMedia
    int RegisterExternalMediaProcessing(VoEMediaProcess* object,
                                        ProcessingTypes type);
    int DeRegisterExternalMediaProcessing(ProcessingTypes type);

    int GetMixingFrequency();

    // VoEVolumeControl
    int SetMute(bool enable);

    bool Mute() const;

    int8_t AudioLevel() const;

    int16_t AudioLevelFullRange() const;

    bool IsRecordingCall();

    bool IsRecordingMic();

    int StartPlayingFileAsMicrophone(const char* fileName,
                                     bool loop,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);

    int StartPlayingFileAsMicrophone(InStream* stream,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);

    int StopPlayingFileAsMicrophone();

    int IsPlayingFileAsMicrophone() const;

    int StartRecordingMicrophone(const char* fileName,
                                 const CodecInst* codecInst);

    int StartRecordingMicrophone(OutStream* stream,
                                 const CodecInst* codecInst);

    int StopRecordingMicrophone();

    int StartRecordingCall(const char* fileName, const CodecInst* codecInst);

    int StartRecordingCall(OutStream* stream, const CodecInst* codecInst);

    int StopRecordingCall();

    void SetMixWithMicStatus(bool mix);

    int32_t RegisterVoiceEngineObserver(VoiceEngineObserver& observer);

    virtual ~TransmitMixer();

    // MonitorObserver
    void OnPeriodicProcess();


    // FileCallback
    void PlayNotification(int32_t id,
                          uint32_t durationMs);

    void RecordNotification(int32_t id,
                            uint32_t durationMs);

    void PlayFileEnded(int32_t id);

    void RecordFileEnded(int32_t id);

                          
                          int32_t RecordAudioToFileCall(uint32_t mixingFrequency);
#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    // Typing detection
    int TimeSinceLastTyping(int &seconds);
    int SetTypingDetectionParameters(int timeWindow,
                                     int costPerTyping,
                                     int reportingThreshold,
                                     int penaltyDecay,
                                     int typeEventDelay);
#endif

  void EnableStereoChannelSwapping(bool enable);
  bool IsStereoChannelSwappingEnabled();

#ifdef WIN32_MIC
	bool IsSilence();
#endif

private:
    TransmitMixer(uint32_t instanceId);

    // Gets the maximum sample rate and number of channels over all currently
    // sending codecs.
    void GetSendCodecInfo(int* max_sample_rate, int* max_channels);
#ifdef WIN32_MIC
	bool silence(const short* data, int sample_length, float thres, float probility);
#endif
    void GenerateAudioFrame(const int16_t audioSamples[],
                            int nSamples,
                            int nChannels,
                            int samplesPerSec);
    int32_t RecordAudioToFile(uint32_t mixingFrequency);

    int32_t MixOrReplaceAudioWithFile(
        int mixingFrequency);

    void ProcessAudio(int delay_ms, int clock_drift, int current_mic_level,
                      bool key_pressed);

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    void TypingDetection(bool keyPressed);
#endif

    // uses
    Statistics* _engineStatisticsPtr;
    ChannelManager* _channelManagerPtr;
    AudioProcessing* audioproc_;
    VoiceEngineObserver* _voiceEngineObserverPtr;
    ProcessThread* _processThreadPtr;

    // owns
    MonitorModule _monitorModule;
    AudioFrame _audioFrame;
    PushResampler<int16_t> resampler_;  // ADM sample rate -> mixing rate
    FilePlayer* _filePlayerPtr;
    FileRecorder* _fileRecorderPtr;
    FileRecorder* _fileCallRecorderPtr;
    int _filePlayerId;
    int _fileRecorderId;
    int _fileCallRecorderId;
    bool _filePlaying;
    bool _fileRecording;
    bool _fileCallRecording;

 
	bool _fileopen;
	WaveFileParams _wavParams;
	int  _fileindex;
	int  _writecount;
    voe::AudioLevel _audioLevel;
    // protect file instances and their variables in MixedParticipants()
    CriticalSectionWrapper& _critSect;
    CriticalSectionWrapper& _callbackCritSect;

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    cloopenwebrtc::TypingDetection _typingDetection;
    bool _typingNoiseWarningPending;
    bool _typingNoiseDetected;
#endif
    bool _saturationWarning;

    int _instanceId;
    bool _mixFileWithMicrophone;
    uint32_t _captureLevel;
    VoEMediaProcess* external_postproc_ptr_;
    VoEMediaProcess* external_preproc_ptr_;
    bool _mute;
    int32_t _remainingMuteMicTimeMs;
    bool stereo_codec_;
    bool swap_stereo_channels_;
    scoped_ptr<int16_t[]> mono_buffer_;
#ifdef WIN32_MIC
	short _data1S[16000];
#endif
//---begin
public:
	//    Sean add begin 20131119 noise suppression
	WebRtc_Word32 GenerateAudioFrameNoiseSuppression(const WebRtc_Word16 audioSamples[],
													const WebRtc_UWord32 nSamples,
													const WebRtc_UWord8 nChannels,
													const WebRtc_UWord32 samplesPerSec,
													const int mixingFrequency);

	WebRtc_Word32 APMProcessStreamNoiseSuppression(const WebRtc_UWord16 totalDelayMS,
													const WebRtc_Word32 clockDrift,
													const WebRtc_UWord16 currentMicLevel);

	//    Sean add end 20131119 noise suppression

	//    Sean add begin 20131119 noise suppression
	AudioFrame _audioFrameNoiseSuppression;
	Resampler _audioResamplerNoiseSuppression;
	//    Sean add end 20131119 noise suppression
private:
	//    sean add begin 20140708 original audio sample
	AudioFrame _audioFrame2Up;
	//    sean add end 20140708 original audio sample
                          
                        
//---end
};

}  // namespace voe

}  // namespace cloopenwebrtc

#endif  // WEBRTC_VOICE_ENGINE_TRANSMIT_MIXER_H
