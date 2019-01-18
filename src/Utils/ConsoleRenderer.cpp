#include "ConsoleRenderer.h"
#include <algorithm>

using namespace agio;
using namespace std;

void ConsoleRenderer::Render(const set<ConsoleItem>& Items) 
{
	// Remove the elements that are no longer in the set
	for (auto item : PrevItems)
	{
		if (Items.find(item) == Items.end())
		{
			rlutil::locate(item.PosX, item.PosY);
			rlutil::setChar(' ');
		}
	}

	// Now print the new elements or update them
	for (auto item : Items)
	{
		auto old_iter = PrevItems.find(item);
		if (old_iter == PrevItems.end())
		{
			// New item, just print it
			rlutil::locate(item.PosX, item.PosY);
			rlutil::setColor(item.Color);
			rlutil::setChar(item.Symbol);
		}
		else
		{
			// Only print if it's different than before
			if (old_iter->Color != item.Color || old_iter->Symbol != item.Symbol)
			{
				rlutil::locate(item.PosX, item.PosY);
				rlutil::setColor(item.Color);
				rlutil::setChar(item.Symbol);
			}
		}
	}

	// Update items set
	PrevItems = Items;
}