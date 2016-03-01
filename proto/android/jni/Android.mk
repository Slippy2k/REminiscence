LOCAL_PATH := $(call my-dir)

RS_PATH := ../..

include $(CLEAR_VARS)
SRC := collision.cpp decode.cpp file.cpp game.cpp input.cpp mixer.cpp piege.cpp render.cpp resource_data.cpp resource_mac.cpp saveload.cpp scaler.cpp staticres.cpp stub.cpp util.cpp
LOCAL_MODULE := libfb
LOCAL_SRC_FILES := $(foreach S,$(SRC),$(RS_PATH)/$(S))
LOCAL_CFLAGS := -DUSE_GLES -DUSE_OPENSL_ES -I$(RS_PATH)
LOCAL_LDLIBS := -lGLESv1_CM -lOpenSLES -lz
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := fb-jni
LOCAL_SRC_FILES := $(RS_PATH)/fb-jni.cpp
LOCAL_CFLAGS    := -Werror -I$(RS_PATH)
LOCAL_LDLIBS    := -llog -ldl
include $(BUILD_SHARED_LIBRARY)

