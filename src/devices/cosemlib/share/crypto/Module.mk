
LOCAL_DIR = $(call my-dir)/

SOURCES += $(addprefix $(LOCAL_DIR),aes.c cipher.c cipher_wrap.c gcm.c sha256.c sha1.c md5.c)

