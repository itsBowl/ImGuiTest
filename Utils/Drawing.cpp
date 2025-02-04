#define IMGUI_DEFINE_MATH_OPERATIONS
#include "drawing.h"
#include <imgui/imgui_internal.h>

void ax::Drawing::drawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 col, ImU32 inCol)
{
    auto rect = ImRect(a, b);
    auto rectX = rect.Min.x;
    auto rectY = rect.Min.y;
    auto rectW = rect.Max.x - rect.Min.x;
    auto rectH = rect.Max.y - rect.Min.y;
    auto rectCenterX = (rect.Min.x + rect.Max.x) * 0.5f;
    auto rectCenterY = (rect.Min.y + rect.Max.y) * 0.5f;
    auto rectCenter = ImVec2(rectCenterX, rectCenterY);
    const auto outlineScale = rectW / 24.0f;
    const auto extraSegments = static_cast<int>(2 * outlineScale);


    if (type == IconType::Flow)
    {
        const auto origin_scale = rectW / 24.0f;

        const auto offset_x = 1.0f * origin_scale;
        const auto offset_y = 0.0f * origin_scale;
        const auto margin = (filled ? 2.0f : 2.0f) * origin_scale;
        const auto rounding = 0.1f * origin_scale;
        const auto tip_round = 0.7f; // percentage of triangle edge
        //const auto edge_round = 0.7f; // percentage of triangle edge
        const auto canvas = ImRect(
            rect.Min.x + margin + offset_x,
            rect.Min.y + margin + offset_y,
            rect.Max.x - margin + offset_x,
            rect.Max.y - margin + offset_y);
        const auto canvas_x = canvas.Min.x;
        const auto canvas_y = canvas.Min.y;
        const auto canvas_w = canvas.Max.x - canvas.Min.x;
        const auto canvas_h = canvas.Max.y - canvas.Min.y;

        const auto left = canvas_x + canvas_w * 0.5f * 0.3f;
        const auto right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
        const auto top = canvas_y + canvas_h * 0.5f * 0.2f;
        const auto bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
        const auto center_y = (top + bottom) * 0.5f;
        //const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

        const auto tip_top = ImVec2(canvas_x + canvas_w * 0.5f, top);
        const auto tip_right = ImVec2(right, center_y);
        const auto tip_bottom = ImVec2(canvas_x + canvas_w * 0.5f, bottom);

        drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, top),
            ImVec2(left, top),
            ImVec2(left, top) + ImVec2(rounding, 0));
        drawList->PathLineTo(tip_top);
        drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
        drawList->PathBezierCubicCurveTo(
            tip_right,
            tip_right,
            tip_bottom + (tip_right - tip_bottom) * tip_round);
        drawList->PathLineTo(tip_bottom);
        drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, bottom),
            ImVec2(left, bottom),
            ImVec2(left, bottom) - ImVec2(0, rounding));

        if (!filled)
        {
            if (inCol & 0xFF000000)
                drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, inCol);

            drawList->PathStroke(col, true, 2.0f * outlineScale);
        }
        else
            drawList->PathFillConvex(col);
    }
    else
    {
        auto triangleStart = rectCenterX + 0.32f * rectW;

        auto offset = -static_cast<int>(rectW * 0.25f * 0.25f);

        rect.Min.x += offset;
        rect.Max.x += offset;
        rectX += offset;
        rectCenterX += offset * 0.5f;
        rectCenter.x += offset * 0.5f;

        if (type == IconType::Circle)
        {
            const auto c = rectCenter;

            if (!filled)
            {
                const auto r = 0.5f * rectW / 2.0f - 0.5f;

                if (inCol & 0xFF000000)
                    drawList->AddCircleFilled(c, r, inCol, 12 + extraSegments);
                drawList->AddCircle(c, r, col, 12 + extraSegments, 2.0f * outlineScale);
            }
            else
            {
                drawList->AddCircleFilled(c, 0.5f * rectW / 2.0f, col, 12 + extraSegments);
            }
        }

        if (type == IconType::Square)
        {
            if (filled)
            {
                const auto r = 0.5f * rectW / 2.0f;
                const auto p0 = rectCenter - ImVec2(r, r);
                const auto p1 = rectCenter + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRectFilled(p0, p1, col, 0, ImDrawFlags_RoundCornersAll);
#else
                drawList->AddRectFilled(p0, p1, color, 0, 15);
#endif
            }
            else
            {
                const auto r = 0.5f * rectW / 2.0f - 0.5f;
                const auto p0 = rectCenter - ImVec2(r, r);
                const auto p1 = rectCenter + ImVec2(r, r);

                if (inCol & 0xFF000000)
                {
#if IMGUI_VERSION_NUM > 18101
                    drawList->AddRectFilled(p0, p1, inCol, 0, ImDrawFlags_RoundCornersAll);
#else
                    drawList->AddRectFilled(p0, p1, inCol, 0, 15);
#endif
                }

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRect(p0, p1, col, 0, ImDrawFlags_RoundCornersAll, 2.0f * outlineScale);
#else
                drawList->AddRect(p0, p1, col, 0, 15, 2.0f * outlineScale);
#endif
            }
        }

        if (type == IconType::Grid)
        {
            const auto r = 0.5f * rectW / 2.0f;
            const auto w = ceilf(r / 3.0f);

            const auto baseTl = ImVec2(floorf(rectCenterX - w * 2.5f), floorf(rectCenterY - w * 2.5f));
            const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

            auto tl = baseTl;
            auto br = baseBr;
            for (int i = 0; i < 3; ++i)
            {
                tl.x = baseTl.x;
                br.x = baseBr.x;
                drawList->AddRectFilled(tl, br, col);
                tl.x += w * 2;
                br.x += w * 2;
                if (i != 1 || filled)
                    drawList->AddRectFilled(tl, br, col);
                tl.x += w * 2;
                br.x += w * 2;
                drawList->AddRectFilled(tl, br, col);

                tl.y += w * 2;
                br.y += w * 2;
            }

            triangleStart = br.x + w + 1.0f / 24.0f * rectW;
        }

        if (type == IconType::RoundSquare)
        {
            if (filled)
            {
                const auto r = 0.5f * rectW / 2.0f;
                const auto cr = r * 0.5f;
                const auto p0 = rectCenter - ImVec2(r, r);
                const auto p1 = rectCenter + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRectFilled(p0, p1, col, cr, ImDrawFlags_RoundCornersAll);
#else
                drawList->AddRectFilled(p0, p1, col, cr, 15);
#endif
            }
            else
            {
                const auto r = 0.5f * rectW / 2.0f - 0.5f;
                const auto cr = r * 0.5f;
                const auto p0 = rectCenter - ImVec2(r, r);
                const auto p1 = rectCenter + ImVec2(r, r);

                if (inCol & 0xFF000000)
                {
#if IMGUI_VERSION_NUM > 18101
                    drawList->AddRectFilled(p0, p1, inCol, cr, ImDrawFlags_RoundCornersAll);
#else
                    drawList->AddRectFilled(p0, p1, inCol, cr, 15);
#endif
                }

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRect(p0, p1, col, cr, ImDrawFlags_RoundCornersAll, 2.0f * outlineScale);
#else
                drawList->AddRect(p0, p1, color, cr, 15, 2.0f * outline_scale);
#endif
            }
        }
        else if (type == IconType::Diamond)
        {
            if (filled)
            {
                const auto r = 0.607f * rectW / 2.0f;
                const auto c = rectCenter;

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));
                drawList->PathFillConvex(col);
            }
            else
            {
                const auto r = 0.607f * rectW / 2.0f - 0.5f;
                const auto c = rectCenter;

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));

                if (inCol & 0xFF000000)
                    drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, inCol);

                drawList->PathStroke(col, true, 2.0f * outlineScale);
            }
        }
        else
        {
            const auto triangleTip = triangleStart + rectW * (0.45f - 0.32f);

            drawList->AddTriangleFilled(
                ImVec2(ceilf(triangleTip), rectY + rectH * 0.5f),
                ImVec2(triangleStart, rectCenterY + 0.15f * rectH),
                ImVec2(triangleStart, rectCenterY - 0.15f * rectH),
                col);
        }
    }
}