#include "sM3u8Util.h"


int main()
{
	LiveM3u8* m3u8 = createLiveM3u8(3);
	initLiveM3u8(m3u8, 10, "test", ".");
	updateLiveM3u8File(m3u8, 0, 9.99999);
	updateLiveM3u8File(m3u8, 0, 10.222);
	updateLiveM3u8File(m3u8, 1, 9.99999);
	updateLiveM3u8File(m3u8, 2, 9.99999);
	updateLiveM3u8File(m3u8, 0, 9.99999);
	updateLiveM3u8File(m3u8, 0, 3.33);
	destroy(m3u8);
	return 0;
}