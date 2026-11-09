#include "Maze.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
using namespace std;
namespace {
const int DR[8] = {-1, 0, 1, 0, -1, -1, 1, 1};
const int DC[8] = {0, 1, 0, -1, -1, 1, 1, -1};
void setErr(string* err, const string& msg) {
    if (err) *err = msg;
}
string stripSpaces(const string& raw) {
    string s;
    for (char ch : raw)
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') s += ch;
    return s;
}
}
bool Maze::loadFromFile(const string& filename, string* err) {
    ifstream in(filename);
    if (!in) {
        setErr(err, "Khong mo duoc file: " + filename);
        return false;
    }
    vector<string> lines;
    string line;
    while (getline(in, line)) lines.push_back(line);
    return loadFromLines(lines, err);
}
bool Maze::loadFromLines(const vector<string>& lines, string* err) {
    vector<string> g;
    int expectRows = -1, expectCols = -1;
    bool first = true;
    for (const string& raw : lines) {
        string s = stripSpaces(raw);
        if (s.empty()) continue;
        if (first) {
            first = false;
            istringstream iss(raw);
            int a, b;
            string extra;
            if ((iss >> a >> b) && !(iss >> extra)) {
                expectRows = a;
                expectCols = b;
                continue;
            }
        }
        g.push_back(s);
    }
    Maze tmp;
    tmp.grid_ = g;
    if (expectRows >= 0 && (static_cast<int>(g.size()) != expectRows ||
                            (!g.empty() && static_cast<int>(g[0].size()) != expectCols))) {
        setErr(err, "Kich thuoc khai bao khong khop voi du lieu thuc te");
        return false;
    }
    if (!tmp.validate(err)) return false;
    for (int r = 0; r < tmp.rows(); ++r)
        for (int c = 0; c < tmp.cols(); ++c) {
            if (g[r][c] == START) tmp.start_ = {r, c};
            if (g[r][c] == END) tmp.end_ = {r, c};
        }
    *this = tmp;
    return true;
}
bool Maze::saveToFile(const string& filename) const {
    ofstream out(filename);
    if (!out) return false;
    out << rows() << " " << cols() << "\n";
    for (const auto& row : grid_) out << row << "\n";
    return true;
}
void Maze::generateRandom(int rows, int cols, unsigned seed, int extraOpenings) {
    rows = max(5, rows);
    cols = max(5, cols);
    if (rows % 2 == 0) ++rows;
    if (cols % 2 == 0) ++cols;
    mt19937 rng(seed);
    grid_.assign(rows, string(cols, WALL));
    vector<Position> st;
    grid_[1][1] = PATH;
    st.push_back({1, 1});
    while (!st.empty()) {
        Position cur = st.back();
        vector<int> dirs = {0, 1, 2, 3};
        shuffle(dirs.begin(), dirs.end(), rng);
        bool moved = false;
        for (int d : dirs) {
            int nr = cur.row + 2 * DR[d], nc = cur.col + 2 * DC[d];
            if (nr > 0 && nr < rows - 1 && nc > 0 && nc < cols - 1 && grid_[nr][nc] == WALL) {
                grid_[cur.row + DR[d]][cur.col + DC[d]] = PATH;
                grid_[nr][nc] = PATH;
                st.push_back({nr, nc});
                moved = true;
                break;
            }
        }
        if (!moved) st.pop_back();
    }
    if (extraOpenings < 0) extraOpenings = rows * cols / 20;
    uniform_int_distribution<int> rr(1, rows - 2), rc(1, cols - 2);
    for (int i = 0, tries = 0; i < extraOpenings && tries < extraOpenings * 20; ++tries) {
        int r = rr(rng), c = rc(rng);
        if (grid_[r][c] == WALL) {
            grid_[r][c] = PATH;
            ++i;
        }
    }
    start_ = {1, 1};
    end_ = {rows - 2, cols - 2};
    grid_[start_.row][start_.col] = START;
    grid_[end_.row][end_.col] = END;
}
void Maze::generateByWallRatio(int rows, int cols, double wallRatio, unsigned seed) {
    rows = max(5, rows);
    cols = max(5, cols);
    wallRatio = min(0.6, max(0.0, wallRatio));
    for (unsigned attempt = 0; attempt < 200; ++attempt) {
        mt19937 rng(seed + attempt);
        uniform_real_distribution<double> u(0.0, 1.0);
        grid_.assign(rows, string(cols, PATH));
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                if (u(rng) < wallRatio) grid_[r][c] = WALL;
        start_ = {1, 1};
        end_ = {rows - 2, cols - 2};
        grid_[start_.row][start_.col] = START;
        grid_[end_.row][end_.col] = END;
        if (isSolvable()) return;
    }
    for (auto& row : grid_) fill(row.begin(), row.end(), PATH);
    grid_[start_.row][start_.col] = START;
    grid_[end_.row][end_.col] = END;
}
void Maze::addWeights(double ratio, int maxCost, unsigned seed) {
    maxCost = min(9, max(2, maxCost));
    ratio = min(1.0, max(0.0, ratio));
    mt19937 rng(seed);
    uniform_real_distribution<double> u(0.0, 1.0);
    uniform_int_distribution<int> w(2, maxCost);
    for (auto& row : grid_)
        for (char& ch : row)
            if ((ch == PATH || isWeightChar(ch)) && u(rng) < ratio)
                ch = static_cast<char>('0' + w(rng));
}
bool Maze::validate(string* err) const {
    if (grid_.empty()) {
        setErr(err, "Me cung rong");
        return false;
    }
    const size_t w = grid_[0].size();
    if (w == 0) {
        setErr(err, "Hang dau tien rong");
        return false;
    }
    int countS = 0, countE = 0;
    for (size_t r = 0; r < grid_.size(); ++r) {
        if (grid_[r].size() != w) {
            setErr(err, "Hang " + to_string(r) + " co do dai khac cac hang con lai");
            return false;
        }
        for (size_t c = 0; c < w; ++c) {
            char ch = grid_[r][c];
            if (ch == START) ++countS;
            else if (ch == END) ++countE;
            else if (ch != PATH && ch != WALL && !isWeightChar(ch)) {
                setErr(err, string("Ky tu khong hop le '") + ch + "' tai (" + to_string(r) +
                                "," + to_string(c) + ")");
                return false;
            }
        }
    }
    if (countS != 1) {
        setErr(err, "Phai co dung 1 diem Start 'S' (hien co " + to_string(countS) + ")");
        return false;
    }
    if (countE != 1) {
        setErr(err, "Phai co dung 1 diem End 'E' (hien co " + to_string(countE) + ")");
        return false;
    }
    return true;
}
bool Maze::isSolvable(bool diagonal) const {
    if (grid_.empty()) return false;
    vector<vector<bool>> seen(rows(), vector<bool>(cols(), false));
    queue<Position> q;
    q.push(start_);
    seen[start_.row][start_.col] = true;
    while (!q.empty()) {
        Position p = q.front();
        q.pop();
        if (p == end_) return true;
        for (const Position& n : neighbors(p, diagonal))
            if (!seen[n.row][n.col]) {
                seen[n.row][n.col] = true;
                q.push(n);
            }
    }
    return false;
}
int Maze::cost(int r, int c) const {
    if (!isWalkable(r, c)) return -1;
    char ch = grid_[r][c];
    return isWeightChar(ch) ? ch - '0' : 1;
}
double Maze::moveCost(const Position& from, const Position& to) const {
    int c = cost(to.row, to.col);
    if (c < 0) return -1.0;
    bool diagonal = (from.row != to.row) && (from.col != to.col);
    return diagonal ? c * SQRT2 : static_cast<double>(c);
}
vector<Position> Maze::neighbors(const Position& p, bool diagonal) const {
    vector<Position> res;
    const int n = diagonal ? 8 : 4;
    for (int d = 0; d < n; ++d) {
        int nr = p.row + DR[d], nc = p.col + DC[d];
        if (!isWalkable(nr, nc)) continue;
        if (d >= 4) {
            if (!isWalkable(p.row + DR[d], p.col) || !isWalkable(p.row, p.col + DC[d])) continue;
        }
        res.push_back({nr, nc});
    }
    return res;
}
bool Maze::setCell(int r, int c, char ch, string* err) {
    if (!inBounds(r, c)) {
        setErr(err, "Toa do ngoai me cung");
        return false;
    }
    if (ch != PATH && ch != WALL && !isWeightChar(ch)) {
        setErr(err, "Chi duoc dat ky tu 0-9 (dung setStart/setEnd cho S, E)");
        return false;
    }
    if (grid_[r][c] == START || grid_[r][c] == END) {
        setErr(err, "Khong the ghi de o Start/End; hay di chuyen S/E truoc");
        return false;
    }
    grid_[r][c] = ch;
    return true;
}
bool Maze::setStart(int r, int c, string* err) {
    if (!inBounds(r, c) || grid_[r][c] == WALL) {
        setErr(err, "Start phai nam tren o di duoc trong me cung");
        return false;
    }
    if (Position{r, c} == end_) {
        setErr(err, "Start khong duoc trung End");
        return false;
    }
    grid_[start_.row][start_.col] = PATH;
    start_ = {r, c};
    grid_[r][c] = START;
    return true;
}
bool Maze::setEnd(int r, int c, string* err) {
    if (!inBounds(r, c) || grid_[r][c] == WALL) {
        setErr(err, "End phai nam tren o di duoc trong me cung");
        return false;
    }
    if (Position{r, c} == start_) {
        setErr(err, "End khong duoc trung Start");
        return false;
    }
    grid_[end_.row][end_.col] = PATH;
    end_ = {r, c};
    grid_[r][c] = END;
    return true;
}
MazeStats Maze::stats() const {
    MazeStats s;
    s.rows = rows();
    s.cols = cols();
    s.totalCells = s.rows * s.cols;
    for (const auto& row : grid_)
        for (char ch : row) {
            if (ch == WALL) {
                ++s.walls;
            } else {
                ++s.walkable;
                if (isWeightChar(ch)) ++s.weighted;
            }
        }
    s.wallRatio = s.totalCells ? static_cast<double>(s.walls) / s.totalCells : 0.0;
    return s;
}
void Maze::printStats(ostream& os) const {
    MazeStats s = stats();
    os << "Kich thuoc : " << s.rows << " x " << s.cols << " (" << s.totalCells << " o)\n"
       << "O tuong    : " << s.walls << " (" << fixed << setprecision(1) << s.wallRatio * 100
       << "%)\n"
       << "O di duoc  : " << s.walkable << " (trong do " << s.weighted << " o co trong so)\n"
       << "Start      : (" << start_.row << "," << start_.col << ")\n"
       << "End        : (" << end_.row << "," << end_.col << ")\n";
}
void Maze::print(ostream& os) const {
    for (const auto& row : grid_) os << row << "\n";
}
string Maze::toStringWithPath(const vector<Position>& path, char mark) const {
    vector<string> g = grid_;
    for (const Position& p : path) {
        if (!inBounds(p.row, p.col)) continue;
        char& ch = g[p.row][p.col];
        if (ch != START && ch != END && ch != WALL) ch = mark;
    }
    string out;
    for (const auto& row : g) out += row + "\n";
    return out;
}
