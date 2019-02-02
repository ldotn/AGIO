#include "SFMLRenderer.h"

using namespace agio;
using namespace std;
using namespace sf;

int SFMLRenderer::LoadSprite(const string& Name, const string& Path)
{
	Texture texture;
	if (!texture.loadFromFile(Path))
		return -1;

	auto tex_ptr = make_unique<sf::Texture>(move(texture));

	sf::Sprite sprite;
	sprite.setTexture(*tex_ptr);

	int id = Textures.size();
	Textures.emplace_back(move(tex_ptr));
	Sprites.push_back(move(sprite));
	
	SpritesIDMap[Name] = id;

	return id;
}

void SFMLRenderer::Render(sf::RenderWindow& Window, const std::vector<SFMLRenderer::Item>& ItemsToRender)
{
	Window.clear();

	for (auto item : ItemsToRender)
	{
		auto sprite = Sprites[item.SpriteID];
		auto size = sprite.getTexture()->getSize();
		sprite.setPosition(item.Position.x*size.x, item.Position.y*size.y);

		Window.draw(sprite);
	}

	Window.display();
}