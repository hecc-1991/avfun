#include "AVPlayer.h"

using avf::AVPlayer;

int main(int argc, char* argv[]) {

	AVPlayer player;

	player.OpenVideo("/Users/hecc/Documents/hecc/dev/avfun/resources/love2.mp4");

    player.Start();

}