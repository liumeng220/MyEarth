#pragma once
#include "IMercatorDriver.h"

namespace deues
{
	class MercatorDriver :	public IMercatorDriver
	{
	public:
		explicit MercatorDriver(void);
		virtual ~MercatorDriver(void);
		virtual bool initialize(const std::string& strUrl, const std::string& strVersion);
		virtual std::string getVersion() const { return m_strVersion; }
		virtual std::string getUrl() const { return m_strUrl; }
		ITileSet* createTileSet(std::string strMapService,std::string strMapStyle);
		virtual bool getMapServices(std::vector<DEUMorcatorInfo>& arrServices);
	private:
		std::string m_strUrl;
		std::string m_strVersion;
	};
}
