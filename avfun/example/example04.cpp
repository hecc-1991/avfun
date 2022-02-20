#include "AVPlayer.h"

using avf::AVPlayer;

int main(int argc, char* argv[]) {

	AVPlayer player;

	player.OpenVideo("/Users/hecc/Documents/hecc/dev/avfun/resources/yangguang.mp4");

    player.Start();

}