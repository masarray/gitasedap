#pragma once

#define PLUG_NAME "GitaSedap"
#define PLUG_MFR "MasArray"
#define PLUG_VERSION_HEX 0x00000100
#define PLUG_VERSION_STR "0.1.0"
#define PLUG_UNIQUE_ID 'GtSd'
#define PLUG_MFR_ID 'MsAr'
#define PLUG_URL_STR "https://github.com/masarray/gitasedap"
#define PLUG_EMAIL_STR ""
#define PLUG_COPYRIGHT_STR "Copyright 2026 GitaSedap contributors"
#define PLUG_CLASS_NAME GitaSedap

#define BUNDLE_NAME "GitaSedap"
#define BUNDLE_MFR "MasArray"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "GitaSedap"

// Guitar input is mono. The host may expose mono or stereo output.
// Stereo processing will become meaningful when the body-space engine arrives.
#define PLUG_CHANNEL_IO "1-1 1-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0

// Host state is serialized as a versioned GitaSedap chunk. This establishes a
// migration boundary before profiles and structural DSP state are introduced.
#define PLUG_DOES_STATE_CHUNKS 1

#define PLUG_HAS_UI 1
#define PLUG_WIDTH 720
#define PLUG_HEIGHT 360
#define PLUG_FPS 30
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 1
#define PLUG_MIN_WIDTH 600
#define PLUG_MIN_HEIGHT 300
#define PLUG_MAX_WIDTH 1440
#define PLUG_MAX_HEIGHT 720

#define AUV2_ENTRY GitaSedap_Entry
#define AUV2_ENTRY_STR "GitaSedap_Entry"
#define AUV2_FACTORY GitaSedap_Factory
#define AUV2_VIEW_CLASS GitaSedap_View
#define AUV2_VIEW_CLASS_STR "GitaSedap_View"

#define AAX_TYPE_IDS 'GSD1'
#define AAX_TYPE_IDS_AUDIOSUITE 'GSA1'
#define AAX_PLUG_MFR_STR "MasArray"
#define AAX_PLUG_NAME_STR "GitaSedap\nGitaSedap"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 0

#define VST3_SUBCATEGORY "Fx"

#define CLAP_MANUAL_URL "https://github.com/masarray/gitasedap"
#define CLAP_SUPPORT_URL "https://github.com/masarray/gitasedap/issues"
#define CLAP_DESCRIPTION "Low-latency acoustic guitar body and polish processor"
#define CLAP_FEATURES "audio-effect"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64
