#pragma once

#include <vector>

#include <boost/serialization/access.hpp>

namespace agio
{
    struct ComponentRef
    {
        int GroupID;
        int ComponentID;

        bool operator==(const ComponentRef & other) const
        {
            return GroupID     == other.GroupID &&
                   ComponentID == other.ComponentID;
        }

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar & GroupID;
            ar & ComponentID;
        }
    };
    typedef std::vector<ComponentRef> MorphologyTag;
}

// This needs to be outside of the agio namespace
namespace std
{
    template <>
    struct hash<agio::MorphologyTag>
    {
        std::size_t operator()(const agio::MorphologyTag& k) const
        {
            // Ref : [https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector]
            std::size_t seed = k.size();

            //for (auto [gid, cid] : k)
			for(auto component : k)
            {
                seed ^= component.GroupID + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                seed ^= component.ComponentID + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }

            return seed;
        }
    };
}