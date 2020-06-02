#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace agio
{
	class SFMLRenderer
	{
	private:
		std::vector<std::unique_ptr<sf::Texture>> Textures;
		std::vector<sf::Sprite> Sprites;
		std::unordered_map<std::string, int> SpritesIDMap;
	public:
		// Loads a sprite from disk and returns it's ID
		int LoadSprite(const std::string& Name, const std::string& Path);

		// Gets the ID for a sprite
		int GetSpriteID(const std::string& Name)
		{
			auto iter = SpritesIDMap.find(Name);
			if (iter != SpritesIDMap.end())
				return iter->second;
			else
				return -1;
		}

		struct Item
		{
			sf::Vector2i Position;
			int SpriteID;
			sf::Color Tint = sf::Color::White;
		};

		void Render(sf::RenderWindow& Window, const std::vector<Item>& ItemsToRender, float Scale = 1.0f);
	};
}
