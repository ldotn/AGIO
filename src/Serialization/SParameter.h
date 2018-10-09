#pragma once

#include <boost/serialization/access.hpp>

namespace agio
{
	class SParameter
	{
	public:
		int id;
		float value;

		SParameter() {};
		SParameter(int id, float value)
		{
			this->id = id;
			this->value = value;
		}

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & id;
			ar & value;
		}
	};
}
