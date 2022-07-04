if (WIN32)
    message("## current platform is Win32")
    set(AVF_WIN32 TRUE)
elseif (UNIX)
    set(AVF_UNIX TRUE)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        message("## current platform is Linux")
        set(AVF_LINUX TRUE)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Android")
        message("## current platform is Android")
        set(AVF_ANDROID TRUE)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        message("## current platform is MacOS")
        set(AVF_MACOS TRUE)
    else()
        message("## current platform is Unix")
    endif ()
elseif (APPLE)
    message("## current platform is Apple")
    set(AVF_APPLE TRUE)
else ()

endif ()