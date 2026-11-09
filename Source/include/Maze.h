#pragma once
#include <iosfwd>
#include <string>
#include <vector>

// Toa do mot o trong me cung
struct Position {
    int row = 0;
    int col = 0;
    bool operator==(const Position& o) const { return row == o.row && col == o.col; }
    bool operator!=(const Position& o) const { return !(*this == o); }
};

// Thong ke me cung
struct MazeStats {
    int rows = 0, cols = 0;
    int totalCells = 0;
    int walls = 0;      // so o tuong
    int walkable = 0;   // so o di duoc (gom S, E, o co trong so)
    int weighted = 0;   // so o co trong so > 1
    double wallRatio = 0.0;
};

// Quy uoc du lieu:
//   '0' = duong di (chi phi 1)      '1' = tuong
//   '2'..'9' = duong di co trong so (chi phi = chu so, vd bun/nuoc/cat)
//   'S' = bat dau                   'E' = ket thuc
class Maze {
public:
    static constexpr char PATH  = '0';
    static constexpr char WALL  = '1';
    static constexpr char START = 'S';
    static constexpr char END   = 'E';
    static constexpr double SQRT2 = 1.4142135623730951;

    Maze() = default;

    // ---- Nap / sinh / luu me cung ----
    bool loadFromFile(const std::string& filename, std::string* err = nullptr);
    bool loadFromLines(const std::vector<std::string>& lines, std::string* err = nullptr);
    bool saveToFile(const std::string& filename) const;
    // Me cung hoan hao (recursive backtracker); kich thuoc duoc lam tron len so le, toi thieu 5.
    // extraOpenings: so buc tuong dap them de co nhieu duong (<0: tu dong).
    void generateRandom(int rows, int cols, unsigned seed, int extraOpenings = -1);
    // Me cung ngau nhien theo ti le tuong (0..0.6); tu thu lai cho den khi co loi thoat.
    void generateByWallRatio(int rows, int cols, double wallRatio, unsigned seed);
    // Rai ngau nhien cac o co trong so (2..maxCost) len cac o duong di hien co.
    void addWeights(double ratio, int maxCost, unsigned seed);

    // ---- Kiem tra hop le ----
    // Hop le: khong rong, hinh chu nhat, chi chua 0-9/S/E, dung 1 S va 1 E.
    bool validate(std::string* err = nullptr) const;
    // Co duong di tu S den E hay khong (BFS)
    bool isSolvable(bool diagonal = false) const;

    // ---- Truy cap du lieu cho cac module khac ----
    bool empty() const { return grid_.empty(); }
    int rows() const { return static_cast<int>(grid_.size()); }
    int cols() const { return grid_.empty() ? 0 : static_cast<int>(grid_[0].size()); }
    char at(int r, int c) const { return grid_[r][c]; }
    // Tra ve toan bo luoi (chi doc, khong cho sua truc tiep)
    const std::vector<std::string>& getGrid() const { return grid_; }
    Position start() const { return start_; }
    Position end() const { return end_; }

    bool inBounds(int r, int c) const { return r >= 0 && r < rows() && c >= 0 && c < cols(); }
    bool isWall(int r, int c) const { return !inBounds(r, c) || grid_[r][c] == WALL; }
    // O di duoc: trong me cung va khong phai tuong
    bool isWalkable(int r, int c) const { return inBounds(r, c) && grid_[r][c] != WALL; }
    // Chi phi di VAO o (r,c): '0','S','E' = 1; '2'..'9' = chu so; tuong/ngoai bien = -1
    int cost(int r, int c) const;
    // Chi phi mot buoc tu 'from' sang 'to' (ke nhau). Duong cheo nhan them SQRT2.
    double moveCost(const Position& from, const Position& to) const;
    // Cac o ke di duoc. diagonal=false: 4 huong; true: 8 huong (khong cat goc tuong).
    std::vector<Position> neighbors(const Position& p, bool diagonal = false) const;

    // ---- Chinh sua (cho giao dien click chuot) ----
    // ch thuoc '0'..'9'. Khong dung de dat/xoa S, E (dung setStart/setEnd).
    bool setCell(int r, int c, char ch, std::string* err = nullptr);
    bool setStart(int r, int c, std::string* err = nullptr);
    bool setEnd(int r, int c, std::string* err = nullptr);

    // ---- Thong ke & hien thi ----
    MazeStats stats() const;
    void printStats(std::ostream& os) const;
    void print(std::ostream& os) const;
    // In me cung kem duong di (cac o trung gian duoc danh dau bang mark)
    std::string toStringWithPath(const std::vector<Position>& path, char mark = '*') const;

private:
    std::vector<std::string> grid_;
    Position start_{}, end_{};
    static bool isWeightChar(char ch) { return ch >= '2' && ch <= '9'; }
};
