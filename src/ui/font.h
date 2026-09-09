#pragma once

#include <raylib.h>

void WFZLoadFont();
void WFZUnloadFont();

void WFZDrawText(const char *text, float x, float y, float size, Color color);
Vector2 WFZMeasureText(const char *text, float size);

void WFZDrawTextCentered(const char *text, Rectangle bounds, float size, Color color);

float WFZDrawTextWrapped(const char *text, Rectangle bounds, float font_size, float line_gap, Color color);
float WFZMeasureTextWrappedHeight(const char *text, float width, float font_size, float line_spacing);
