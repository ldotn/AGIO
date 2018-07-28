#pragma once

namespace agio
{
	struct Settings
	{
		Settings() = delete;

		// Loads the settings from a cfg file
		static void LoadFromFile();
		
		// Controls the spread of the mutation of a parameter when shifting it
		// TODO : better docs?
		inline static float ParameterMutationSpread = 0.025f;
	};
}