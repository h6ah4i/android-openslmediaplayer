
MY_DIR := $(call my-dir)

# 
#
#
LOCAL_PATH := $(MY_DIR)

include $(CLEAR_VARS)

LOCAL_MODULE           := OpenSLMediaPlayerNativeAPIExample
LOCAL_SRC_FILES        := \
    src/main.cpp \
    src/app_engine.cpp \
    src/app_command_pipe.cpp \
    src/com_h6ah4i_android_example_nativeopenslmediaplayer_MainNativeActivity.cpp
LOCAL_LDLIBS           := -llog -landroid -lEGL -lGLESv1_CM
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := OpenSLMediaPlayer

include $(BUILD_SHARED_LIBRARY)

#
# native_app_glue
#
$(call import-module,android/native_app_glue)

#
# OpenSLMediaPlayer
#
OPENSLMEDIAPLAYER_TOP_DIR := $(MY_DIR)/../..
$(call import-add-path,$(OPENSLMEDIAPLAYER_TOP_DIR))
$(call import-module,library/jni)
