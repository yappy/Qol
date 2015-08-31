#pragma once

#include "util.hpp"
#include "exceptions.hpp"
#include <map>

namespace dx9lib {

	class ConfigFile : public Uncopyable {
	private:
		typedef std::map<tstring, tstring> map_type;
		static const int LINE_BUF_SIZE = 1024;
		const tstring m_fileName;
		const bool m_storeOnDestruct;
		map_type datamap;
	public:
		ConfigFile(const tstring &filename, bool storeOnDestruct = false);
		virtual ~ConfigFile();

		void load();
		void store();

		const tstring getString(const tstring &key, const tstring &def);
		int getInt(const tstring &key, int def);
		void setString(const tstring &key, const tstring &value);
		void setInt(const tstring &key, int value);
	};

}
