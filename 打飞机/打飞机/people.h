#ifndef PEOPLE_H
#define PEOPLE_H
#include <graphics.h>
const int WIDTH1 = 480;
const int HEIGHT1 = 800;
const int CIRNO_CELL = 40;                  // 精灵图源格子尺寸
const int FRAME_W = CIRNO_CELL * 3 / 4;     // 自机绘制/碰撞尺寸 = 源格 3/4（30px，加载时整图等比缩放）
const int FRAME_H = CIRNO_CELL * 3 / 4;
const int COLS = 3;
const int ROWS = 4;
struct Cirno
{
    int x, y;
    int row;
    int frame;
    int timer;
};

extern IMAGE cirno_sheet;
extern Cirno My;

void initCirno();
void MoveCirno();
void DrawCirno();

#endif