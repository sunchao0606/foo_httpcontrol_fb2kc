#define STR2(s) #s
#define STR(s) STR2(s)

#define VERSION_MAJOR               0
#define VERSION_MINOR               97
#define VERSION_REVISION            28
#define VERSION_BUILD               0

#define VER_PRODUCTNAME_STR         "foo_httpcontrol"
#define VER_FILE_DESCRIPTION_STR    "HTTP Control"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR        STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_REVISION) "." STR(VERSION_BUILD)
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   VER_PRODUCTNAME_STR ".dll"
#define VER_COPYRIGHT_STR           "(C) oblikoamorale"
#define VER_SUPPORT_FORUM			"https://hydrogenaud.io/index.php/topic,62218.0.html"

#ifdef _DEBUG
#define VER_VER_DEBUG             VS_FF_DEBUG
#else
#define VER_VER_DEBUG             0
#endif

#define VER_FILEFLAGS               VER_VER_DEBUG
