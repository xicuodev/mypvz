#include <graphics.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "png.h"

#define WIN_WIDTH   1020
#define WIN_HEIGHT  600

#define UNIT_SUNBALL 25

enum {PEASHOOTER, SUNFLOWER, PLANTS_COUNT}; // CHERRY, WALLNUT, POTATO, SNOW_PEASHOOTER, CHOMPER, 

IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCard[PLANTS_COUNT];
IMAGE* imgPlant[PLANTS_COUNT][20];
IMAGE imgSunball[29];
IMAGE imgZombie[22];
IMAGE imgBullet;
IMAGE imgBulletSplats[4];
IMAGE imgZombieDie[10];
IMAGE imgZombieChomp[21];

struct plant
{
	int type; // 当前植物类型·-1 is 未定义·0、1、2…… is 第 1、2、3…… 种植物
	int frameIndex; // 当前序列帧
	int row, col; // 所在行列
	int x, y;
	bool fire; // 是否开火
	int cold; // 冷却·开火
	int health; // HP
	bool catched; // 是否被捕获
	
};
struct plant map[5][9];

struct sunball
{
	int x, y;
	int frameIndex; // 当前序列帧
	int destY; // 阳光球的目标落点
	bool used; // 出入池
	int timer; // 计次计时器
	float xoff; // 收集时每单位时长 x 的偏移量
	float yoff; // 收集时每单位时长 y 的偏移量

};
struct sunball pool[10]; // 阳光球池

struct zombie
{
	int x, y;
	int row; // 所在行
	int speed; // 走路速度
	int frameIndex; // 当前序列帧
	bool used; // 出入池
	bool target; // 标记
	int health; // HP
	bool dead; // 嗝屁
	bool chomp; // 吃饭
	int damage; // 伤害
};
struct zombie zbpool[20];

struct bullet
{
	int x, y;
	int row; // 所在行
	int speed; // 飞行速度
	bool used; // 出入池
	int damage; // 伤害
	bool hit; // 是否击中
	int frameIndex; // 当前序列帧
	int cold; // 冷却·序列帧前进
};
struct bullet bpool[45 * 45];

int curX, curY; // 当前选中卡牌·跟随鼠标时的坐标
int curPlant; // 当前选中植物·-1 is 未选中·0、1、2…… is 第 1、2、3…… 种植物
int sunshine; // 阳光值
int rowZombie[5] = { 0 }; // 每行的可见僵尸数量

void colorPrintf(const char* text, int color)
{
	/* 0 = 黑色 1 = 蓝色 2 = 绿色 3 = 浅蓝色 4 = 红色 5 = 紫色 6 = 黄色 7 = 白色 8 = 灰色
	   9 = 淡蓝色 10 = 淡绿色 11 = 淡浅绿色 12 = 淡红色 13 = 淡紫色 14 = 淡黄色 15 = 亮白色 */

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | color);
	printf(text);
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 7);
}

bool fileExist(const char* name) // 判断文件是否存在
{
	FILE* fp = fopen(name, "r");
	if (fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

void drawZombie() {
	int zbCount = sizeof(zbpool) / sizeof(zbpool[0]);
	for (int j = 0; j < 5; j++)
	{
		for (int i = 0; i < zbCount; i++)
		{
			if (zbpool[i].used && zbpool[i].row == j)
			{
				IMAGE* img = (zbpool[i].dead) ? imgZombieDie : ((zbpool[i].chomp) ? imgZombieChomp : imgZombie); // 判断僵尸状态，据此分配变量
				img += zbpool[i].frameIndex; // 指针往前走至第 n 个序列帧

				// 基于真实位置·基准位置，对于不同状态计算不同的渲染位置
				int drawX = zbpool[i].x; // 一般状态·走路
				int drawY = zbpool[i].y;
				if (zbpool[i].chomp) // 吃饭状态
					drawX -= 75;
				else if (zbpool[i].dead) // 濒死状态
				{
					drawX -= 80;
					drawY -= 5;
				}
				drawAlpha(drawX, drawY, img);
			}
		}
	}
}

void drawSunball() {
	int sunballMax = sizeof(pool) / sizeof(pool[0]);
	for (int i = 0; i < sunballMax; i++)
	{
		if (pool[i].used || pool[i].xoff) // 阳光球是否被取用或被收集，是则渲染，否则不渲染
		{
			IMAGE* img = &imgSunball[pool[i].frameIndex];
			drawAlpha(pool[i].x, pool[i].y, img);
		}
	}
}

void drawBullet() {
	int bMax = sizeof(bpool) / sizeof(bpool[0]);
	for (int i = 0; i < bMax; i++)
	{
		if (bpool[i].used == false) continue;

		int x = bpool[i].x;
		int y = bpool[i].y;

		if (bpool[i].hit)
		{
			IMAGE* img = &imgBulletSplats[bpool[i].frameIndex];
			drawAlpha(x, y, img);
		}
		else
		{
			drawAlpha(x, y, &imgBullet);
		}
	}
}

void gameInit() {
    loadimage(&imgBg, "res/images/background1.jpg");
    loadimage(&imgBar, "res/images/SeedBank.png");
	memset(imgPlant, 0, sizeof(imgPlant));
	memset(map, 0, sizeof(map));
	char name[64];
	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		// 加载卡牌图像
		sprintf_s(name, sizeof(name), "res/seeds/seed-%d.png", i + 1);
		loadimage(&imgCard[i], name, 50, 70); // 卡牌尺寸

		// 加载植物自走序列帧
		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/plants/%d/%d.png", i, j + 1);
			if (fileExist(name))
			{
				imgPlant[i][j] = new IMAGE;
				loadimage(imgPlant[i][j], name);
			}
			else
			{
				break;
			}
		}
	}

	// 初始·空草皮
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			map[i][j].type = -1;
		}
	}
	curPlant = -1; // 初始·当前未选中植物
	sunshine = 50; // 初始·阳光值

	// 加载阳光球自走序列帧
	memset(pool, 0, sizeof(pool));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunball/%d.png", i + 1);
		loadimage(&imgSunball[i], name);
	}

	// 配置随机种子
	srand(time(NULL));

    initgraph(WIN_WIDTH, WIN_HEIGHT, 1); // 1 代表保留控制台界面

	// 设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 25; // 字高
	f.lfWidth = 10; // 字宽
	strcpy(f.lfFaceName, "Segoe UI"); // 字体名称
	// f.lfQuality = ANTIALIASED_QUALITY; // 抗锯齿
	settextstyle(&f);
	setbkmode(TRANSPARENT); // 背景透明
	setcolor(BLACK); // 字体颜色

	// 加载僵尸走路序列帧
	memset(zbpool, 0, sizeof(zbpool));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zombies/zb-walk/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}

	// 加载子弹图像
	memset(bpool, 0, sizeof(bpool));
	loadimage(&imgBullet, "res/images/ProjectilePea.png");

	// 加载子弹击中序列帧
	for (int i = 0; i < 4; i++)
	{
		sprintf_s(name, sizeof(name), "res/bullets/pea_splats-%d.png", i + 1);
		loadimage(&imgBulletSplats[i], name);
	}

	// 加载僵尸死亡序列帧
	for (int i = 0; i < 10; i++)
	{
		sprintf_s(name, sizeof(name), "res/zombies/zb-nohead-die/%d.png", i + 1);
		loadimage(&imgZombieDie[i], name);
	}

	// 加载僵尸吃饭序列帧
	for (int i = 0; i < 21; i++)
	{
		sprintf_s(name, sizeof(name), "res/zombies/zb-chomp/%d.png", i + 1);
		loadimage(&imgZombieChomp[i], name);
	}
}

void startUI() {
	IMAGE imgBg, imgMenu, imgMenuPressed;
	loadimage(&imgBg, "res/ui/menu.png");
	loadimage(&imgMenu, "res/ui/menu1.png");
	loadimage(&imgMenuPressed, "res/ui/menu2.png");
	int flag = 0;
	while (true)
	{
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		drawAlpha(475, 75, flag ? &imgMenuPressed : &imgMenu);
		ExMessage msg;
		if (peekmessage(&msg))
		{
			if (msg.x > 475 && msg.x < 805 &&
				msg.y > 75 && msg.y < 215)
			{
				flag = 1;
			}
			else
			{
				flag = 0;
			}
			if (msg.message == WM_LBUTTONUP && flag)
			{
				return;
			}
		}
		EndBatchDraw();
	}
}

void updatePlant() {
	static int count = 0;
	static int fre = 5;
	count++;
	if (count >= fre) // 自走动画·植物序列帧前进
	{
		count = 0;

		for (int i = 0; i < 5; i++) { for (int j = 0; j < 9; j++) {
			if (map[i][j].type > -1 && map[i][j].catched == false)
			{
				map[i][j].frameIndex++;
				if (imgPlant[map[i][j].type][map[i][j].frameIndex] == NULL) // 序列帧图像跑到头了
				{
					map[i][j].frameIndex = 0;
				}
			}
		} }
	}
}

void createSunball() {
	// 计次计时器，由下文循环本函数每指定时长执行 1 次，故计次即计时
	static int count = 0;
	static int fre = 500;
	count++;
	if (count >= fre)
	{
		// 重置随机次数 fre 和计次标 count
		fre = 500 + rand() % 501; // fre 取值范围：[500, 1000]
		count = 0;

		// 从阳光球池中依次取出第 1 个可用的阳光球
		int i;
		int sunballMax = sizeof(pool) / sizeof(pool[0]);
		for (i = 0; i < sunballMax && pool[i].used; i++);
		if (i >= sunballMax) return; // 池子空了
	
		// 设定阳光球属性
		pool[i].used = true;
		pool[i].frameIndex = 0;
		pool[i].x = 296 + rand() % (8 * 80); // x 取值范围：[256 + col / 2, 256 + 8.5 * col - 1]
		pool[i].y = 0;
		pool[i].destY = 85 + (rand() % 5) * 100; // destY 取值范围：[85, 85 + 4 * row], 且仅包括代入 row 可得的值
		pool[i].timer = 0;
		pool[i].xoff = 0;
		pool[i].yoff = 0;
	}
}

void updateSunball() {
	int sunballMax = sizeof(pool) / sizeof(pool[0]);
	for (int i = 0; i < sunballMax; i++)
	{
		if (pool[i].used)
		{
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // 自走动画·阳光球序列帧前进
			if (pool[i].timer == 0)
			{
				pool[i].y += 1; // 位移动画·阳光球下落
			}
			if (pool[i].y >= pool[i].destY) // 阳光球落地
			{
				pool[i].timer++; // 启动计次计时器·阳光球自动消失
				if (pool[i].timer > 500)
				{
					pool[i].used = false; // 弃用阳光球
				}
			}
		}
		else if (pool[i].xoff) // 弃用后执行收集动画
		{
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // 自走动画·阳光球序列帧前进

			pool[i].x -= pool[i].xoff;
			pool[i].y -= pool[i].yoff;

			// 设置每单位时长 x、y 的偏移量
			// 循环执行，使每次坐标更新时重新计算，以拮抗浮点数转换整数带来的误差累计
			// 收集匣中心坐标 (200, 0)
			float destX = 200;
			float destY = 0;
			float theta = atan((pool[i].y - destY) / (pool[i].x - destX)); // 始末点连线水平偏角 theta
			float loff = 10; // 始末点连线上的单位偏移量 loff
			pool[i].xoff = loff * cos(theta);
			pool[i].yoff = loff * sin(theta);

			if (pool[i].x <= destX || pool[i].y <= destY)
			{
				pool[i].xoff = 0;
				pool[i].yoff = 0;
				sunshine += UNIT_SUNBALL; // 完成收集
			}
		}
	}
}

void collectSunshine(ExMessage* msg) {
	int countSunball = sizeof(pool) / sizeof(pool[0]);
	int w = imgSunball[0].getwidth();
	int h = imgSunball[0].getheight();
	for (int i = 0; i < countSunball; i++)
	{
		if (pool[i].used)
		{
			int x = pool[i].x;
			int y = pool[i].y;
			if (msg->x > x && msg->x < x + w && msg->y > y && msg->y < y + h)
			{
				pool[i].used = false;

				// 设置每单位时长 x、y 的偏移量
				// 收集匣中心坐标 (200, 0)
				float destX = 200;
				float destY = 0;
				float theta = atan((y - destY) / (x - destX)); // 始末点连线水平偏角 theta
				float loff = 10; // 始末点连线上的单位偏移量 loff
				pool[i].xoff = loff * cos(theta);
				pool[i].yoff = loff * sin(theta);
			}
		}
	}
}

void createZombie() {
	static int count = 0;
	static int zbFre = 100;
	count++;
	if (count >= zbFre)
	{
		count = 0;
		zbFre = 100 - rand() % 101;

		int i;
		int zbMax = sizeof(zbpool) / sizeof(zbpool[0]);
		for (i = 0; i < zbMax && zbpool[i].used; i++);
		if (i >= zbMax)
			return; // 池子空了，终止本次函数呼叫

		memset(&zbpool[i], 0, sizeof(zbpool[i])); // 简单粗暴的初始化
		zbpool[i].used = true;
		zbpool[i].frameIndex = 0;
		zbpool[i].x = WIN_WIDTH;
		zbpool[i].row = rand() % 5;
		zbpool[i].y = 25 + zbpool[i].row * 100;
		zbpool[i].speed = 2;
		zbpool[i].health = 200;
		zbpool[i].damage = 1;
		zbpool[i].dead = false;
		zbpool[i].chomp = false;

		// 日志区
		char info[100];
		sprintf_s(info, sizeof(info), "【生成僵尸】从池中取出第 %d 名可用僵尸置于第 %d 行;\n", i, zbpool[i].row + 1);
		colorPrintf(info, 13);
	}
}

void updateZombie() {
	int zbMax = sizeof(zbpool) / sizeof(zbpool[0]);

	static int count = 0;
	static int fre = 8;
	count++;
	if (count >= fre)
	{
		count = 0;

		for (int i = 0; i < zbMax; i++)
		{
			if (zbpool[i].used)
			{
				zbpool[i].x -= zbpool[i].speed; // 位移动画·僵尸走路

				if (zbpool[i].x <= 170)
				{
					zbpool[i].used = false;

					// 游戏结束，待优化
					printf("GAME OVER!\n");
					MessageBox(NULL, "GAME OVER!", "GAME OVER!", 0);
					exit(0);
				}
			}
		}
	}

	static int countF = 0;
	static int freF = 5;
	countF++;
	if (countF >= freF)
	{
		countF = 0;

		for (int i = 0; i < zbMax; i++) // 遍历僵尸池
		{
			if (zbpool[i].used)
			{
				if (zbpool[i].dead) // 死了
				{
					zbpool[i].frameIndex++; // 自走动画·僵尸死亡序列帧前进
					if (zbpool[i].frameIndex >= 10)
					{
						zbpool[i].used = false;
					}
				}
				else if (zbpool[i].chomp) // 吃饭
				{
					zbpool[i].frameIndex = (zbpool[i].frameIndex + 1) % 21; // 自走动画·循环·僵尸吃饭序列帧前进
				}
				else // 正常状态·走路
				{
					zbpool[i].frameIndex = (zbpool[i].frameIndex + 1) % 22; // 自走动画·循环·僵尸走路序列帧前进
				}
			}
		}
	}
}

void createBullet() {
	// 对每名僵尸判断开火
	int zbCount = sizeof(zbpool) / sizeof(zbpool[0]);
	int bulletMax = sizeof(bpool) / sizeof(bpool[0]);
	int visibleX = WIN_WIDTH - 40;
	for (int i = 0; i < zbCount; i++)
	{
		if (zbpool[i].used && zbpool[i].dead == false && zbpool[i].target == false && zbpool[i].x <= visibleX)
		{
			zbpool[i].target = true;
			rowZombie[zbpool[i].row]++;

			// 日志区
			char info[100];
			sprintf_s(info, sizeof(info), "【僵尸参战】第 %d 名僵尸参战，现在第 %d 行有 %d 名可见僵尸;\n", i, zbpool[i].row + 1, rowZombie[zbpool[i].row]);
			colorPrintf(info, 6);
		}
	}

	// 启动/关闭开火植物
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == PEASHOOTER)
			{
				if (rowZombie[i] > 0 && map[i][j].fire == false)
				{
					map[i][j].fire = true;

					// 日志区
					char info[100];
					sprintf_s(info, sizeof(info), "【开火检测】行列坐标 (%d, %d) 的豌豆射手已开火;\n", i + 1, j + 1);
					colorPrintf(info, 3);
				}
				else if (rowZombie[i] <= 0 && map[i][j].fire)
				{
					map[i][j].fire = false;

					// 日志区
					char info[100];
					sprintf_s(info, sizeof(info), "【停火检测】行列坐标 (%d, %d) 的豌豆射手已停火;\n", i + 1, j + 1);
					colorPrintf(info, 2);
				}
			}
		}
	}

	// 开火
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == PEASHOOTER && map[i][j].fire)
			{
				int fre = 50;
				map[i][j].cold++;
				if (map[i][j].cold >= fre)
				{
					map[i][j].cold = 0;

					int k;
					for (k = 0; k < bulletMax && bpool[k].used; k++);
					if (k >= bulletMax) return;

					bpool[k].used = true;
					bpool[k].row = i;
					bpool[k].speed = 4;
					bpool[k].x = 256 + j * 80 + imgPlant[map[i][j].type][0]->getwidth() - 10;
					bpool[k].y = 90 + i * 100 + 5;
					bpool[k].damage = 20;
					bpool[k].hit = false;
					bpool[k].frameIndex = 0;
					bpool[k].cold = 0;

					// 日志区
					// printf("【配备子弹】给行列坐标 (%d, %d) 的豌豆射手配备一颗子弹;\n", i + 1, j + 1);
				}
			}
		}
	}
}

void updateBullet() {
	int bMax = sizeof(bpool) / sizeof(bpool[0]);
	for (int i = 0; i < bMax; i++)
	{
		if (bpool[i].used)
		{
			bpool[i].x += bpool[i].speed;
			if (bpool[i].x >= WIN_WIDTH)
			{
				bpool[i].used = false;
			}

			// 子弹击中动画
			static int fre = 4;
			bpool[i].cold++;
			if (bpool[i].cold >= fre)
			{
				bpool[i].cold = 0;

				if (bpool[i].hit)
				{
					bpool[i].frameIndex++;
					if (bpool[i].frameIndex > 4)
					{
						bpool[i].frameIndex = 0;
						bpool[i].used = false;
						bpool[i].hit = false;
					}
				}
			}
		}
	}
}

void checkBullet2Zombie() {
	int bCount = sizeof(bpool) / sizeof(bpool[0]);
	int zbCount = sizeof(zbpool) / sizeof(zbpool[0]); 
	for (int i = 0; i < bCount; i++)
	{
		if (bpool[i].used == false || bpool[i].hit)\
			continue;

		for (int j = 0; j < zbCount; j++)
		{
			if (zbpool[j].used == false)
				continue;

			// 僵尸信息
			int xLeft = zbpool[j].x + 15;
			int xRight = zbpool[j].x + 55;
			int rowTarget = zbpool[j].row;

			// 子弹信息
			int x = bpool[i].x;
			int row = bpool[i].row;

			if (x > xLeft && x < xRight && row == rowTarget && zbpool[j].dead == false)
			{
				zbpool[j].health -= bpool[i].damage;
				if (zbpool[j].health <= 0) // 僵尸死亡
				{
					zbpool[j].speed = 0;
					zbpool[j].dead = true;
					zbpool[j].health = 200;
					zbpool[j].frameIndex = 0;
					if (zbpool[j].target)
					{
						zbpool[j].target = false;
						rowZombie[rowTarget]--;
					}

					// 日志区
					char info[100];
					sprintf_s(info, sizeof(info), "【僵尸战死】第 %d 名僵尸战死，现在第 %d 行有 %d 名可见僵尸;\n", i, rowTarget + 1, rowZombie[rowTarget]);
					colorPrintf(info, 4);
				}

				bpool[i].hit = true;
				bpool[i].speed = 0;

				break;
			}
		}
	}
}

void checkZombie2Plant() {
	int zbCount = sizeof(zbpool) / sizeof(zbpool[0]);
	for (int i = 0; i < zbCount; i++) // 遍历僵尸池
	{
		if (zbpool[i].dead)
			continue; // 跳过死僵尸

		int row = zbpool[i].row;
		for (int col = 0; col < 9; col++) // 遍历该行每列草皮
		{
			if (map[row][col].type == -1 && zbpool[i].chomp == false)
				continue; // 跳过空草皮

			int xPlantL = map[row][col].x + 10;
			int xPlantR = map[row][col].x + 60;
			int xZombieL = zbpool[i].x;
			if (xZombieL > xPlantL && xZombieL < xPlantR)
			{
				if (map[row][col].type == -1 && zbpool[i].chomp) // 规避有些僵尸反应慢（吃完了还在吃）的情况
				{
					zbpool[i].chomp = false;
					zbpool[i].speed = 2;
					zbpool[i].frameIndex = 0;
				}
				else if (map[row][col].catched && zbpool[i].chomp)
				{
					map[row][col].health -= zbpool[i].damage; // 植物掉血

					// 日志区
					printf("【植物掉血】行列坐标 (%d, %d) 的植物掉血，目前血量为 %d;\n", row + 1, col + 1, map[row][col].health);

					if (map[row][col].health <= 0) // 植物死亡
					{
						map[row][col].type = -1;
						map[row][col].catched = false;

						zbpool[i].chomp = false;
						zbpool[i].speed = 2;
						zbpool[i].frameIndex = 0;
					}
				}
				else
				{
					map[row][col].catched = true;

					zbpool[i].chomp = true;
					zbpool[i].speed = 0;
					zbpool[i].frameIndex = 0;

					// 日志区
					printf("【僵尸吃饭】第 %d 名僵尸开始吃饭，饭是行列坐标 (%d, %d) 的植物;\n", i, row + 1, col + 1);
				}
			}
		}
	}
}

void collisionCheck() {
	checkBullet2Zombie();
	checkZombie2Plant();
}

void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) // 嗅探鼠标消息
	{
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x > 278 && msg.x < 278 + 51 * PLANTS_COUNT && msg.y > 8 && msg.y < 78)
			{
				// 获取选中卡牌序号
				int index = (msg.x - 278) / 51;
				curPlant = index;

				status = 1;
				curX = msg.x;
				curY = msg.y;
			}
			else if (msg.x > 256 && msg.y > 90 && status == 1)
			{
				int row = (msg.y - 90) / 100;
				int col = (msg.x - 256) / 80;
				if (map[row][col].type == -1)
				{
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
					map[row][col].row = row;
					map[row][col].col = col;
					map[row][col].cold = 0;
					map[row][col].health = 300;
					map[row][col].catched = false;
				}
				curPlant = -1;
				status = 0;
			}
			else
			{
				curPlant = -1;
				status = 0;
			}
			collectSunshine(&msg);
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP && status == 1)
		{
			if (msg.x > 256 && msg.y > 90)
			{
				int row = (msg.y - 90) / 100;
				int col = (msg.x - 256) / 80;
				if (map[row][col].type == -1)
				{
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
					map[row][col].row = row;
					map[row][col].col = col;
					map[row][col].cold = 0;
					map[row][col].health = 300;
					map[row][col].catched = false;
				}
				curPlant = -1;
				status = 0;
			}
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			// printf("px(%d, %d)\n", msg.x, msg.y); // 土法·鼠标法测坐标
		}
	}
}

void updateGame() {
	updatePlant();

	createSunball();
	updateSunball();

	createZombie();
	updateZombie();

	createBullet();
	updateBullet();
	collisionCheck(); // 碰撞检测
}

void updateWin() {
	//双缓冲·解决画面更新时闪烁
	BeginBatchDraw();//开始双缓冲

    putimage(0, 0, &imgBg); // 渲染·地图
	
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > -1)
			{
				int x = 256 + j * 80;
				int y = 90 + i * 100;
				map[i][j].x = x;
				map[i][j].y = y;
				drawAlpha(x, y, imgPlant[map[i][j].type][map[i][j].frameIndex]); // 渲染·种下的植物
			}
		}
	}

    drawAlpha(200, 0, &imgBar); // 渲染·卡槽

	drawZombie(); // 渲染·僵尸

	drawBullet(); // 渲染·子弹

	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		int x = 278 + i * 51;
		int y = 8;
		drawAlpha(x, y, &imgCard[i]); // 渲染·卡牌
	}

	char pointText[8];
	sprintf_s(pointText, sizeof(pointText), "%d", sunshine);
	outtextxy(222, 58, pointText); // 渲染·阳光值文本

	if (curPlant > -1)
	{
		IMAGE* img = imgPlant[curPlant][0];
		int curPX = curX - img->getwidth() / 2;
		int curPY = curY - img->getheight() / 2;
		drawAlpha(curPX, curPY, img); //渲染·跟随鼠标移动的待命植物
	}

	// 渲染·阳光球
	drawSunball();

	// 渲染·测试图像
	// drawAlpha(500, 200, &imgZombie[0]);
	// drawAlpha(420, 195, &imgZombieDie[0]); // x - 80, y - 5
	// drawAlpha(425, 200, &imgZombieChomp[0]); // x - 75, y

	EndBatchDraw();//结束双缓冲
}

int main() {
    gameInit();
	startUI();
	int timer = 0;
	bool flag = true;
	while (true)
	{
		userClick();
		timer += getDelay();
		if (timer > 10) // 两次更新之间累计超过指定时长，flag 标记才为真 
		{
			flag = true;
			timer = 0;
		}
		if (flag) // 如果 flag 标记为真，执行一次游戏更新
		{
			flag = false;
			updateGame(); // 非玩家操作反馈，顺滑养眼
		}
		updateWin(); // 玩家操作反馈，丝滑跟手
	}
    system("pause");
    return 0;
}