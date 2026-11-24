// Source/app_gui/main.cpp
// Person D - "a new window that is easy to look at".
//
// A Win32 + GDI app that wires the whole pipeline together:
//     A (Maze) -> B (Graph) -> C (Dijkstra) -> D (Visualization)
// and paints it in a real window: the maze, Start/End, visited shading, the
// shortest path, an animated replay of the search, and a stats strip.
//
// It does NOT reimplement any algorithm: it only drives the shared modules in
// the static library `pathgo_core` and the host-agnostic frames produced by
// visualization::Renderer (Source/include/Visualization.h). That split is the
// point (docs/Q&A.md: "the same code works for a console, D's window and E's
// tests"): the core stays testable, the window stays a thin renderer.
//
// Text is drawn with the ANSI (A) GDI APIs because every label is ASCII; that
// keeps the file free of wide-char conversion. Build: the PathGo_GUI target in
// CMakeLists.txt.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "Maze.h"
#include "Graph.h"
#include "Dijkstra.h"
#include "Visualization.h"

using Clock = std::chrono::steady_clock;

namespace {

    // ---- control ids (one per button) ----
    const int kIdLoadFile = 1001;
    const int kIdGenerate = 1002;
    const int kIdRun      = 1003;
    const int kIdAnimate  = 1004;
    const int kIdReset    = 1005;

    // Colours per cell state. The index must match visualization::CellState.
    const COLORREF kColors[] = {
        RGB(35, 35, 35),    // Wall
        RGB(255, 255, 255), // Floor
        RGB(185, 215, 250), // Weighted
        RGB(255, 198, 120), // Visited
        RGB(80, 200, 80),   // Path
        RGB(225, 80, 80),   // Start
        RGB(80, 130, 225),  // Goal
    };
    static_assert(7 == (int)(sizeof(kColors) / sizeof(kColors[0])),
                  "kColors must cover every CellState");

    // ---- the app state, one global set for the message loop ----
    struct App {
        HWND hwnd = nullptr;
        Maze maze;
        graph::Graph g;
        dijkstra::Result result;
        double searchMs = 0.0;

        int visitedUpTo = -1;   // -1 = not animating, else 0..total
        bool animating = false;
        int animStep = 2;        // visited nodes revealed per tick
        UINT_PTR timer = 0;
    } app;

    void refresh(HWND wnd, int visitedUpTo) {
        app.visitedUpTo = visitedUpTo;
        if (wnd) {
            InvalidateRect(wnd, nullptr, TRUE);
        }
    }

    void clearSearch() {
        app.result = dijkstra::Result{};
        app.searchMs = 0.0;
        app.animating = false;
        app.visitedUpTo = -1;
    }

    // Build B's graph and run C's search against the current maze.
    void solve() {
        app.g = graph::Graph::fromMaze(app.maze);
        if (app.g.startVertex() < 0 || app.g.endVertex() < 0) {
            app.result = dijkstra::Result{};
            app.searchMs = 0.0;
            return;
        }
        const std::vector<int> targets = {app.g.endVertex()};
        const Clock::time_point begin = Clock::now();
        app.result = dijkstra::findShortestPath(app.g, app.g.startVertex(), targets);
        app.searchMs =
            std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    }

    void advanceTick() {
        const int total = static_cast<int>(app.result.nodesExpanded);
        app.visitedUpTo += app.animStep;
        if (app.visitedUpTo >= total) {
            app.visitedUpTo = total;
            app.animating = false;
            if (app.timer != 0) {
                KillTimer(app.hwnd, app.timer);
                app.timer = 0;
            }
        }
        refresh(app.hwnd, app.visitedUpTo);
    }

    void loadFile(HWND wnd) {
        char path[MAX_PATH] = {0};
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = wnd;
        ofn.lpstrFilter = "Text files\0*.txt\0All files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileNameA(&ofn)) {
            return;  // the user cancelled
        }
        std::string err;
        Maze m;
        if (m.loadFromFile(path, &err)) {
            app.maze = std::move(m);
            clearSearch();
            refresh(wnd, -1);
        } else {
            MessageBoxA(wnd, "Cannot load that maze file.", "Load", MB_ICONWARNING);
        }
    }

    void generateRandom(HWND wnd) {
        const int size = 21;  // a good default for a readable window
        const unsigned seed = static_cast<unsigned>(GetTickCount());
        app.maze.generateRandom(size, size, seed);
        clearSearch();
        refresh(wnd, -1);
    }

    void runSearch(HWND wnd) {
        solve();
        app.animating = false;
        refresh(wnd, -1);  // show the complete result at once
    }

    void animate(HWND wnd) {
        if (app.result.visitOrder.empty()) {
            solve();
        }
        if (app.result.visitOrder.empty()) {
            refresh(wnd, -1);
            return;
        }
        app.visitedUpTo = 0;
        app.animating = true;
        if (app.timer == 0) {
            app.timer = SetTimer(wnd, 1, 30, nullptr);  // ~33 fps
        }
        refresh(wnd, 0);
    }

    void reset(HWND wnd) {
        clearSearch();
        refresh(wnd, -1);
    }

    // Build the ASCII stats line for the panel.
    std::string statsLine(const visualization::SearchStats& s) {
        const double ms = s.searchMs;
        char msbuf[32];
        sprintf(msbuf, "%.2f", ms);
        std::string line;
        if (s.reachable) {
            line = "Reachable: YES    Cost: " + std::to_string(s.totalCost) +
                   "    Moves: " + std::to_string(s.moves) +
                   "    Visited: " + std::to_string(s.nodesVisitedShown) + " / " +
                   std::to_string(s.nodesVisitedTotal) + "    Search: " + msbuf + " ms";
        } else {
            line = "Reachable: NO     Visited: " + std::to_string(s.nodesVisitedShown) +
                   " / " + std::to_string(s.nodesVisitedTotal) + "    Search: " + msbuf +
                   " ms";
        }
        return line;
    }

    // ---- painting ----
    void paint(HWND wnd, HDC target) {
        HDC dc = CreateCompatibleDC(target);
        HBITMAP bmp = CreateCompatibleBitmap(target, 1, 1);
        HBITMAP oldBmp = (HBITMAP)SelectObject(dc, bmp);

        RECT rc{};
        GetClientRect(wnd, &rc);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;

        HBRUSH bg = CreateSolidBrush(RGB(245, 245, 245));
        FillRect(dc, &rc, bg);
        DeleteObject(bg);

        const int rows = app.maze.rows();
        const int cols = app.maze.cols();

        if (rows == 0 || cols == 0) {
            SetBkMode(dc, TRANSPARENT);
            HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SelectObject(dc, f);
            TextOutA(dc, 16, 40,
                    "Use the buttons above to load or generate a maze, then Run / Animate.",
                    70);
            SelectObject(dc, oldBmp);
            DeleteObject(bmp);
            DeleteDC(dc);
            return;
        }

        // Layout: a strip holds the buttons, the stats sit right under it, and
        // the maze grid fills the rest.
        const int topStrip = 60;
        const int statsH = 30;
        const int pad = 12;
        const int availW = w - 2 * pad;
        const int availH = h - topStrip - statsH - 2 * pad;
        int cs = availW / cols;  // pixel size of one cell
        if (availH / rows < cs) {
            cs = availH / rows;
        }
        if (cs < 3) {
            cs = 3;
        }
        const int gridW = cs * cols;
        const int gridH = cs * rows;
        const int ox = pad + (availW - gridW) / 2;
        const int oy = topStrip + statsH + pad + (availH - gridH) / 2;

        const visualization::Frame frame = visualization::Renderer::buildFrame(
            app.maze, app.g, app.result, app.searchMs, app.visitedUpTo);

        // Cells.
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const visualization::Cell& cc =
                    frame.cells[static_cast<std::size_t>(c + r * cols)];
                HBRUSH b = CreateSolidBrush(kColors[static_cast<int>(cc.state)]);
                RECT cellRect = {ox + c * cs, oy + r * cs,
                                 ox + (c + 1) * cs, oy + (r + 1) * cs};
                FillRect(dc, &cellRect, b);
                DeleteObject(b);

                if (cc.state == visualization::CellState::Weighted && cs >= 12) {
                    char num[8];
                    sprintf(num, "%d", cc.weight);
                    SetTextColor(dc, RGB(15, 40, 90));
                    SetBkMode(dc, TRANSPARENT);
                    SetTextAlign(dc, TA_CENTER | TA_BASELINE);
                    TextOutA(dc, (cellRect.left + cellRect.right) / 2, cellRect.top + 1,
                             num, (int)strlen(num));
                    SetTextAlign(dc, TA_LEFT | TA_TOP);
                }
            }
        }

        // Start / Goal letters.
        auto drawLetter = [&](int r, int c, const char* ch, COLORREF color) {
            if (r < 0 || c < 0) {
                return;
            }
            SetTextColor(dc, color);
            SetBkMode(dc, TRANSPARENT);
            SetTextAlign(dc, TA_CENTER | TA_BASELINE);
            TextOutA(dc, ox + c * cs + cs / 2, oy + r * cs + 1, ch, 1);
            SetTextAlign(dc, TA_LEFT | TA_TOP);
        };
        drawLetter(frame.startRow, frame.startCol, "S", RGB(255, 255, 255));
        drawLetter(frame.endRow, frame.endCol, "E", RGB(255, 255, 255));

        // Stats strip, right under the buttons.
        const std::string line = statsLine(frame.stats);
        SetBkMode(dc, OPAQUE);
        SetTextColor(dc, RGB(20, 20, 20));
        HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SelectObject(dc, f);
        TextOutA(dc, pad, topStrip + 2, line.c_str(), (int)line.size());

        SelectObject(dc, oldBmp);
        DeleteObject(bmp);
        DeleteDC(dc);
    }

    LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CREATE:
                app.hwnd = hwnd;
                return 0;

            case WM_COMMAND:
                switch (LOWORD(wp)) {
                    case kIdLoadFile:
                        loadFile(hwnd);
                        return 0;
                    case kIdGenerate:
                        generateRandom(hwnd);
                        return 0;
                    case kIdRun:
                        runSearch(hwnd);
                        return 0;
                    case kIdAnimate:
                        animate(hwnd);
                        return 0;
                    case kIdReset:
                        reset(hwnd);
                        return 0;
                    default:
                        return 0;
                }

            case WM_TIMER:
                if (wp == 1 && app.animating) {
                    advanceTick();
                }
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(hwnd, &ps);
                paint(hwnd, dc);
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;  // the paint handler fills the whole client area

            case WM_DESTROY:
                if (app.timer != 0) {
                    KillTimer(hwnd, app.timer);
                    app.timer = 0;
                }
                PostQuitMessage(0);
                return 0;

            default:
                return DefWindowProcW(hwnd, msg, wp, lp);
        }
    }

    void buildButtons(HWND hwnd) {
        const char* labels[] = {"1 Load", "2 Generate", "3 Run", "4 Animate",
                                "5 Reset"};
        const int ids[] = {kIdLoadFile, kIdGenerate, kIdRun, kIdAnimate, kIdReset};
        const int bw = 92, bh = 30, gap = 6, x0 = 12, y0 = 10;
        for (int i = 0; i < 5; ++i) {
            CreateWindowExA(0, "BUTTON", labels[i],
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            x0 + i * (bw + gap), y0, bw, bh, hwnd,
                            (HMENU)(UINT_PTR)ids[i], GetModuleHandleW(nullptr),
                            nullptr);
        }
    }

}  // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    const wchar_t title[] = L"Dijkstra Maze - Visualization (Person D)";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"PathGoGuiWnd";
    wc.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, title,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                860, 680, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        return 0;
    }
    buildButtons(hwnd);

    app.maze.generateRandom(21, 21, 12345);  // a starting maze
    refresh(hwnd, -1);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
