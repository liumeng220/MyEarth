#ifndef DEUDB_CLIENT_EXPORT_H_162729C1_801A_4CBB_9FC7_E44CEFE2AF97_INCLUDE
#define DEUDB_CLIENT_EXPORT_H_162729C1_801A_4CBB_9FC7_E44CEFE2AF97_INCLUDE

#ifdef WIN32
	#ifdef DEUDB_PROXY_EXPORTS
		#define DEUDB_PROXY_EXPORT    __declspec(dllexport)
	#else
		#define DEUDB_PROXY_EXPORT    __declspec(dllimport)
	#endif
#else
	#define DEUDB_PROXY_EXPORT
#endif

#endif
