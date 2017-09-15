//
//  ec_rtmp_publisher.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_rtmp_publisher_hpp
#define ec_rtmp_publisher_hpp

#include <stdio.h>
#include <list>
#include "ec_live_common.h"
#include "ec_rtmp_bitrate_controller.h"

#endif /* ec_rtmp_publisher_hpp */

namespace cloopenwebrtc {

    enum RTMP_STATUS
    {
        RS_STM_Init,		// ≥ı ºªØ◊¥Ã¨
        RS_STM_Handshaked,	// ”Î∑˛ŒÒ∆˜–≠…Ãπ˝≥Ã÷–
        RS_STM_Connected,	// ”Î∑˛ŒÒ∆˜¡¨Ω”≥…π¶
        RS_STM_Published,	// ø™ ºÕ∆¡˜
        RS_STM_Closed		// Õ∆¡˜πÿ±’
    };

    class EventTimerWrapper;
    class ThreadWrapper;
    class CriticalSectionWrapper;

    class ECRtmpPublisher : public EC_CapturerCallback {
    public:
        ECRtmpPublisher(EC_RtmpPublishCallback *callback, EC_RTMP_BitrateController *bc);
        ~ECRtmpPublisher();
        void start(const char *url);
        void stop();
        void EnableOnlyAudioMode();
        static bool publishThreadRun(void *pThis);
        void needClearCacher();
        // void OnAVCEncodeData(uint8_t *p, uint32_t length, uint32_t ts);

    private:
        // publish pthread
        bool run();
        void clearMediaCacher();
        void callOnDisconnect();
        void doPushRtmpPacket();
        void GotH264Nal(uint8_t* pData, int nLen, bool isKeyFrame, uint32_t ts);
        
        void OnCapturerAvcDataReady(uint8_t *pData, int nLen, uint32_t ts);
        void OnCapturerAacDataReady(uint8_t *pData, int nLen, uint32_t ts);
    private:
        std::string rtmp_url;
        ThreadWrapper* rtmpPublishThread_;
        EventTimerWrapper* cacher_update_event_;
        bool running_;
        void* rtmp_;

        RTMP_STATUS rtmp_status_;
        bool need_keyframe_;
        std::list<EncodedData*>		lst_enc_data_;
        int retrys_;
        
        
        CriticalSectionWrapper* rtmp_lock_;
        EC_RtmpPublishCallback *callback_;
        EC_RTMP_BitrateController *rtmp_bitrate_ontroller_;
        
        bool need_clear_av_cacher_;
    };
}

//int Send_AAC_SPEC();
//int BuildAndCacheAudioPacket(unsigned char *aac_data, int aac_data_len);
//int Send_SPS_PPS();
//int BuildAndCacheVideoPacket(std::vector<uint8_t> &nalus);
