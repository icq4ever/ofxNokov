meta:
	ADDON_NAME = ofxNokov
	ADDON_DESCRIPTION = openFrameworks addon for Nokov (XING) motion capture system
	ADDON_AUTHOR = icq4ever
	ADDON_TAGS = "mocap" "motion capture" "nokov" "xing" "rigid body"
	ADDON_URL = https://github.com/icq4ever/ofxNokov

common:
	ADDON_INCLUDES = src/
	ADDON_INCLUDES += libs/nokov_sdk/include/

linux64:
	ADDON_LDFLAGS = -lnokov_sdk
	ADDON_LIBS = libs/nokov_sdk/lib/linux64/libnokov_sdk.so
	ADDON_DEFINES = _LINUX

linux:
	ADDON_LDFLAGS = -lnokov_sdk
	ADDON_LIBS = libs/nokov_sdk/lib/linux64/libnokov_sdk.so
	ADDON_DEFINES = _LINUX

osx:
	ADDON_LIBS = libs/nokov_sdk/lib/osx/libnokov_sdk.dylib
	ADDON_DEFINES = _LINUX

linuxarmv6l:
	ADDON_LDFLAGS = -lnokov_sdk
	ADDON_LIBS = libs/nokov_sdk/lib/linuxarmv6l/libnokov_sdk.so
	ADDON_DEFINES = _LINUX

linuxaarch64:
	ADDON_LDFLAGS = -lnokov_sdk
	ADDON_LIBS = libs/nokov_sdk/lib/linuxaarch64/libnokov_sdk.so
	ADDON_DEFINES = _LINUX

vs:
	ADDON_LIBS = libs/nokov_sdk/lib/vs/nokov_sdk.lib
