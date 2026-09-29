#include "people.h"
#include <windows.h>
#include <conio.h>

IMAGE cirno_sheet;
Cirno My;

void initCirno()
{
    My.x = 200;
    My.y = 600;
    My.row = 0;
    My.frame = 0;
    My.timer = 0;
}

void MoveCirno()
{
    int dx = 0, dy = 0;
    const int SPEED = 12;

    if (GetAsyncKeyState('W') & 0x8000) dy = -SPEED;
    if (GetAsyncKeyState('S') & 0x8000) dy = SPEED;
    if (GetAsyncKeyState('A') & 0x8000) dx = -SPEED;
    if (GetAsyncKeyState('D') & 0x8000) dx = SPEED;

    // 斜向移动时保持动画一致性：左右优先于上下
    if (dx != 0) {
        My.row = (dx < 0) ? 1 : 2;
    } else if (dy != 0) {
        My.row = (dy < 0) ? 3 : 0;
    }

    My.x += dx;
    My.y += dy;

    if (My.x < 0)                  My.x = 0;
    if (My.x > WIDTH1 - FRAME_W)   My.x = WIDTH1 - FRAME_W;
    if (My.y < 0)                  My.y = 0;
    if (My.y > HEIGHT1 - FRAME_H)  My.y = HEIGHT1 - FRAME_H;
}

void DrawCirno()
{
    My.timer++;
    if (My.timer >= 8)
    {
        My.timer = 0;
        My.frame = (My.frame + 1) % COLS;
    }

    putimage(
        My.x, My.y,
        FRAME_W, FRAME_H,
        &cirno_sheet,
        My.frame * FRAME_W,
        My.row * FRAME_H
    );
}
