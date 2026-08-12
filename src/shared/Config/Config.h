/*
 * Game server
 * Copyright (C) 2010 Miroslav 'Wayland' Kudrnac
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "../Defines.h"

struct ConfigSetting
{
    explicit ConfigSetting() : AsInt(0), AsFloat(0.0f), AsBool(false)
    {
        
    }
    
    std::string AsString;
	int         AsInt;
	float       AsFloat;
	bool        AsBool;
};

typedef	std::map<std::string, ConfigSetting>    ConfigBlock;
typedef std::map<std::string, ConfigBlock>      ConfigSettings;

class ConfigFile
{
public:
	explicit ConfigFile();
	~ConfigFile();

	bool SetSource(const char *file);
	ConfigSetting * GetSetting(const char * Block, const char * Setting);

	bool GetString(const char * block, const char* name, std::string *value);
	std::string GetStringDefault(const char * block, const char* name, const char* def);
	std::string GetStringVA(const char * block, const char* def, const char * name, ...);
	bool GetString(const char * block, char * buffer, const char * name, const char * def, size_t len);

	bool GetBool(const char * block, const char* name, bool *value);
	bool GetBoolDefault(const char * block, const char* name, const bool def);

	bool GetInt(const char * block, const char* name, int *value);
	int GetIntDefault(const char * block, const char* name, const int def);
	int GetIntVA(const char * block, int def, const char* name, ...);

	bool GetFloat(const char * block, const char* name, float *value);
	float GetFloatDefault(const char * block, const char* name, const float def);
	float GetFloatVA(const char * block, float def, const char* name, ...);
    
    INLINE const std::string &GetConfigFilePath() const
    {
        return m_rFilePath;
    }

private:
	ConfigSettings  m_settings;
    std::string     m_rFilePath;
};


class ConfigMgr
{
public:
	ConfigFile MainConfig;
};

extern ConfigMgr g_rConfig;

#endif
