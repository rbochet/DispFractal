LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := random_image
LOCAL_CFLAGS    := -Werror -lm
LOCAL_SRC_FILES := random-image.c
LOCAL_LDLIBS    := -llog 

include $(BUILD_SHARED_LIBRARY)