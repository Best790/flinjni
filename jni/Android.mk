LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := jsoncpp
LOCAL_CPPFLAGS := -fexceptions
LOCAL_SRC_FILES := vendor/json/jsoncpp.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)vendor/json/json
LOCAL_LDLIBS := -L$(call host-path, $(LOCAL_PATH)/../../libs/armeabi)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libopenal
LOCAL_SRC_FILES := vendor/openal/libopenal.a

include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE    := libopus
LOCAL_SRC_FILES := vendor/opus/libopus.a

include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE    := libenet
LOCAL_SRC_FILES := vendor/enet/libenet.a

include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE    := libcurl
LOCAL_SRC_FILES := vendor/curl/libcurl.a

include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)

LOCAL_MODULE := samp
LOCAL_LDLIBS := -lm -llog -lc -lz -ljnigraphics -landroid -lEGL -lGLESv2 -lOpenSLES

LOCAL_C_INCLUDES += $(wildcard $(LOCAL_PATH)/vendor/)
LOCAL_C_INCLUDES += $(wildcard $(LOCAL_PATH)/xrtutils/)
# samp
FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/TextRasterizer/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/Custom/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/net/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/util/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/RW/RenderWare.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/gui/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/voice/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/cryptors/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/graphics/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/xrtutils/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../santrope-tea-gtasa/encryption/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../santrope-tea-gtasa/encryption/*.c)

# vendor
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/ini/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/RakNet/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/RakNet/SAMP/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/imgui/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/hash/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/hash/bcrypt/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/hash/bcrypt/crypt_blowfish/*.cpp)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES := libopenal libopus libenet libcurl
#LOCAL_CPPFLAGS := -w -s -pthread -Wall -fpack-struct=1 -O2 -std=c++14 -fexceptions
LOCAL_CPPFLAGS := -w -s -fvisibility=default -pthread -Wall -fpack-struct=1 -O2 -std=c++14 -fexceptions -ferror-limit=5
#-fvisibility=hidden

LOCAL_C_INCLUDES := $(LOCAL_PATH)/vendor/json/json
LOCAL_SHARED_LIBRARIES := jsoncpp

include $(BUILD_SHARED_LIBRARY)