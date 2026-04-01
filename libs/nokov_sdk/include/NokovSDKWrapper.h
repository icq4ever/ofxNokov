#pragma once

// Platform abstraction for Nokov/Seeker SDK
// Linux/Windows: NokovSDKClient  |  Mac: SeekerSDKClient

#if defined(__APPLE__)
    #define _LINUX
    #include "SeekerSDKTypes.h"
    #include "SeekerSDKClient.h"
    #include "SeekerSDKCAPI.h"

    // Alias SeekerSDKClient to NokovSDKClient for unified usage
    typedef SeekerSDKClient NokovSDKClientBase;

    inline void NokovSDK_GetVersion(unsigned char ver[4]) {
        // SeekerSDK uses instance method, not a static function
    }

#elif defined(_WIN32)
    #include "NokovSDKTypes.h"
    #include "NokovSDKClient.h"
    #include "NokovSDKCAPI.h"

    typedef NokovSDKClient NokovSDKClientBase;

#else
    // Linux
    #ifndef _LINUX
        #define _LINUX
    #endif
    #include "NokovSDKTypes.h"
    #include "NokovSDKClient.h"
    #include "NokovSDKCAPI.h"

    typedef NokovSDKClient NokovSDKClientBase;

#endif
