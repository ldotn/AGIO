#include <string>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <algorithm>

namespace agio
{
	// General base loader
	// It loads settings from files in the format
	// variable = value
	class ConfigLoader
	{
	public:
		ConfigLoader(const std::string& Path)
		{
			std::ifstream file(Path);
			while (file.is_open() && !file.eof())
			{
				std::string name, dummy_, value;
				file >> name;
				if (std::find(name.begin(), name.end(), '#') != name.end())
				{
					while (!file.eof() && file.get() != '\n');
					continue; // lines starting with # are comments and ignored	
				}

				file >> dummy_; // delimiter
				file >> value;

				Values[name] = value;

				while (!file.eof() && file.get() != '\n');
			}
		}

		template<typename T>
		void LoadValue(T& Value, const std::string& Name)
		{
			auto iter = Values.find(Name);
			if (iter != Values.end())
			{
				std::stringstream ss;
				ss << iter->second;
				ss >> Value;
			}
		}
	private:
		std::unordered_map<std::string, std::string> Values;
	};
}