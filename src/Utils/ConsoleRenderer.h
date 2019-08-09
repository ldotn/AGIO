#pragma once
#include "../rlutil-master/rlutil.h"
#include <set>

namespace agio
{
	struct ConsoleItem
	{
		int PosX, PosY;

		char Symbol;
		int Color; // from rlutil enum

		// Required by set
		bool operator<(const ConsoleItem& rhs) const
		{
			int this_idx = PosX + rlutil::tcols()*PosY;
			int other_idx = rhs.PosX + rlutil::tcols()*rhs.PosY;

			return this_idx < other_idx;
		}

		bool operator==(const ConsoleItem& rhs) const
		{
			// Two items are considered equal if they are going to be on the same position
			return PosX == rhs.PosX
				&& PosY == rhs.PosY;
		}
	};

	class ConsoleRenderer
	{
	private:
		std::set<ConsoleItem> PrevItems;
	public:
		void Render(const std::set<ConsoleItem>& Items);
	};
}