#include "App.h"

int main(int argc, char* argv[]) {
	
	auto app = App::Make();
	app->Run();
	
	return 0;
}