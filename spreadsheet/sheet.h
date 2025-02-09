#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <set>

template<>
struct std::hash<Position> {
    std::size_t operator()(const Position& pos) const noexcept {
        std::size_t h1 = std::hash<int>{}(pos.row);
        std::size_t h2 = std::hash<int>{}(pos.col);
        return h1 ^ (h2 << 1);
    }
};

class Sheet : public SheetInterface
{
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    
    std::map<Position, std::set<Position>> cells_dependencies_;

    std::unordered_map<Position, std::unique_ptr<Cell>, std::hash<Position>> sheet_;

    //enum class PrintType;
    void Print(std::ostream& output,  std::function<void(std::ostream&, const CellInterface*)> print_func) const;
    CellInterface* GetCellImpl(Position pos);

    void UpdatePrintableSize();
    bool CellExists(Position pos) const;
    void InvalidateCell(const Position& pos);
    void AddDependentCell(const Position& main_cell, const Position& dependent_cell);
    const std::set<Position> GetDependentCells(const Position& pos);
    void DeleteDependencies(const Position& pos);


    int max_row_ = 0;    
    int max_col_ = 0;    
    bool area_is_valid_ = true; 
};
