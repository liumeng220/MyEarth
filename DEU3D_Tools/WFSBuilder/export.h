#ifndef WFS_BUILDER_EXPORT_H_A6C7FF88_6289_48F3_9D18_1E6E0BDE62DE_INCLUDE
#define WFS_BUILDER_EXPORT_H_A6C7FF88_6289_48F3_9D18_1E6E0BDE62DE_INCLUDE

#ifdef WFS_BUILDER_EXPORTS
    #define WFSBUILDER_API __declspec(dllexport)
#else
    #define WFSBUILDER_API __declspec(dllimport)
#endif


#endif //WFS_BUILDER_EXPORT_H_A6C7FF88_6289_48F3_9D18_1E6E0BDE62DE_INCLUDE

