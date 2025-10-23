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
	int type; // ��ǰֲ�����͡�-1 is δ���塤0��1��2���� is �� 1��2��3���� ��ֲ��
	int frameIndex; // ��ǰ����֡
	int row, col; // ��������
	int x, y;
	bool fire; // �Ƿ񿪻�
	int cold; // ��ȴ������
	int health; // HP
	bool catched; // �Ƿ񱻲���
	
};
struct plant map[5][9];

struct sunball
{
	int x, y;
	int frameIndex; // ��ǰ����֡
	int destY; // �������Ŀ�����
	bool used; // �����
	int timer; // �ƴμ�ʱ��
	float xoff; // �ռ�ʱÿ��λʱ�� x ��ƫ����
	float yoff; // �ռ�ʱÿ��λʱ�� y ��ƫ����

};
struct sunball pool[10]; // �������

struct zombie
{
	int x, y;
	int row; // ������
	int speed; // ��·�ٶ�
	int frameIndex; // ��ǰ����֡
	bool used; // �����
	bool target; // ���
	int health; // HP
	bool dead; // ��ƨ
	bool chomp; // �Է�
	int damage; // �˺�
};
struct zombie zbpool[20];

struct bullet
{
	int x, y;
	int row; // ������
	int speed; // �����ٶ�
	bool used; // �����
	int damage; // �˺�
	bool hit; // �Ƿ����
	int frameIndex; // ��ǰ����֡
	int cold; // ��ȴ������֡ǰ��
};
struct bullet bpool[45 * 45];

int curX, curY; // ��ǰѡ�п��ơ��������ʱ������
int curPlant; // ��ǰѡ��ֲ�-1 is δѡ�С�0��1��2���� is �� 1��2��3���� ��ֲ��
int sunshine; // ����ֵ
int rowZombie[5] = { 0 }; // ÿ�еĿɼ���ʬ����

void colorPrintf(const char* text, int color)
{
	/* 0 = ��ɫ 1 = ��ɫ 2 = ��ɫ 3 = ǳ��ɫ 4 = ��ɫ 5 = ��ɫ 6 = ��ɫ 7 = ��ɫ 8 = ��ɫ
	   9 = ����ɫ 10 = ����ɫ 11 = ��ǳ��ɫ 12 = ����ɫ 13 = ����ɫ 14 = ����ɫ 15 = ����ɫ */

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | color);
	printf(text);
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 7);
}

bool fileExist(const char* name) // �ж��ļ��Ƿ����
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
				IMAGE* img = (zbpool[i].dead) ? imgZombieDie : ((zbpool[i].chomp) ? imgZombieChomp : imgZombie); // �жϽ�ʬ״̬���ݴ˷������
				img += zbpool[i].frameIndex; // ָ����ǰ������ n ������֡

				// ������ʵλ�á���׼λ�ã����ڲ�ͬ״̬���㲻ͬ����Ⱦλ��
				int drawX = zbpool[i].x; // һ��״̬����·
				int drawY = zbpool[i].y;
				if (zbpool[i].chomp) // �Է�״̬
					drawX -= 75;
				else if (zbpool[i].dead) // ����״̬
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
		if (pool[i].used || pool[i].xoff) // �������Ƿ�ȡ�û��ռ���������Ⱦ��������Ⱦ
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
		// ���ؿ���ͼ��
		sprintf_s(name, sizeof(name), "res/seeds/seed-%d.png", i + 1);
		loadimage(&imgCard[i], name, 50, 70); // ���Ƴߴ�

		// ����ֲ����������֡
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

	// ��ʼ���ղ�Ƥ
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			map[i][j].type = -1;
		}
	}
	curPlant = -1; // ��ʼ����ǰδѡ��ֲ��
	sunshine = 50; // ��ʼ������ֵ

	// ������������������֡
	memset(pool, 0, sizeof(pool));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunball/%d.png", i + 1);
		loadimage(&imgSunball[i], name);
	}

	// �����������
	srand(time(NULL));

    initgraph(WIN_WIDTH, WIN_HEIGHT, 1); // 1 ����������̨����

	// ��������
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 25; // �ָ�
	f.lfWidth = 10; // �ֿ�
	strcpy(f.lfFaceName, "Segoe UI"); // ��������
	// f.lfQuality = ANTIALIASED_QUALITY; // �����
	settextstyle(&f);
	setbkmode(TRANSPARENT); // ����͸��
	setcolor(BLACK); // ������ɫ

	// ���ؽ�ʬ��·����֡
	memset(zbpool, 0, sizeof(zbpool));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zombies/zb-walk/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}

	// �����ӵ�ͼ��
	memset(bpool, 0, sizeof(bpool));
	loadimage(&imgBullet, "res/images/ProjectilePea.png");

	// �����ӵ���������֡
	for (int i = 0; i < 4; i++)
	{
		sprintf_s(name, sizeof(name), "res/bullets/pea_splats-%d.png", i + 1);
		loadimage(&imgBulletSplats[i], name);
	}

	// ���ؽ�ʬ��������֡
	for (int i = 0; i < 10; i++)
	{
		sprintf_s(name, sizeof(name), "res/zombies/zb-nohead-die/%d.png", i + 1);
		loadimage(&imgZombieDie[i], name);
	}

	// ���ؽ�ʬ�Է�����֡
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
	if (count >= fre) // ���߶�����ֲ������֡ǰ��
	{
		count = 0;

		for (int i = 0; i < 5; i++) { for (int j = 0; j < 9; j++) {
			if (map[i][j].type > -1 && map[i][j].catched == false)
			{
				map[i][j].frameIndex++;
				if (imgPlant[map[i][j].type][map[i][j].frameIndex] == NULL) // ����֡ͼ���ܵ�ͷ��
				{
					map[i][j].frameIndex = 0;
				}
			}
		} }
	}
}

void createSunball() {
	// �ƴμ�ʱ����������ѭ��������ÿָ��ʱ��ִ�� 1 �Σ��ʼƴμ���ʱ
	static int count = 0;
	static int fre = 500;
	count++;
	if (count >= fre)
	{
		// ����������� fre �ͼƴα� count
		fre = 500 + rand() % 501; // fre ȡֵ��Χ��[500, 1000]
		count = 0;

		// ���������������ȡ���� 1 �����õ�������
		int i;
		int sunballMax = sizeof(pool) / sizeof(pool[0]);
		for (i = 0; i < sunballMax && pool[i].used; i++);
		if (i >= sunballMax) return; // ���ӿ���
	
		// �趨����������
		pool[i].used = true;
		pool[i].frameIndex = 0;
		pool[i].x = 296 + rand() % (8 * 80); // x ȡֵ��Χ��[256 + col / 2, 256 + 8.5 * col - 1]
		pool[i].y = 0;
		pool[i].destY = 85 + (rand() % 5) * 100; // destY ȡֵ��Χ��[85, 85 + 4 * row], �ҽ��������� row �ɵõ�ֵ
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
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // ���߶���������������֡ǰ��
			if (pool[i].timer == 0)
			{
				pool[i].y += 1; // λ�ƶ���������������
			}
			if (pool[i].y >= pool[i].destY) // ���������
			{
				pool[i].timer++; // �����ƴμ�ʱ�����������Զ���ʧ
				if (pool[i].timer > 500)
				{
					pool[i].used = false; // ����������
				}
			}
		}
		else if (pool[i].xoff) // ���ú�ִ���ռ�����
		{
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // ���߶���������������֡ǰ��

			pool[i].x -= pool[i].xoff;
			pool[i].y -= pool[i].yoff;

			// ����ÿ��λʱ�� x��y ��ƫ����
			// ѭ��ִ�У�ʹÿ���������ʱ���¼��㣬���׿�������ת����������������ۼ�
			// �ռ�ϻ�������� (200, 0)
			float destX = 200;
			float destY = 0;
			float theta = atan((pool[i].y - destY) / (pool[i].x - destX)); // ʼĩ������ˮƽƫ�� theta
			float loff = 10; // ʼĩ�������ϵĵ�λƫ���� loff
			pool[i].xoff = loff * cos(theta);
			pool[i].yoff = loff * sin(theta);

			if (pool[i].x <= destX || pool[i].y <= destY)
			{
				pool[i].xoff = 0;
				pool[i].yoff = 0;
				sunshine += UNIT_SUNBALL; // ����ռ�
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

				// ����ÿ��λʱ�� x��y ��ƫ����
				// �ռ�ϻ�������� (200, 0)
				float destX = 200;
				float destY = 0;
				float theta = atan((y - destY) / (x - destX)); // ʼĩ������ˮƽƫ�� theta
				float loff = 10; // ʼĩ�������ϵĵ�λƫ���� loff
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
			return; // ���ӿ��ˣ���ֹ���κ�������

		memset(&zbpool[i], 0, sizeof(zbpool[i])); // �򵥴ֱ��ĳ�ʼ��
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

		// ��־��
		char info[100];
		sprintf_s(info, sizeof(info), "�����ɽ�ʬ���ӳ���ȡ���� %d �����ý�ʬ���ڵ� %d ��;\n", i, zbpool[i].row + 1);
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
				zbpool[i].x -= zbpool[i].speed; // λ�ƶ�������ʬ��·

				if (zbpool[i].x <= 170)
				{
					zbpool[i].used = false;

					// ��Ϸ���������Ż�
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

		for (int i = 0; i < zbMax; i++) // ������ʬ��
		{
			if (zbpool[i].used)
			{
				if (zbpool[i].dead) // ����
				{
					zbpool[i].frameIndex++; // ���߶�������ʬ��������֡ǰ��
					if (zbpool[i].frameIndex >= 10)
					{
						zbpool[i].used = false;
					}
				}
				else if (zbpool[i].chomp) // �Է�
				{
					zbpool[i].frameIndex = (zbpool[i].frameIndex + 1) % 21; // ���߶�����ѭ������ʬ�Է�����֡ǰ��
				}
				else // ����״̬����·
				{
					zbpool[i].frameIndex = (zbpool[i].frameIndex + 1) % 22; // ���߶�����ѭ������ʬ��·����֡ǰ��
				}
			}
		}
	}
}

void createBullet() {
	// ��ÿ����ʬ�жϿ���
	int zbCount = sizeof(zbpool) / sizeof(zbpool[0]);
	int bulletMax = sizeof(bpool) / sizeof(bpool[0]);
	int visibleX = WIN_WIDTH - 40;
	for (int i = 0; i < zbCount; i++)
	{
		if (zbpool[i].used && zbpool[i].dead == false && zbpool[i].target == false && zbpool[i].x <= visibleX)
		{
			zbpool[i].target = true;
			rowZombie[zbpool[i].row]++;

			// ��־��
			char info[100];
			sprintf_s(info, sizeof(info), "����ʬ��ս���� %d ����ʬ��ս�����ڵ� %d ���� %d ���ɼ���ʬ;\n", i, zbpool[i].row + 1, rowZombie[zbpool[i].row]);
			colorPrintf(info, 6);
		}
	}

	// ����/�رտ���ֲ��
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == PEASHOOTER)
			{
				if (rowZombie[i] > 0 && map[i][j].fire == false)
				{
					map[i][j].fire = true;

					// ��־��
					char info[100];
					sprintf_s(info, sizeof(info), "�������⡿�������� (%d, %d) ���㶹�����ѿ���;\n", i + 1, j + 1);
					colorPrintf(info, 3);
				}
				else if (rowZombie[i] <= 0 && map[i][j].fire)
				{
					map[i][j].fire = false;

					// ��־��
					char info[100];
					sprintf_s(info, sizeof(info), "��ͣ���⡿�������� (%d, %d) ���㶹������ͣ��;\n", i + 1, j + 1);
					colorPrintf(info, 2);
				}
			}
		}
	}

	// ����
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

					// ��־��
					// printf("���䱸�ӵ������������� (%d, %d) ���㶹�����䱸һ���ӵ�;\n", i + 1, j + 1);
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

			// �ӵ����ж���
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

			// ��ʬ��Ϣ
			int xLeft = zbpool[j].x + 15;
			int xRight = zbpool[j].x + 55;
			int rowTarget = zbpool[j].row;

			// �ӵ���Ϣ
			int x = bpool[i].x;
			int row = bpool[i].row;

			if (x > xLeft && x < xRight && row == rowTarget && zbpool[j].dead == false)
			{
				zbpool[j].health -= bpool[i].damage;
				if (zbpool[j].health <= 0) // ��ʬ����
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

					// ��־��
					char info[100];
					sprintf_s(info, sizeof(info), "����ʬս������ %d ����ʬս�������ڵ� %d ���� %d ���ɼ���ʬ;\n", i, rowTarget + 1, rowZombie[rowTarget]);
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
	for (int i = 0; i < zbCount; i++) // ������ʬ��
	{
		if (zbpool[i].dead)
			continue; // ��������ʬ

		int row = zbpool[i].row;
		for (int col = 0; col < 9; col++) // ��������ÿ�в�Ƥ
		{
			if (map[row][col].type == -1 && zbpool[i].chomp == false)
				continue; // �����ղ�Ƥ

			int xPlantL = map[row][col].x + 10;
			int xPlantR = map[row][col].x + 60;
			int xZombieL = zbpool[i].x;
			if (xZombieL > xPlantL && xZombieL < xPlantR)
			{
				if (map[row][col].type == -1 && zbpool[i].chomp) // �����Щ��ʬ��Ӧ���������˻��ڳԣ������
				{
					zbpool[i].chomp = false;
					zbpool[i].speed = 2;
					zbpool[i].frameIndex = 0;
				}
				else if (map[row][col].catched && zbpool[i].chomp)
				{
					map[row][col].health -= zbpool[i].damage; // ֲ���Ѫ

					// ��־��
					printf("��ֲ���Ѫ���������� (%d, %d) ��ֲ���Ѫ��ĿǰѪ��Ϊ %d;\n", row + 1, col + 1, map[row][col].health);

					if (map[row][col].health <= 0) // ֲ������
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

					// ��־��
					printf("����ʬ�Է����� %d ����ʬ��ʼ�Է��������������� (%d, %d) ��ֲ��;\n", i, row + 1, col + 1);
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
	if (peekmessage(&msg)) // ��̽�����Ϣ
	{
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x > 278 && msg.x < 278 + 51 * PLANTS_COUNT && msg.y > 8 && msg.y < 78)
			{
				// ��ȡѡ�п������
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
			// printf("px(%d, %d)\n", msg.x, msg.y); // ��������귨������
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
	collisionCheck(); // ��ײ���
}

void updateWin() {
	//˫���塤����������ʱ��˸
	BeginBatchDraw();//��ʼ˫����

    putimage(0, 0, &imgBg); // ��Ⱦ����ͼ
	
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
				drawAlpha(x, y, imgPlant[map[i][j].type][map[i][j].frameIndex]); // ��Ⱦ�����µ�ֲ��
			}
		}
	}

    drawAlpha(200, 0, &imgBar); // ��Ⱦ������

	drawZombie(); // ��Ⱦ����ʬ

	drawBullet(); // ��Ⱦ���ӵ�

	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		int x = 278 + i * 51;
		int y = 8;
		drawAlpha(x, y, &imgCard[i]); // ��Ⱦ������
	}

	char pointText[8];
	sprintf_s(pointText, sizeof(pointText), "%d", sunshine);
	outtextxy(222, 58, pointText); // ��Ⱦ������ֵ�ı�

	if (curPlant > -1)
	{
		IMAGE* img = imgPlant[curPlant][0];
		int curPX = curX - img->getwidth() / 2;
		int curPY = curY - img->getheight() / 2;
		drawAlpha(curPX, curPY, img); //��Ⱦ����������ƶ��Ĵ���ֲ��
	}

	// ��Ⱦ��������
	drawSunball();

	// ��Ⱦ������ͼ��
	// drawAlpha(500, 200, &imgZombie[0]);
	// drawAlpha(420, 195, &imgZombieDie[0]); // x - 80, y - 5
	// drawAlpha(425, 200, &imgZombieChomp[0]); // x - 75, y

	EndBatchDraw();//����˫����
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
		if (timer > 10) // ���θ���֮���ۼƳ���ָ��ʱ����flag ��ǲ�Ϊ�� 
		{
			flag = true;
			timer = 0;
		}
		if (flag) // ��� flag ���Ϊ�棬ִ��һ����Ϸ����
		{
			flag = false;
			updateGame(); // ����Ҳ���������˳������
		}
		updateWin(); // ��Ҳ���������˿������
	}
    system("pause");
    return 0;
}