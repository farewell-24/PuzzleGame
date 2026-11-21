#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#endif

namespace {

    const wchar_t kWindowClass[] = L"CppPuzzleGameWindow";
    const wchar_t kWindowTitle[] = L"拼图工坊 - C++";
    constexpr UINT_PTR kTimerId = 1;

    const Gdiplus::Color kBackgroundTop(255, 11, 15, 23);
    const Gdiplus::Color kBackgroundBottom(255, 22, 28, 41);
    const Gdiplus::Color kPanel(255, 24, 30, 42);
    const Gdiplus::Color kPanelHover(255, 34, 42, 57);
    const Gdiplus::Color kBorder(255, 50, 60, 77);
    const Gdiplus::Color kText(255, 240, 244, 250);
    const Gdiplus::Color kMuted(255, 145, 157, 174);
    const Gdiplus::Color kAccent(255, 242, 172, 87);
    const Gdiplus::Color kAccentHover(255, 255, 193, 107);
    const Gdiplus::Color kAccentDark(255, 45, 34, 24);
    const Gdiplus::Color kSuccess(255, 89, 208, 150);
    const Gdiplus::Color kDanger(255, 239, 113, 113);

    struct UiRect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        bool contains(float px, float py) const {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }
    };

    enum class Action {
        None,
        NewGame,
        NextImage,
        ChooseImage,
        ToggleOriginal,
        ToggleChallenge,
        Difficulty3,
        Difficulty4,
        Difficulty5
    };

    struct Layout {
        UiRect board;
        UiRect panel;
        std::array<UiRect, 3> difficulty{};
        UiRect newGame;
        UiRect nextImage;
        UiRect chooseImage;
        UiRect original;
        UiRect challenge;
    };

    HWND g_window = nullptr;
    ULONG_PTR g_gdiplusToken = 0;
    Gdiplus::FontFamily* g_fontFamily = nullptr;
    HBRUSH g_windowBrush = nullptr;

    std::unique_ptr<Gdiplus::Image> g_image;
    std::wstring g_imageName = L"暮色山谷";
    int g_builtinIndex = 0;

    std::vector<int> g_tiles;
    int g_grid = 3;
    int g_moves = 0;
    int g_elapsedSeconds = 0;
    bool g_solved = false;
    bool g_timedOut = false;
    bool g_viewOriginal = false;
    bool g_challenge = false;
    int g_challengeLimit = 120;

    int g_dragCell = -1;
    int g_hoverCell = -1;
    POINT g_mouse{ -1000, -1000 };
    float g_dragOffsetX = 0.0f;
    float g_dragOffsetY = 0.0f;
    bool g_trackingMouse = false;

    void repaint() {
        if (g_window) {
            InvalidateRect(g_window, nullptr, FALSE);
        }
    }

    std::wstring twoDigits(int value) {
        return (value < 10 ? L"0" : L"") + std::to_wstring(value);
    }

    std::wstring formatTime(int seconds) {
        seconds = std::max(0, seconds);
        return twoDigits(seconds / 60) + L":" + twoDigits(seconds % 60);
    }

    std::wstring difficultyText(int grid) {
        switch (grid) {
        case 3: return L"3 × 3 · 轻松";
        case 4: return L"4 × 4 · 中等";
        case 5: return L"5 × 5 · 困难";
        default: return L"自定义";
        }
    }

    int challengeSeconds(int grid) {
        switch (grid) {
        case 3: return 120;
        case 4: return 240;
        case 5: return 420;
        default: return 180;
        }
    }

    // ------------------------- 拼图模型 -------------------------
    bool isSolved() {
        if (g_tiles.empty()) {
            return false;
        }
        for (int i = 0; i < static_cast<int>(g_tiles.size()); ++i) {
            if (g_tiles[i] != i) {
                return false;
            }
        }
        return true;
    }

    void initializeOrderedTiles() {
        const int count = g_grid * g_grid;
        g_tiles.resize(count);
        std::iota(g_tiles.begin(), g_tiles.end(), 0);
    }

    void shuffleTiles() {
        initializeOrderedTiles();
        std::random_device device;
        std::mt19937 generator(device());
        do {
            std::shuffle(g_tiles.begin(), g_tiles.end(), generator);
        } while (isSolved());
    }

    void startNewGame() {
        if (g_dragCell >= 0 && GetCapture() == g_window) {
            ReleaseCapture();
        }
        g_dragCell = -1;
        g_hoverCell = -1;
        g_moves = 0;
        g_elapsedSeconds = 0;
        g_solved = false;
        g_timedOut = false;
        g_viewOriginal = false;
        if (g_challenge) {
            g_challengeLimit = challengeSeconds(g_grid);
        }
        shuffleTiles();
        repaint();
    }

    void cancelDrag() {
        if (g_dragCell >= 0 && GetCapture() == g_window) {
            ReleaseCapture();
        }
        g_dragCell = -1;
        g_hoverCell = -1;
        repaint();
    }

    // ------------------------- 界面布局 -------------------------
    Layout makeLayout(int width, int height) {
        Layout layout;
        constexpr float margin = 28.0f;
        constexpr float headerHeight = 96.0f;
        constexpr float bottomMargin = 24.0f;
        constexpr float gap = 24.0f;
        constexpr float sideWidth = 300.0f;

        const float availableHeight = static_cast<float>(height) - headerHeight - bottomMargin;
        const float availableWidth = static_cast<float>(width) - margin * 2.0f - gap - sideWidth;
        const float boardSize = std::max(360.0f, std::min(availableWidth, availableHeight));
        const float groupWidth = boardSize + gap + sideWidth;
        const float groupX = std::max(margin, (static_cast<float>(width) - groupWidth) * 0.5f);
        const float groupY = headerHeight + std::max(0.0f, (availableHeight - boardSize) * 0.5f);

        layout.board = { groupX, groupY, boardSize, boardSize };
        layout.panel = { groupX + boardSize + gap, groupY, sideWidth, boardSize };

        const float px = layout.panel.x + 18.0f;
        const float py = layout.panel.y;
        const float innerWidth = layout.panel.w - 36.0f;
        const float difficultyWidth = (innerWidth - 16.0f) / 3.0f;

        for (int i = 0; i < 3; ++i) {
            layout.difficulty[i] = {
                px + i * (difficultyWidth + 8.0f), py + 42.0f,
                difficultyWidth, 38.0f
            };
        }

        layout.newGame = { px, py + 130.0f, innerWidth, 42.0f };
        layout.nextImage = { px, py + 180.0f, innerWidth, 36.0f };
        layout.chooseImage = { px, py + 222.0f, innerWidth, 36.0f };
        layout.original = { px, py + 270.0f, innerWidth, 36.0f };
        layout.challenge = { px, py + 316.0f, innerWidth, 40.0f };

        return layout;
    }

    Action hitAction(float x, float y) {
        RECT client{};
        GetClientRect(g_window, &client);
        const Layout layout = makeLayout(client.right, client.bottom);

        if (layout.newGame.contains(x, y)) return Action::NewGame;
        if (layout.nextImage.contains(x, y)) return Action::NextImage;
        if (layout.chooseImage.contains(x, y)) return Action::ChooseImage;
        if (layout.original.contains(x, y)) return Action::ToggleOriginal;
        if (layout.challenge.contains(x, y)) return Action::ToggleChallenge;
        if (layout.difficulty[0].contains(x, y)) return Action::Difficulty3;
        if (layout.difficulty[1].contains(x, y)) return Action::Difficulty4;
        if (layout.difficulty[2].contains(x, y)) return Action::Difficulty5;
        return Action::None;
    }

    int cellAtPoint(float x, float y) {
        RECT client{};
        GetClientRect(g_window, &client);
        const Layout layout = makeLayout(client.right, client.bottom);
        if (!layout.board.contains(x, y)) {
            return -1;
        }

        const float cellSize = layout.board.w / static_cast<float>(g_grid);
        int column = static_cast<int>((x - layout.board.x) / cellSize);
        int row = static_cast<int>((y - layout.board.y) / cellSize);
        column = std::clamp(column, 0, g_grid - 1);
        row = std::clamp(row, 0, g_grid - 1);
        return row * g_grid + column;
    }

    // ------------------------- GDI+ 绘图辅助 -------------------------
    void addRoundedRectangle(Gdiplus::GraphicsPath& path,
        const Gdiplus::RectF& rect, float radius) {
        const float diameter = std::min(radius * 2.0f, std::min(rect.Width, rect.Height));
        if (diameter <= 0.0f) {
            path.AddRectangle(rect);
            return;
        }

        const float arc = diameter;
        path.AddArc(rect.X, rect.Y, arc, arc, 180.0f, 90.0f);
        path.AddArc(rect.X + rect.Width - arc, rect.Y, arc, arc, 270.0f, 90.0f);
        path.AddArc(rect.X + rect.Width - arc, rect.Y + rect.Height - arc,
            arc, arc, 0.0f, 90.0f);
        path.AddArc(rect.X, rect.Y + rect.Height - arc, arc, arc, 90.0f, 90.0f);
        path.CloseFigure();
    }

    Gdiplus::RectF toRectF(const UiRect& rect) {
        return Gdiplus::RectF(rect.x, rect.y, rect.w, rect.h);
    }

    void drawRoundedRect(Gdiplus::Graphics& graphics, const UiRect& rect,
        float radius, const Gdiplus::Color& fill,
        const Gdiplus::Color& border = Gdiplus::Color(0, 0, 0, 0),
        float borderWidth = 1.0f) {
        Gdiplus::GraphicsPath path;
        addRoundedRectangle(path, toRectF(rect), radius);
        Gdiplus::SolidBrush brush(fill);
        graphics.FillPath(&brush, &path);
        if (border.GetA() > 0) {
            Gdiplus::Pen pen(border, borderWidth);
            graphics.DrawPath(&pen, &path);
        }
    }

    void drawText(Gdiplus::Graphics& graphics, const std::wstring& text,
        const UiRect& rect, float size, const Gdiplus::Color& color,
        INT style = Gdiplus::FontStyleRegular,
        Gdiplus::StringAlignment alignment = Gdiplus::StringAlignmentNear,
        Gdiplus::StringAlignment vertical = Gdiplus::StringAlignmentCenter,
        bool wrap = false) {
        if (!g_fontFamily) {
            return;
        }

        Gdiplus::Font font(g_fontFamily, size, style, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brush(color);
        Gdiplus::StringFormat format;
        format.SetAlignment(alignment);
        format.SetLineAlignment(vertical);
        if (!wrap) {
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        }
        graphics.DrawString(text.c_str(), -1, &font, toRectF(rect), &format, &brush);
    }

    void drawButton(Gdiplus::Graphics& graphics, const UiRect& rect,
        const std::wstring& text, bool primary = false,
        bool selected = false, bool danger = false) {
        const bool hover = rect.contains(static_cast<float>(g_mouse.x),
            static_cast<float>(g_mouse.y));
        Gdiplus::Color fill = primary ? kAccent : Gdiplus::Color(255, 32, 39, 52);
        Gdiplus::Color border = primary ? kAccent : kBorder;
        Gdiplus::Color textColor = primary ? kAccentDark : kText;

        if (hover) {
            fill = primary ? kAccentHover : kPanelHover;
        }
        if (selected) {
            fill = danger ? Gdiplus::Color(255, 75, 39, 44) : Gdiplus::Color(255, 70, 57, 38);
            border = danger ? kDanger : kAccent;
            textColor = danger ? kDanger : kAccent;
        }

        drawRoundedRect(graphics, rect, 10.0f, fill, border);
        drawText(graphics, text, rect, selected || primary ? 15.0f : 14.0f,
            textColor, Gdiplus::FontStyleRegular,
            Gdiplus::StringAlignmentCenter);
    }

    Gdiplus::RectF imageCropRect(Gdiplus::Image* image) {
        const float width = static_cast<float>(image->GetWidth());
        const float height = static_cast<float>(image->GetHeight());
        const float side = std::min(width, height);
        return Gdiplus::RectF((width - side) * 0.5f, (height - side) * 0.5f, side, side);
    }

    void drawImagePiece(Gdiplus::Graphics& graphics, const UiRect& destination,
        int pieceIndex) {
        if (!g_image) {
            return;
        }

        const Gdiplus::RectF crop = imageCropRect(g_image.get());
        const float pieceWidth = crop.Width / static_cast<float>(g_grid);
        const float pieceHeight = crop.Height / static_cast<float>(g_grid);
        const int row = pieceIndex / g_grid;
        const int column = pieceIndex % g_grid;

        graphics.DrawImage(
            g_image.get(), toRectF(destination),
            crop.X + column * pieceWidth,
            crop.Y + row * pieceHeight,
            pieceWidth, pieceHeight, Gdiplus::UnitPixel);
    }

    // ------------------------- 内置图片 -------------------------
    std::unique_ptr<Gdiplus::Image> createBuiltinImage(int index) {
        using namespace Gdiplus;

        auto bitmap = std::make_unique<Bitmap>(900, 900, PixelFormat32bppPARGB);
        Graphics graphics(bitmap.get());
        if (graphics.GetLastStatus() != Ok) {
            return nullptr;
        }

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        graphics.SetCompositingQuality(CompositingQualityHighQuality);

        std::mt19937 rng(20261026u + static_cast<unsigned>(index) * 97u);
        std::uniform_real_distribution<float> random(0.0f, 1.0f);

        if (index == 0) {
            LinearGradientBrush sky(Rect(0, 0, 900, 900),
                Color(255, 13, 27, 52),
                Color(255, 240, 128, 86),
                LinearGradientModeVertical);
            graphics.FillRectangle(&sky, 0, 0, 900, 900);

            SolidBrush star(Color(165, 245, 245, 255));
            for (int i = 0; i < 95; ++i) {
                const float x = random(rng) * 900.0f;
                const float y = random(rng) * 430.0f;
                const float size = 1.0f + random(rng) * 2.6f;
                graphics.FillEllipse(&star, x, y, size, size);
            }

            for (int i = 7; i >= 1; --i) {
                const float radius = 95.0f + i * 39.0f;
                SolidBrush glow(Color(12, 255, 181, 96));
                graphics.FillEllipse(&glow, 625.0f - radius, 255.0f - radius,
                    radius * 2.0f, radius * 2.0f);
            }
            SolidBrush sun(Color(255, 255, 222, 154));
            graphics.FillEllipse(&sun, 525.0f, 155.0f, 200.0f, 200.0f);

            GraphicsPath backMountains;
            PointF backPoints[] = {
                PointF(0, 640), PointF(0, 500), PointF(120, 380),
                PointF(235, 482), PointF(350, 350), PointF(510, 520),
                PointF(670, 380), PointF(810, 475), PointF(900, 420),
                PointF(900, 900), PointF(0, 900)
            };
            backMountains.AddPolygon(backPoints, 11);
            SolidBrush backBrush(Color(255, 79, 67, 91));
            graphics.FillPath(&backBrush, &backMountains);

            LinearGradientBrush lake(Rect(0, 605, 900, 295),
                Color(255, 59, 60, 80),
                Color(255, 13, 22, 38),
                LinearGradientModeVertical);
            graphics.FillRectangle(&lake, 0, 605, 900, 295);

            GraphicsPath middleMountains;
            PointF middlePoints[] = {
                PointF(0, 690), PointF(120, 555), PointF(255, 630),
                PointF(375, 515), PointF(520, 625), PointF(680, 520),
                PointF(790, 610), PointF(900, 550), PointF(900, 900),
                PointF(0, 900)
            };
            middleMountains.AddPolygon(middlePoints, 10);
            SolidBrush middleBrush(Color(255, 43, 47, 67));
            graphics.FillPath(&middleBrush, &middleMountains);

            GraphicsPath frontMountains;
            PointF frontPoints[] = {
                PointF(0, 790), PointF(150, 685), PointF(300, 760),
                PointF(475, 675), PointF(650, 770), PointF(815, 690),
                PointF(900, 725), PointF(900, 900), PointF(0, 900)
            };
            frontMountains.AddPolygon(frontPoints, 9);
            SolidBrush frontBrush(Color(255, 20, 28, 42));
            graphics.FillPath(&frontBrush, &frontMountains);

            Pen reflection(Color(90, 255, 205, 133), 5.0f);
            for (int i = 0; i < 7; ++i) {
                const float y = 715.0f + i * 18.0f;
                const float half = 145.0f - i * 12.0f;
                graphics.DrawLine(&reflection, 625.0f - half, y, 625.0f + half, y);
            }
        }
        else if (index == 1) {
            LinearGradientBrush sky(Rect(0, 0, 900, 900),
                Color(255, 7, 16, 43),
                Color(255, 22, 58, 88),
                LinearGradientModeVertical);
            graphics.FillRectangle(&sky, 0, 0, 900, 900);

            SolidBrush aurora(Color(48, 67, 235, 197));
            GraphicsPath auroraPath;
            PointF auroraPoints[] = {
                PointF(-50, 260), PointF(170, 90), PointF(420, 150),
                PointF(610, 40), PointF(760, 210), PointF(540, 280),
                PointF(310, 245), PointF(120, 350)
            };
            auroraPath.AddPolygon(auroraPoints, 8);
            graphics.FillPath(&aurora, &auroraPath);

            SolidBrush stars(Color(190, 235, 245, 255));
            for (int i = 0; i < 190; ++i) {
                const float x = random(rng) * 900.0f;
                const float y = random(rng) * 560.0f;
                const float size = 1.0f + random(rng) * 2.3f;
                graphics.FillEllipse(&stars, x, y, size, size);
            }

            for (int i = 6; i >= 1; --i) {
                const float radius = 74.0f + i * 27.0f;
                SolidBrush glow(Color(12, 224, 232, 255));
                graphics.FillEllipse(&glow, 685.0f - radius, 185.0f - radius,
                    radius * 2.0f, radius * 2.0f);
            }
            SolidBrush moon(Color(255, 234, 239, 211));
            graphics.FillEllipse(&moon, 615.0f, 115.0f, 140.0f, 140.0f);
            SolidBrush moonShade(Color(255, 192, 208, 202));
            graphics.FillEllipse(&moonShade, 646.0f, 126.0f, 94.0f, 116.0f);

            GraphicsPath mountain;
            PointF mountainPoints[] = {
                PointF(0, 650), PointF(110, 515), PointF(260, 580),
                PointF(390, 455), PointF(520, 600), PointF(700, 500),
                PointF(820, 585), PointF(900, 530), PointF(900, 900),
                PointF(0, 900)
            };
            mountain.AddPolygon(mountainPoints, 10);
            SolidBrush mountainBrush(Color(255, 15, 29, 45));
            graphics.FillPath(&mountainBrush, &mountain);

            LinearGradientBrush lake(Rect(0, 615, 900, 285),
                Color(255, 21, 66, 84),
                Color(255, 7, 20, 35),
                LinearGradientModeVertical);
            graphics.FillRectangle(&lake, 0, 615, 900, 285);

            SolidBrush cabin(Color(255, 23, 35, 42));
            graphics.FillRectangle(&cabin, 180.0f, 655.0f, 92.0f, 52.0f);
            SolidBrush roof(Color(255, 13, 22, 28));
            PointF roofPoints[] = {
                PointF(166, 658), PointF(226, 616), PointF(286, 658)
            };
            GraphicsPath roofPath;
            roofPath.AddPolygon(roofPoints, 3);
            graphics.FillPath(&roof, &roofPath);
            SolidBrush windowLight(Color(255, 255, 192, 89));
            graphics.FillRectangle(&windowLight, 212.0f, 671.0f, 22.0f, 20.0f);

            Pen reflection(Color(75, 188, 221, 222), 3.0f);
            for (int i = 0; i < 8; ++i) {
                const float y = 710.0f + i * 20.0f;
                graphics.DrawLine(&reflection, 120.0f + i * 11.0f, y,
                    780.0f - i * 9.0f, y);
            }
        }
        else {
            LinearGradientBrush background(Point(0, 0), Point(900, 900),
                Color(255, 241, 220, 183),
                Color(255, 28, 74, 79));
            graphics.FillRectangle(&background, 0, 0, 900, 900);

            SolidBrush largeCircle(Color(255, 222, 92, 72));
            graphics.FillEllipse(&largeCircle, 485.0f, 95.0f, 360.0f, 360.0f);
            SolidBrush tealCircle(Color(220, 27, 96, 99));
            graphics.FillEllipse(&tealCircle, 60.0f, 500.0f, 330.0f, 330.0f);
            SolidBrush creamCircle(Color(230, 252, 238, 204));
            graphics.FillEllipse(&creamCircle, 565.0f, 535.0f, 250.0f, 250.0f);

            GraphicsState state = graphics.Save();
            graphics.TranslateTransform(410.0f, 430.0f);
            for (int i = 0; i < 12; ++i) {
                graphics.RotateTransform(30.0f);
                SolidBrush petal(Color(135, 255, 232, 187));
                graphics.FillEllipse(&petal, -58.0f, -325.0f, 116.0f, 410.0f);
            }
            graphics.Restore(state);

            SolidBrush core(Color(255, 238, 126, 77));
            graphics.FillEllipse(&core, 345.0f, 365.0f, 130.0f, 130.0f);
            SolidBrush coreInner(Color(255, 255, 224, 167));
            graphics.FillEllipse(&coreInner, 378.0f, 398.0f, 64.0f, 64.0f);

            Pen ring(Color(140, 255, 241, 210), 5.0f);
            graphics.DrawEllipse(&ring, 160.0f, 170.0f, 560.0f, 520.0f);
            graphics.DrawEllipse(&ring, 215.0f, 225.0f, 450.0f, 410.0f);

            SolidBrush dots(Color(150, 255, 241, 210));
            for (int i = 0; i < 18; ++i) {
                const float angle = random(rng) * 6.2831853f;
                const float radius = 270.0f + random(rng) * 130.0f;
                const float size = 3.0f + random(rng) * 8.0f;
                graphics.FillEllipse(&dots,
                    450.0f + std::cos(angle) * radius,
                    450.0f + std::sin(angle) * radius,
                    size, size);
            }
        }

        return bitmap;
    }

    bool loadBuiltinImage(int index) {
        auto nextImage = createBuiltinImage(index);
        if (!nextImage) {
            return false;
        }

        static const wchar_t* names[] = {
            L"暮色山谷", L"星夜湖泊", L"几何花园"
        };
        g_image = std::move(nextImage);
        g_builtinIndex = index;
        g_imageName = names[index];
        return true;
    }

    void switchToNextBuiltin() {
        std::random_device device;
        const int offset = 1 + static_cast<int>(device() % 2u);
        const int next = (g_builtinIndex + offset) % 3;
        loadBuiltinImage(next);
        startNewGame();
    }

    bool chooseImageFile() {
        wchar_t fileName[MAX_PATH] = L"";
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = g_window;
        dialog.lpstrFilter =
            L"图片文件\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0所有文件\0*.*\0\0";
        dialog.lpstrFile = fileName;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrTitle = L"选择一张图片";
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        if (!GetOpenFileNameW(&dialog)) {
            return false;
        }

        std::unique_ptr<Gdiplus::Image> loaded(
            Gdiplus::Image::FromFile(fileName, FALSE));
        if (!loaded || loaded->GetLastStatus() != Gdiplus::Ok ||
            loaded->GetWidth() < 32 || loaded->GetHeight() < 32) {
            MessageBoxW(g_window, L"无法读取这张图片，请选择 PNG、JPG、BMP 或 GIF 文件。",
                L"图片加载失败", MB_OK | MB_ICONERROR);
            return false;
        }

        std::wstring path(fileName);
        const size_t slash = path.find_last_of(L"\\/");
        g_imageName = slash == std::wstring::npos ? path : path.substr(slash + 1);
        g_image = std::move(loaded);
        startNewGame();
        return true;
    }

    // ------------------------- 游戏操作 -------------------------
    void showSuccessMessage(bool challengeCompleted) {
        const std::wstring title = challengeCompleted ? L"挑战成功" : L"拼图完成";
        std::wstring message = L"恭喜你完成了拼图！\n\n";
        message += L"难度：" + std::to_wstring(g_grid) + L" × " +
            std::to_wstring(g_grid) + L"\n";
        message += L"用时：" + formatTime(g_elapsedSeconds) + L"\n";
        message += L"步数：" + std::to_wstring(g_moves);
        MessageBoxW(g_window, message.c_str(), title.c_str(),
            MB_OK | MB_ICONINFORMATION);
    }

    void swapTiles(int first, int second) {
        if (first < 0 || second < 0 || first == second ||
            first >= static_cast<int>(g_tiles.size()) ||
            second >= static_cast<int>(g_tiles.size())) {
            return;
        }

        std::swap(g_tiles[first], g_tiles[second]);
        ++g_moves;
        if (isSolved()) {
            const bool challengeCompleted = g_challenge;
            g_solved = true;
            g_challenge = false;
            g_viewOriginal = false;
            g_hoverCell = -1;
            repaint();
            showSuccessMessage(challengeCompleted);
            repaint();
        }
    }

    void handleAction(Action action) {
        switch (action) {
        case Action::NewGame:
            cancelDrag();
            startNewGame();
            break;
        case Action::NextImage:
            cancelDrag();
            switchToNextBuiltin();
            break;
        case Action::ChooseImage:
            cancelDrag();
            chooseImageFile();
            break;
        case Action::ToggleOriginal:
            if (!g_solved && !g_timedOut) {
                cancelDrag();
                g_viewOriginal = !g_viewOriginal;
                repaint();
            }
            break;
        case Action::ToggleChallenge:
            cancelDrag();
            if (g_challenge) {
                g_challenge = false;
                repaint();
            }
            else {
                g_challenge = true;
                startNewGame();
            }
            break;
        case Action::Difficulty3:
            if (g_grid != 3) {
                g_grid = 3;
                cancelDrag();
                startNewGame();
            }
            break;
        case Action::Difficulty4:
            if (g_grid != 4) {
                g_grid = 4;
                cancelDrag();
                startNewGame();
            }
            break;
        case Action::Difficulty5:
            if (g_grid != 5) {
                g_grid = 5;
                cancelDrag();
                startNewGame();
            }
            break;
        default:
            break;
        }
    }

    // ------------------------- 绘图 -------------------------
    void drawHeader(Gdiplus::Graphics& graphics, int width) {
        drawRoundedRect(graphics, { 28.0f, 22.0f, 5.0f, 42.0f }, 2.5f, kAccent);
        drawText(graphics, L"拼图工坊",
            { 46.0f, 15.0f, 250.0f, 38.0f }, 28.0f, kText,
            Gdiplus::FontStyleBold);
        drawText(graphics, L"拖动拼块 · 拼出完整画面",
            { 47.0f, 52.0f, 300.0f, 20.0f }, 13.0f, kMuted);

        const float pillWidth = std::min(300.0f, std::max(230.0f, width * 0.26f));
        UiRect pill{ static_cast<float>(width) - 28.0f - pillWidth, 19.0f, pillWidth, 54.0f };
        drawRoundedRect(graphics, pill, 13.0f, Gdiplus::Color(255, 25, 31, 43), kBorder);
        drawText(graphics, L"当前图片",
            { pill.x + 15.0f, pill.y + 6.0f, pill.w - 30.0f, 16.0f },
            11.0f, kMuted);
        drawText(graphics, g_imageName,
            { pill.x + 15.0f, pill.y + 24.0f, pill.w - 30.0f, 22.0f },
            14.0f, kText, Gdiplus::FontStyleRegular,
            Gdiplus::StringAlignmentNear, Gdiplus::StringAlignmentCenter);

        Gdiplus::Pen line(Gdiplus::Color(90, 63, 73, 90), 1.0f);
        graphics.DrawLine(&line, 28.0f, 86.0f,
            static_cast<float>(width) - 28.0f, 86.0f);
    }

    void drawBoard(Gdiplus::Graphics& graphics, const Layout& layout) {
        const UiRect board = layout.board;
        drawRoundedRect(graphics, { board.x + 6.0f, board.y + 9.0f, board.w, board.h },
            20.0f, Gdiplus::Color(100, 0, 0, 0));
        drawRoundedRect(graphics, board, 18.0f, Gdiplus::Color(255, 15, 19, 28), kBorder);

        Gdiplus::GraphicsPath clip;
        addRoundedRectangle(clip, toRectF(board), 18.0f);
        graphics.SetClip(&clip);

        if (!g_image) {
            drawText(graphics, L"正在准备图片…", board, 18.0f, kMuted,
                Gdiplus::FontStyleRegular, Gdiplus::StringAlignmentCenter);
            graphics.ResetClip();
            return;
        }

        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

        if (g_viewOriginal) {
            graphics.DrawImage(g_image.get(), toRectF(board));
        }
        else {
            const float cellSize = board.w / static_cast<float>(g_grid);
            const float gap = std::max(3.0f, cellSize * 0.010f);

            for (int cell = 0; cell < g_grid * g_grid; ++cell) {
                if (g_dragCell == cell) {
                    continue;
                }
                const int row = cell / g_grid;
                const int column = cell % g_grid;
                UiRect destination{
                    board.x + column * cellSize + gap * 0.5f,
                    board.y + row * cellSize + gap * 0.5f,
                    cellSize - gap,
                    cellSize - gap
                };
                drawImagePiece(graphics, destination, g_tiles[cell]);
                Gdiplus::Pen pieceEdge(Gdiplus::Color(50, 255, 255, 255), 1.0f);
                graphics.DrawRectangle(&pieceEdge, destination.x, destination.y,
                    destination.w, destination.h);
            }

            if (g_dragCell >= 0 && g_hoverCell >= 0 &&
                g_hoverCell != g_dragCell) {
                const int row = g_hoverCell / g_grid;
                const int column = g_hoverCell % g_grid;
                UiRect target{
                    board.x + column * cellSize + gap * 0.5f,
                    board.y + row * cellSize + gap * 0.5f,
                    cellSize - gap, cellSize - gap
                };
                drawRoundedRect(graphics, target, 3.0f,
                    Gdiplus::Color(45, 242, 172, 87), kAccent, 2.0f);
            }

            if (g_dragCell >= 0) {
                const int sourceRow = g_dragCell / g_grid;
                const int sourceColumn = g_dragCell % g_grid;
                const float cellX = board.x + sourceColumn * cellSize;
                const float cellY = board.y + sourceRow * cellSize;
                UiRect floating{
                    static_cast<float>(g_mouse.x) - g_dragOffsetX,
                    static_cast<float>(g_mouse.y) - g_dragOffsetY,
                    cellSize, cellSize
                };

                drawRoundedRect(graphics,
                    { floating.x + 6.0f, floating.y + 9.0f,
                     floating.w - gap, floating.h - gap },
                    5.0f, Gdiplus::Color(130, 0, 0, 0));

                Gdiplus::GraphicsPath pieceClip;
                UiRect inset{ floating.x + gap * 0.5f, floating.y + gap * 0.5f,
                             floating.w - gap, floating.h - gap };
                addRoundedRectangle(pieceClip, toRectF(inset), 5.0f);
                graphics.SetClip(&pieceClip);
                drawImagePiece(graphics, inset, g_tiles[g_dragCell]);
                graphics.ResetClip();
                graphics.SetClip(&clip);
                Gdiplus::Pen border(kAccent, 2.0f);
                graphics.DrawRectangle(&border, inset.x, inset.y, inset.w, inset.h);

                // 保留一个明显的拖动起点，避免用户误以为拼块没有移动。
                Gdiplus::Pen ghost(Gdiplus::Color(70, 242, 172, 87), 1.0f);
                graphics.DrawRectangle(&ghost, cellX + gap, cellY + gap,
                    cellSize - gap * 2.0f, cellSize - gap * 2.0f);
            }
        }

        graphics.ResetClip();

        if (g_viewOriginal) {
            UiRect badge{ board.x + 16.0f, board.y + 16.0f, 120.0f, 34.0f };
            drawRoundedRect(graphics, badge, 10.0f, Gdiplus::Color(210, 15, 19, 28),
                Gdiplus::Color(150, 242, 172, 87));
            drawText(graphics, L"原图预览", badge, 13.0f, kAccent,
                Gdiplus::FontStyleRegular, Gdiplus::StringAlignmentCenter);
        }

        if (g_challenge && !g_solved) {
            UiRect badge{ board.x + board.w - 158.0f, board.y + 16.0f, 142.0f, 34.0f };
            const int remaining = std::max(0, g_challengeLimit - g_elapsedSeconds);
            drawRoundedRect(graphics, badge, 10.0f, Gdiplus::Color(215, 15, 19, 28),
                remaining <= 30 ? kDanger : kBorder);
            drawText(graphics, L"剩余 " + formatTime(remaining), badge, 13.0f,
                remaining <= 30 ? kDanger : kText,
                Gdiplus::FontStyleBold, Gdiplus::StringAlignmentCenter);
        }

        if (g_solved || g_timedOut) {
            drawRoundedRect(graphics, board, 18.0f, Gdiplus::Color(178, 8, 11, 17));
            const float cardWidth = std::min(350.0f, board.w - 70.0f);
            const float cardHeight = 178.0f;
            UiRect card{
                board.x + (board.w - cardWidth) * 0.5f,
                board.y + (board.h - cardHeight) * 0.5f,
                cardWidth, cardHeight
            };
            drawRoundedRect(graphics, card, 18.0f, Gdiplus::Color(255, 26, 32, 44), kBorder);
            drawText(graphics, g_solved ? L"拼图完成" : L"挑战结束",
                { card.x + 20.0f, card.y + 22.0f, card.w - 40.0f, 42.0f },
                25.0f, g_solved ? kSuccess : kDanger,
                Gdiplus::FontStyleBold, Gdiplus::StringAlignmentCenter);
            drawText(graphics,
                L"用时 " + formatTime(g_elapsedSeconds) +
                L"   ·   步数 " + std::to_wstring(g_moves),
                { card.x + 20.0f, card.y + 76.0f, card.w - 40.0f, 24.0f },
                14.0f, kMuted, Gdiplus::FontStyleRegular,
                Gdiplus::StringAlignmentCenter);
            drawText(graphics, g_solved ? L"点击“重新打乱”再来一局" : L"点击“重新打乱”再次挑战",
                { card.x + 20.0f, card.y + 112.0f, card.w - 40.0f, 24.0f },
                12.0f, kText, Gdiplus::FontStyleRegular,
                Gdiplus::StringAlignmentCenter);
        }
    }

    void drawPanel(Gdiplus::Graphics& graphics, const Layout& layout) {
        const UiRect panel = layout.panel;
        drawRoundedRect(graphics, panel, 18.0f, kPanel, kBorder);

        const float px = panel.x + 18.0f;
        const float py = panel.y;
        const float innerWidth = panel.w - 36.0f;

        drawText(graphics, L"难度等级", { px, py + 16.0f, innerWidth, 20.0f },
            12.0f, kMuted);
        drawText(graphics, difficultyText(g_grid),
            { px, py + 16.0f, innerWidth, 20.0f }, 12.0f, kAccent,
            Gdiplus::FontStyleRegular, Gdiplus::StringAlignmentFar);

        const wchar_t* labels[] = { L"3 × 3", L"4 × 4", L"5 × 5" };
        for (int i = 0; i < 3; ++i) {
            drawButton(graphics, layout.difficulty[i], labels[i], false,
                g_grid == i + 3);
        }

        Gdiplus::Pen divider(Gdiplus::Color(105, 58, 69, 86), 1.0f);
        graphics.DrawLine(&divider, px, py + 94.0f, px + innerWidth, py + 94.0f);

        drawText(graphics, L"游戏控制", { px, py + 106.0f, innerWidth, 20.0f },
            12.0f, kMuted);

        drawButton(graphics, layout.newGame, L"重新打乱", true);
        drawButton(graphics, layout.nextImage, L"随机切换内置图片");
        drawButton(graphics, layout.chooseImage, L"选择本地图片…");
        drawButton(graphics, layout.original,
            g_viewOriginal ? L"返回拼图" : L"查看原图",
            false, g_viewOriginal);
        drawButton(graphics, layout.challenge,
            g_challenge ? L"结束挑战" : L"开始限时挑战",
            false, g_challenge, g_challenge);

        graphics.DrawLine(&divider, px, py + 368.0f, px + innerWidth, py + 368.0f);
        drawText(graphics, L"本局状态", { px, py + 380.0f, innerWidth, 20.0f },
            12.0f, kMuted);

        const auto drawStatRow = [&](float y, const std::wstring& label,
            const std::wstring& value,
            const Gdiplus::Color& color) {
                drawText(graphics, label, { px, y, 84.0f, 22.0f }, 13.0f, kMuted);
                drawText(graphics, value, { px + 84.0f, y, innerWidth - 84.0f, 22.0f },
                    14.0f, color, Gdiplus::FontStyleBold,
                    Gdiplus::StringAlignmentFar);
            };

        drawStatRow(py + 404.0f, L"用时", formatTime(g_elapsedSeconds), kText);
        drawStatRow(py + 430.0f, L"步数", std::to_wstring(g_moves), kText);
        if (g_challenge) {
            const int remaining = std::max(0, g_challengeLimit - g_elapsedSeconds);
            drawStatRow(py + 456.0f, L"剩余", formatTime(remaining),
                remaining <= 30 ? kDanger : kAccent);
        }
        else {
            drawStatRow(py + 456.0f, L"难度", std::to_wstring(g_grid) + L" × " +
                std::to_wstring(g_grid), kAccent);
        }

        std::wstring status;
        Gdiplus::Color statusColor = kMuted;
        if (g_solved) {
            status = L"拼图完成\n用时 " + formatTime(g_elapsedSeconds) +
                L" · 步数 " + std::to_wstring(g_moves);
            statusColor = kSuccess;
        }
        else if (g_timedOut) {
            status = L"挑战超时\n重新打乱后再试一次";
            statusColor = kDanger;
        }
        else if (g_viewOriginal) {
            status = L"正在查看原图\n点击按钮返回拼图";
            statusColor = kAccent;
        }
        else if (g_challenge) {
            status = L"挑战进行中\n在倒计时结束前完成拼图";
            statusColor = kAccent;
        }
        else {
            status = L"拖动任意两块\n交换位置完成拼图";
        }

        drawRoundedRect(graphics, { px, py + 488.0f, innerWidth, 52.0f },
            9.0f, Gdiplus::Color(255, 20, 25, 35));
        drawText(graphics, status, { px + 10.0f, py + 492.0f,
                 innerWidth - 20.0f, 44.0f }, 12.0f, statusColor,
            Gdiplus::FontStyleRegular, Gdiplus::StringAlignmentCenter,
            Gdiplus::StringAlignmentCenter, true);
    }

    void drawBackground(Gdiplus::Graphics& graphics, int width, int height) {
        Gdiplus::LinearGradientBrush background(
            Gdiplus::Rect(0, 0, width, height), kBackgroundTop,
            kBackgroundBottom, Gdiplus::LinearGradientModeVertical);
        graphics.FillRectangle(&background, 0, 0, width, height);

        Gdiplus::SolidBrush glow(Gdiplus::Color(18, 242, 172, 87));
        graphics.FillEllipse(&glow, static_cast<float>(width) - 310.0f, -120.0f,
            420.0f, 420.0f);
        Gdiplus::SolidBrush coolGlow(Gdiplus::Color(11, 84, 155, 179));
        graphics.FillEllipse(&coolGlow, -130.0f, static_cast<float>(height) - 260.0f,
            420.0f, 420.0f);
    }

    void drawClient(Gdiplus::Graphics& graphics, int width, int height) {
        drawBackground(graphics, width, height);
        drawHeader(graphics, width);
        const Layout layout = makeLayout(width, height);
        drawBoard(graphics, layout);
        drawPanel(graphics, layout);
    }

    void paintWindow(HWND window) {
        PAINTSTRUCT paint{};
        HDC deviceContext = BeginPaint(window, &paint);

        RECT client{};
        GetClientRect(window, &client);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;

        if (width > 0 && height > 0) {
            Gdiplus::Bitmap canvas(width, height, PixelFormat32bppPARGB);
            Gdiplus::Graphics graphics(&canvas);
            if (graphics.GetLastStatus() == Gdiplus::Ok) {
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
                drawClient(graphics, width, height);

                Gdiplus::Graphics screen(deviceContext);
                screen.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
                screen.DrawImage(&canvas, 0, 0, width, height);
            }
        }

        EndPaint(window, &paint);
    }

    // ------------------------- 初始化 -------------------------
    void initializeGame() {
        loadBuiltinImage(0);
        g_grid = 3;
        g_challenge = false;
        g_challengeLimit = challengeSeconds(g_grid);
        startNewGame();
    }

    // ------------------------- 窗口过程 -------------------------
    LRESULT CALLBACK windowProcedure(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
            g_window = window;
            initializeGame();
            SetTimer(window, kTimerId, 1000, nullptr);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paintWindow(window);
            return 0;

        case WM_SIZE:
            repaint();
            return 0;

        case WM_GETMINMAXINFO: {
            auto* information = reinterpret_cast<MINMAXINFO*>(lParam);
            information->ptMinTrackSize.x = 1040;
            information->ptMinTrackSize.y = 720;
            return 0;
        }

        case WM_TIMER:
            if (wParam == kTimerId && !g_solved && !g_timedOut) {
                ++g_elapsedSeconds;
                if (g_challenge && g_elapsedSeconds >= g_challengeLimit) {
                    g_timedOut = true;
                    g_challenge = false;
                    g_dragCell = -1;
                    g_hoverCell = -1;
                    repaint();
                    MessageBoxW(window,
                        L"挑战时间到！\n\n点击“重新打乱”可以再次挑战。",
                        L"挑战失败", MB_OK | MB_ICONWARNING);
                }
                else {
                    repaint();
                }
            }
            return 0;

        case WM_MOUSEMOVE: {
            g_mouse.x = GET_X_LPARAM(lParam);
            g_mouse.y = GET_Y_LPARAM(lParam);
            if (!g_trackingMouse) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = window;
                TrackMouseEvent(&track);
                g_trackingMouse = true;
            }

            if (g_dragCell >= 0) {
                g_hoverCell = cellAtPoint(static_cast<float>(g_mouse.x),
                    static_cast<float>(g_mouse.y));
            }
            repaint();
            return 0;
        }

        case WM_MOUSELEAVE:
            g_trackingMouse = false;
            g_mouse = { -1000, -1000 };
            if (g_dragCell < 0) {
                g_hoverCell = -1;
            }
            repaint();
            return 0;

        case WM_LBUTTONDOWN: {
            SetFocus(window);
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));
            g_mouse = { static_cast<LONG>(x), static_cast<LONG>(y) };

            const Action action = hitAction(x, y);
            if (action != Action::None) {
                handleAction(action);
                return 0;
            }

            if (g_solved || g_timedOut || g_viewOriginal) {
                return 0;
            }

            const int cell = cellAtPoint(x, y);
            if (cell < 0) {
                return 0;
            }

            RECT client{};
            GetClientRect(window, &client);
            const Layout layout = makeLayout(client.right, client.bottom);
            const float cellWidth = layout.board.w / static_cast<float>(g_grid);
            const int row = cell / g_grid;
            const int column = cell % g_grid;

            g_dragCell = cell;
            g_hoverCell = cell;
            g_dragOffsetX = x - (layout.board.x + column * cellWidth);
            g_dragOffsetY = y - (layout.board.y + row * cellWidth);
            SetCapture(window);
            repaint();
            return 0;
        }

        case WM_LBUTTONUP: {
            if (g_dragCell < 0) {
                return 0;
            }

            const int from = g_dragCell;
            const int to = cellAtPoint(static_cast<float>(GET_X_LPARAM(lParam)),
                static_cast<float>(GET_Y_LPARAM(lParam)));
            g_dragCell = -1;
            g_hoverCell = -1;
            if (GetCapture() == window) {
                ReleaseCapture();
            }

            if (to >= 0 && to != from) {
                swapTiles(from, to);
            }
            repaint();
            return 0;
        }

        case WM_CAPTURECHANGED:
            if (g_dragCell >= 0) {
                g_dragCell = -1;
                g_hoverCell = -1;
                repaint();
            }
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                POINT point{};
                GetCursorPos(&point);
                ScreenToClient(window, &point);
                if (g_dragCell >= 0) {
                    SetCursor(LoadCursorW(nullptr, IDC_SIZEALL));
                    return TRUE;
                }

                const Action action = hitAction(
                    static_cast<float>(point.x), static_cast<float>(point.y));
                RECT client{};
                GetClientRect(window, &client);
                const Layout layout = makeLayout(client.right, client.bottom);
                if (action != Action::None ||
                    (!g_solved && !g_timedOut && !g_viewOriginal &&
                        layout.board.contains(static_cast<float>(point.x),
                            static_cast<float>(point.y)))) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (g_dragCell >= 0) {
                    cancelDrag();
                }
                else if (g_viewOriginal) {
                    g_viewOriginal = false;
                    repaint();
                }
            }
            else if (wParam == 'N') {
                handleAction(Action::NewGame);
            }
            else if (wParam == 'O') {
                handleAction(Action::ToggleOriginal);
            }
            else if (wParam == '1') {
                handleAction(Action::Difficulty3);
            }
            else if (wParam == '2') {
                handleAction(Action::Difficulty4);
            }
            else if (wParam == '3') {
                handleAction(Action::Difficulty5);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(window, kTimerId);
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    SetProcessDPIAware();

    Gdiplus::GdiplusStartupInput startupInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &startupInput, nullptr) !=
        Gdiplus::Ok) {
        MessageBoxW(nullptr, L"GDI+ 初始化失败，无法启动游戏。",
            L"拼图工坊", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_fontFamily = new Gdiplus::FontFamily(L"Microsoft YaHei UI");
    g_windowBrush = CreateSolidBrush(RGB(12, 16, 24));

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = g_windowBrush;
    windowClass.lpszClassName = kWindowClass;
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&windowClass)) {
        MessageBoxW(nullptr, L"窗口注册失败。", L"拼图工坊",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rectangle{ 0, 0, 1220, 820 };
    AdjustWindowRect(&rectangle, style, FALSE);
    const int width = rectangle.right - rectangle.left;
    const int height = rectangle.bottom - rectangle.top;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    HWND window = CreateWindowExW(
        0, kWindowClass, kWindowTitle, style, x, y, width, height,
        nullptr, nullptr, instance, nullptr);

    if (!window) {
        MessageBoxW(nullptr, L"游戏窗口创建失败。", L"拼图工坊",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_image.reset();
    delete g_fontFamily;
    g_fontFamily = nullptr;
    DeleteObject(g_windowBrush);
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return static_cast<int>(message.wParam);
}
