#pragma once

#include <raylib.h>

struct WFZScrollArea
{
    Rectangle view;
    float content_height;
    float scroll_offset;
};

void WFZUpdateScrollArea(WFZScrollArea &scroll, float wheel_speed = 42.0f);
void WFZBeginScrollArea(const WFZScrollArea &scroll);
void WFZEndScrollArea();
void WFZDrawScrollBar(const WFZScrollArea &scroll, float x, float width = 4.0f);
