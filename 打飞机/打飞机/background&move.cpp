#include<stdio.h>
#include<graphics.h>
#include<conio.h>
#include<easyx.h>
#include<cstdlib>
#include<ctime>
#include<cmath>
#include<windows.h>
#include<string>
#include "people.h"

#define BULLET_SPEED 10
#define BULLET_MAX 400
#define BULLET_SIZE 40
#define BULLET_KINDS 4       // 弹幕种类数
#define BULLET_FRAMES_MAX 10 // 帧数上限（种类0为10帧，其余为5帧/3帧）
#define HEALTH 8
#define PATTERN_DURATION 300   // 每种弹幕模式持续帧数（约 10 秒）
#define EYE_SIZE 80            // 眼球敌机尺寸
#define EYE_START_TICK 2000    // 约 60 秒后眼球降临（每帧约 30ms）
#define EYE_FALL_SPEED 0.24f   // 下压速度：像素/帧（约 3 分 09 秒压出屏幕，晚于 3 分钟获胜线）
#define OCC_MAX 16             // 污染遮挡上限
#define WIN_TICK 5400          // 存活约 3 分钟获胜（每帧约 33ms）

// 弹幕模式
enum DanmakuPattern
{
    PAT_RANDOM,   // 随机直落（原版）
    PAT_RAIN,     // 雨弹：整排均匀下落
    PAT_AIMED,    // 自机狙：单发瞄准玩家
    PAT_RADIAL,   // 全方位弹：一圈 20 发
    PAT_FAN,      // 扇形弹：朝玩家方向散开
    PAT_COUNT
};

struct Bullet
{
    float x, y;
    float vx, vy;
    int kind;       // 弹幕种类 0/1，对应精灵图第几行
    bool live;
};

// 眼球污染遮挡：碰到眼球后永久叠在画面上的一块
struct Occlusion
{
    int x, y;
    int size;   // 60~99
    int frame;  // 用哪一帧眼球图
};

struct GameState
{
    int health;
    int bulletSpeed;
    int bulletCd;
    int pattern;      // 当前弹幕模式
    int stage;        // 已完成的阶段数（每 10 秒一个阶段，驱动难度爬升）
    int patternTimer; // 模式已持续帧数
    int fireTimer;    // 距上次发射的帧数
    int fireCd;       // 当前模式的发射间隔（帧）
    int tick;         // 全局动画时钟（驱动弹幕形态轮播）
    Bullet bullets[BULLET_MAX];
    IMAGE bk;
    IMAGE bulletFrames[BULLET_KINDS][BULLET_FRAMES_MAX];  // 弹幕帧表：[种类][帧]，已转竖直朝下
    int bulletFrameCount[BULLET_KINDS];                   // 每种弹的实际帧数
    IMAGE startImg;
    // 眼球阶段
    bool eyeActive;                 // 眼球是否已降临
    float eyeX, eyeY;               // 眼球左上角坐标
    int eyeCd;                      // 碰撞后的遮挡叠加冷却（帧）
    IMAGE eyeFrames[6];             // 眼球动画帧
    Occlusion occlusions[OCC_MAX];  // 永久污染遮挡列表
    int occCount;
    int snapTimer;                  // 检查点计时（每 40 秒快照一次）
    int san;                        // 疯狂值：每次读档复活 +1（不随回滚还原）
};

// 局内检查点快照：死亡后回滚到最近一次快照继续
struct Snapshot
{
    int health, tick, stage;
    int pattern, patternTimer, fireTimer, fireCd;
    int bulletSpeed, bulletCd;
    int myX, myY;
    bool eyeActive;
    float eyeX, eyeY;
    int occCount;
    Occlusion occlusions[OCC_MAX];
};

GameState g;
Snapshot g_snap;
bool g_snapValid = false;

void TakeSnapshot()
{
    g_snap.health = g.health;
    g_snap.tick = g.tick;
    g_snap.stage = g.stage;
    g_snap.pattern = g.pattern;
    g_snap.patternTimer = g.patternTimer;
    g_snap.fireTimer = g.fireTimer;
    g_snap.fireCd = g.fireCd;
    g_snap.bulletSpeed = g.bulletSpeed;
    g_snap.bulletCd = g.bulletCd;
    g_snap.myX = My.x;
    g_snap.myY = My.y;
    g_snap.eyeActive = g.eyeActive;
    g_snap.eyeX = g.eyeX;
    g_snap.eyeY = g.eyeY;
    g_snap.occCount = g.occCount;
    for (int i = 0; i < OCC_MAX; i++)
    {
        g_snap.occlusions[i] = g.occlusions[i];
    }
    g_snapValid = true;
}

// 死亡 → 回滚到最近检查点：状态整体倒退，弹幕清空留出喘息窗口
void RespawnFromSnapshot()
{
    cleardevice();
    g.san++;
    settextcolor(WHITE);
    settextstyle(28, 0, _T("宋体"));
    outtextxy(105, 380, _T("在记忆的残响中，苏醒…"));
    settextcolor(RGB(160, 160, 160));
    settextstyle(20, 0, _T("宋体"));
    TCHAR sanText[32];
    _stprintf_s(sanText, _T("SAN 值：%d"), g.san);
    outtextxy(190, 430, sanText);
    FlushBatchDraw();
    Sleep(2000);

    g.health = g_snap.health;
    g.tick = g_snap.tick;
    g.stage = g_snap.stage;
    g.pattern = g_snap.pattern;
    g.patternTimer = g_snap.patternTimer;
    g.fireTimer = g_snap.fireTimer;
    g.fireCd = g_snap.fireCd;
    g.bulletSpeed = g_snap.bulletSpeed;
    g.bulletCd = g_snap.bulletCd;
    My.x = g_snap.myX;
    My.y = g_snap.myY;
    g.eyeActive = g_snap.eyeActive;
    g.eyeX = g_snap.eyeX;
    g.eyeY = g_snap.eyeY;
    g.eyeCd = 90;
    g.occCount = g_snap.occCount;
    for (int i = 0; i < OCC_MAX; i++)
    {
        g.occlusions[i] = g_snap.occlusions[i];
    }
    for (int i = 0; i < BULLET_MAX; i++)
    {
        g.bullets[i].live = false;
    }
    g.snapTimer = 0;
}

void InitGameState()
{
    g.health = HEALTH;
    g.bulletSpeed = BULLET_SPEED;
    g.bulletCd = 5;
    g.pattern = PAT_RANDOM;
    g.stage = 0;
    g.patternTimer = 0;
    g.fireTimer = 0;
    g.fireCd = g.bulletCd;
    g.tick = 0;
    g.snapTimer = 0;
    g_snapValid = false;   // 新一局从零开始，旧局快照作废
    g.eyeActive = false;
    g.eyeX = 0;
    g.eyeY = -(float)EYE_SIZE;
    g.eyeCd = 0;
    g.occCount = 0;
    g.san = 0;
    for (int i = 0; i < BULLET_MAX; i++)
    {
        g.bullets[i].live = false;
    }
}

void loadImg()
{
    loadimage(&g.bk, _T("Resource/images/background2.jpg"), 2000, 1000);
    // 精灵图缩放加载：整图拉伸到 3/4，每格 40px → 30px，动画帧内容完整
    loadimage(&cirno_sheet, _T("Resource/images/planeNormal_12b.jpg"), CIRNO_CELL * 3 / 4 * COLS, CIRNO_CELL * 3 / 4 * ROWS);
    loadimage(&g.startImg, _T("Resource/images/start.jpg"), 480, 700);
}

void Draw() {
    cleardevice();
    putimage(0, 0, &g.bk);
}

void BulletScreenInit()
{
    // 弹头朝右的素材，切帧后逐帧顺时针转 90° 变竖直朝下
    IMAGE tmp;
    TCHAR path[128];

    // 种类0：竖直下落弹，10 帧（fall_1~10.png）
    for (int f = 0; f < 10; f++)
    {
        _stprintf_s(path, _T("Resource/images/fall_%d.png"), f + 1);
        loadimage(&tmp, path, BULLET_SIZE, BULLET_SIZE);
        rotateimage(&g.bulletFrames[0][f], &tmp, -3.14159265f / 2);  // -90° = 顺时针
    }
    g.bulletFrameCount[0] = 10;

    // 种类1：从源点发射的弹，5 帧（bullet_screen1~5.jpg）
    for (int f = 0; f < 5; f++)
    {
        _stprintf_s(path, _T("Resource/images/bullet_screen%d.jpg"), f + 1);
        loadimage(&tmp, path, BULLET_SIZE, BULLET_SIZE);
        rotateimage(&g.bulletFrames[1][f], &tmp, -3.14159265f / 2);
    }
    g.bulletFrameCount[1] = 5;

    // 种类2：风车弹，5 帧（spin_1~5.png），中心对称无需转向
    for (int f = 0; f < 5; f++)
    {
        _stprintf_s(path, _T("Resource/images/spin_%d.png"), f + 1);
        loadimage(&g.bulletFrames[2][f], path, BULLET_SIZE, BULLET_SIZE);
    }
    g.bulletFrameCount[2] = 5;

    // 种类3：法阵弹，3 帧（seal_1~3.png），圆形对称无需转向
    for (int f = 0; f < 3; f++)
    {
        _stprintf_s(path, _T("Resource/images/seal_%d.png"), f + 1);
        loadimage(&g.bulletFrames[3][f], path, BULLET_SIZE, BULLET_SIZE);
    }
    g.bulletFrameCount[3] = 3;

    // 眼球敌机，6 帧（eye_1~6.png），无方向性
    for (int f = 0; f < 6; f++)
    {
        _stprintf_s(path, _T("Resource/images/eye_%d.png"), f + 1);
        loadimage(&g.eyeFrames[f], path, EYE_SIZE, EYE_SIZE);
    }
}

void MoveBULLET()
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (g.bullets[i].live)
        {
            g.bullets[i].x += g.bullets[i].vx;
            g.bullets[i].y += g.bullets[i].vy;
            // 斜向弹幕会飞出左右和上方，四边都要出界回收
            bool off = g.bullets[i].y > HEIGHT1 ||
                       g.bullets[i].y < -BULLET_SIZE ||
                       g.bullets[i].x < -BULLET_SIZE ||
                       g.bullets[i].x > WIDTH1;
            if (off)
            {
                g.bullets[i].live = false;
            }
        }
    }
}

// 找一个空槽，返回下标；找不到返回 -1
int FindEmptyBullet()
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (!g.bullets[i].live)
            return i;
    }
    return -1;
}

// 通用弹幕生成：指定位置、速度、种类
void SpawnBullet(float x, float y, float vx, float vy, int kind)
{
    int idx = FindEmptyBullet();
    if (idx < 0) return;
    g.bullets[idx].x = x;
    g.bullets[idx].y = y;
    g.bullets[idx].vx = vx;
    g.bullets[idx].vy = vy;
    g.bullets[idx].kind = kind;
    g.bullets[idx].live = true;
}

// 旧的随机生成，先保留做默认
void AddBullet()
{
    SpawnBullet(
        (float)(rand() % (WIDTH1 - BULLET_SIZE)),
        0,
        0,
        (float)g.bulletSpeed,
        0
    );
}

// ---------- 弹幕模式 ----------
// 虚拟弹幕源点（还没有敌机，先固定在顶部中央，之后换成敌机坐标即可）
const float BOSS_X = WIDTH1 / 2.0f;
const float BOSS_Y = 60.0f;

// 雨弹：一排均匀间距直落
void PatternRain()
{
    const int M = 14;
    int gap = WIDTH1 / M;
    int offset = rand() % gap;
    for (int i = 0; i < M; i++)
    {
        int x = offset + i * gap + (gap - BULLET_SIZE) / 2;
        if (x < 0) x = 0;
        if (x > WIDTH1 - BULLET_SIZE) x = WIDTH1 - BULLET_SIZE;
        SpawnBullet((float)x, 0, 0, (float)g.bulletSpeed, 0);
    }
}

// 自机狙：三连发瞄准玩家当前位置
void PatternAimed()
{
    float dx = My.x + FRAME_W / 2.0f - BOSS_X;
    float dy = My.y + FRAME_H / 2.0f - BOSS_Y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < 1.0f) d = 1.0f;
    float vx = dx / d * g.bulletSpeed;
    float vy = dy / d * g.bulletSpeed;
    // 三连发：正中 + 左右偏移
    float baseAng = atan2f(vy, vx);
    for (int i = 0; i < 3; i++)
    {
        float a = baseAng + (i - 1) * 0.12f;
        SpawnBullet(BOSS_X, BOSS_Y, cosf(a) * g.bulletSpeed, sinf(a) * g.bulletSpeed, 2);
    }
}

// 全方位弹：一圈 N 发
void PatternRadial()
{
    const int N = 28;
    for (int i = 0; i < N; i++)
    {
        float a = 6.2831853f * i / N;
        SpawnBullet(BOSS_X, BOSS_Y, cosf(a) * g.bulletSpeed, sinf(a) * g.bulletSpeed, 3);
    }
}

// 扇形弹：朝玩家方向 ±0.6 弧度散开 12 发
void PatternFan()
{
    const int N = 12;
    const float SPAN = 0.6f;
    float base = atan2f(My.y + FRAME_H / 2.0f - BOSS_Y, My.x + FRAME_W / 2.0f - BOSS_X);
    for (int i = 0; i < N; i++)
    {
        float a = base + SPAN * (i * 2.0f / (N - 1) - 1.0f);
        SpawnBullet(BOSS_X, BOSS_Y, cosf(a) * g.bulletSpeed, sinf(a) * g.bulletSpeed, 2);
    }
}

// 切换到下一个弹幕模式
void NextPattern()
{
    g.pattern = (g.pattern + 1) % PAT_COUNT;
    g.patternTimer = 0;
    g.fireTimer = 0;

    // 阶段制难度爬升（加速度 ×3）：每 10 秒弹速 +3（上限 30），每 5 个阶段发射间隔 -3（下限 1）
    // 约 7 个阶段（70 秒）弹速到顶，15 个阶段（150 秒）发射间隔到底
    g.stage++;
    if (g.bulletSpeed < 30)
    {
        g.bulletSpeed += 3;
        if (g.bulletSpeed > 30) g.bulletSpeed = 30;
    }
    if (g.stage % 5 == 0 && g.bulletCd > 1)
    {
        g.bulletCd -= 3;
        if (g.bulletCd < 1) g.bulletCd = 1;
    }

    switch (g.pattern)
    {
    case PAT_RANDOM: g.fireCd = g.bulletCd; break;
    case PAT_RAIN:   g.fireCd = 60; break;
    case PAT_AIMED:  g.fireCd = 18; break;
    case PAT_RADIAL: g.fireCd = 50; break;
    case PAT_FAN:    g.fireCd = 35; break;
    }
}

// 每帧调用：按当前模式发射 + 计时换模式
void FirePattern()
{
    if (g.pattern == PAT_RANDOM)
        g.fireCd = g.bulletCd;  // 随机模式沿用全局难度节奏

    if (g.fireTimer >= g.fireCd)
    {
        g.fireTimer = 0;
        switch (g.pattern)
        {
        case PAT_RANDOM: AddBullet();    break;
        case PAT_RAIN:   PatternRain();  break;
        case PAT_AIMED:  PatternAimed(); break;
        case PAT_RADIAL: PatternRadial();break;
        case PAT_FAN:    PatternFan();   break;
        }
    }
    g.fireTimer++;
    g.patternTimer++;
    if (g.patternTimer >= PATTERN_DURATION)
    {
        NextPattern();
    }
}

void DrawBULLET()
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (g.bullets[i].live)
        {
            int kind = g.bullets[i].kind;
            int n = g.bulletFrameCount[kind];
            // 往复循环 1→n→2（每 6 帧切换一形态，全场同步）
            int period = 2 * n - 2;
            int k = (g.tick / 6) % period;
            int frame = (k < n) ? k : period - k;  // 0 基帧号

            int dx = (int)g.bullets[i].x;
            int dy = (int)g.bullets[i].y;
            IMAGE &frm = g.bulletFrames[kind][frame];
            // 黑底发光弹单遍 OR 贴图：黑底透出背景，弹体发光叠加；
            // 弹与弹重叠同样是 OR 叠加，不会互相挖黑边（之前的 SRCAND 会把矩形内背景压黑，已删）
            putimage(dx, dy, &frm, SRCPAINT);
        }
    }
}

// ---------- 眼球阶段 ----------
// 添加一块永久污染遮挡（随机位置、随机大小、随机帧）
void AddOcclusion()
{
    if (g.occCount >= OCC_MAX) return;
    Occlusion &o = g.occlusions[g.occCount++];
    o.x = rand() % (WIDTH1 - EYE_SIZE);
    o.y = rand() % (HEIGHT1 - EYE_SIZE);
    o.size = 60 + rand() % 40;
    o.frame = rand() % 6;
}

// 眼球阶段逻辑：降临、下压、碰撞加遮挡、压底判负
void UpdateEye()
{
    if (!g.eyeActive)
    {
        if (g.tick >= EYE_START_TICK)
        {
            g.eyeActive = true;
            g.eyeX = WIDTH1 / 2.0f - EYE_SIZE / 2.0f;
            g.eyeY = -(float)EYE_SIZE;
        }
        return;
    }

    // 缓慢下压 + 轻微横摆
    g.eyeY += EYE_FALL_SPEED;
    g.eyeX = WIDTH1 / 2.0f - EYE_SIZE / 2.0f + 22.0f * sinf(g.tick * 0.004f);

    // 压出屏幕底 = 被吞噬
    if (g.eyeY > HEIGHT1)
    {
        g.health = 0;
        return;
    }

    // 碰到自机 → 永久遮挡一块（带冷却，防止贴脸连刷）
    if (g.eyeCd > 0)
    {
        g.eyeCd--;
    }
    else
    {
        float dx = My.x + FRAME_W / 2.0f - (g.eyeX + EYE_SIZE / 2.0f);
        float dy = My.y + FRAME_H / 2.0f - (g.eyeY + EYE_SIZE / 2.0f);
        if (dx * dx + dy * dy < 45.0f * 45.0f)
        {
            AddOcclusion();
            g.eyeCd = 90;
        }
    }
}

void DrawEye()
{
    if (!g.eyeActive) return;
    IMAGE &frm = g.eyeFrames[(g.tick / 8) % 6];
    int dx = (int)g.eyeX, dy = (int)g.eyeY;
    // 两遍贴图：黑底透出画面，眼球本体不透明地压在弹幕上
    putimage(dx, dy, &frm, SRCAND);
    putimage(dx, dy, &frm, SRCPAINT);
}

void DrawOcclusions()
{
    for (int i = 0; i < g.occCount; i++)
    {
        Occlusion &o = g.occlusions[i];
        IMAGE &frm = g.eyeFrames[(g.tick / 8) % 6];  // 污染块与眼球同步动画
        // 污染块拉伸到随机大小，两遍贴图 = 该区域被眼球图案永久覆盖
        putimage(o.x, o.y, o.size, o.size, &frm, 0, 0, SRCAND);
        putimage(o.x, o.y, o.size, o.size, &frm, 0, 0, SRCPAINT);
    }
}

void Collision()
{
    for (int i = 0; i < BULLET_MAX; i++)
    {
        if (g.bullets[i].live)
        {
            int cx = FRAME_W / 6;
            int cy = FRAME_H / 6;

            if (g.bullets[i].x < My.x + FRAME_W / 2 + cx &&
                g.bullets[i].x + BULLET_SIZE > My.x + FRAME_W / 2 - cx &&
                g.bullets[i].y < My.y + FRAME_H / 2 + cy &&
                g.bullets[i].y + BULLET_SIZE > My.y + FRAME_H / 2 - cy)
            {
                g.health -= 1;
                g.bullets[i].live = false;
            }
        }
    }
}

void victory()
{
    if (g.tick >= WIN_TICK)
    {
        cleardevice();
        putimage(0, 0, &g.startImg);
        settextcolor(YELLOW);
        settextstyle(50, 0, _T("宋体"));
        outtextxy(140, 300, _T("你赢了！"));
        settextcolor(WHITE);
        settextstyle(25, 0, _T("宋体"));
        outtextxy(80, 400, _T("琪露诺的完美下塔！"));
        if (g.health == HEALTH)
        {
            outtextxy(100, 450, _T("无伤通关！强 baka 的荣耀！"));
        }
        FlushBatchDraw();
        Sleep(3000);
        exit(0);
    }
}

void Lose()
{
    if (g.health == 0)
    {
        // 有检查点 → 回滚复活；没有（开局 40 秒内阵亡）→ 真失败
        if (g_snapValid)
        {
            RespawnFromSnapshot();
            return;
        }
        cleardevice();
        putimage(0, 0, &g.startImg);
        settextcolor(RED);
        settextstyle(50, 0, _T("宋体"));
        outtextxy(140, 300, _T("你输了！"));
        settextcolor(WHITE);
        settextstyle(25, 0, _T("宋体"));
        outtextxy(80, 400, _T("再练练吧129"));
        FlushBatchDraw();
        Sleep(3000);
        exit(0);
    }
}

void StartMenu()
{
    HWND hwnd = GetHWnd();
    if (hwnd) {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }

    while (1)
    {
        putimage(0, 0, &g.startImg);
        settextcolor(WHITE);
        settextstyle(36, 0, _T("宋体"));
        outtextxy(150, 700, _T("飞机大战"));
        settextstyle(24, 0, _T("宋体"));
        outtextxy(130, 750, _T("按 空格键 开始游戏"));
        outtextxy(150, 800, _T("按 ESC 退出"));
        FlushBatchDraw();

        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
            break;
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(10);
            exit(0);
        }

        Sleep(30);
    }
}

int main()
{
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string dir = exePath;
    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);
        SetCurrentDirectoryA(dir.c_str());
    }

    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    initgraph(WIDTH1, HEIGHT1, 1);
    BeginBatchDraw();
    InitGameState();
    loadImg();
    initCirno();
    srand((unsigned)time(0));
    BulletScreenInit();
    StartMenu();
    Draw();
    while (true)
    {
        Draw();
        MoveCirno();
        DrawCirno();
        FirePattern();
        MoveBULLET();
        DrawBULLET();
        DrawEye();
        Collision();
        UpdateEye();
        DrawOcclusions();
        g.tick++;
        // 每 20 秒（600 帧）做一个检查点
        g.snapTimer++;
        if (g.snapTimer >= 600)
        {
            TakeSnapshot();
            g.snapTimer = 0;
        }
        victory();
        Lose();
        FlushBatchDraw();
        Sleep(30);
    }
    EndBatchDraw();
    closegraph();
    return 0;
}
