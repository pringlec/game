#include "Header.hpp"

int main(void)
{
	Game * theGame = new Game();

	theGame->initialise();
	theGame->run();
	theGame->shutdown();

	return 0;
}