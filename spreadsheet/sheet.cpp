#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <optional>

using namespace std::literals;

enum class Sheet::PrintType {
    TEXT,
    VALUE
};

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position for SetCell()");
    }

    auto cell = GetCell(pos);
    if (cell) {
        std::string old_text = cell->GetText();
        InvalidateCell(pos);
        DeleteDependencies(pos);
        dynamic_cast<Cell*>(cell)->Clear();
        dynamic_cast<Cell*>(cell)->Set(text);
        if (dynamic_cast<Cell*>(cell)->IsCyclicDependent(dynamic_cast<Cell*>(cell), pos)) {
            dynamic_cast<Cell*>(cell)->Set(std::move(old_text));
            throw CircularDependencyException("Circular dependency detected!");
        }
        for (const auto& ref_cell : dynamic_cast<Cell*>(cell)->GetReferencedCells()) {
            AddDependentCell(ref_cell, pos);
        }
    }
    else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);
        if (new_cell->IsCyclicDependent(new_cell.get(), pos)) {
            throw CircularDependencyException("Circular dependency detected!");
        }
        for (const auto& ref_cell : new_cell->GetReferencedCells()) {
            AddDependentCell(ref_cell, pos);
        }
        sheet_[pos] = std::move(new_cell);
        UpdatePrintableSize();
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position for GetCell()");
    }
    auto it = sheet_.find(pos);
    return (it != sheet_.end()) ? it->second.get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position for GetCell()");
    }
    auto it = sheet_.find(pos);
    return (it != sheet_.end()) ? it->second.get() : nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position for ClearCell()");
    }

    if (CellExists(pos)) {
        sheet_.erase(pos);
        area_is_valid_ = false;
        UpdatePrintableSize();
    }
}

Size Sheet::GetPrintableSize() const {
    if (area_is_valid_) {
        return Size{ max_row_, max_col_ };
    }
    throw InvalidPositionException("The size of printable area has not been updated");
}

void Sheet::PrintValues(std::ostream& output) const {
    Sheet::Print(output, PrintType::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const {
    Sheet::Print(output, PrintType::TEXT);
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) 
{
    std::visit([&](const auto& val) {output << val;}, value);
    return output;
}

void Sheet::Print(std::ostream& output, PrintType type) const {
    if (sheet_.empty()) {
        output << ""s;
        return;
    }
    Size printable_area = GetPrintableSize();
    for (int row = 0; row < printable_area.rows; ++row)
    {
        for (int col = 0; col < printable_area.cols; ++col) 
        {
            const auto& cell = GetCell({ row, col });

            if (cell == nullptr) 
            {
                output << "";
            }
            else if (type == PrintType::TEXT) {
                output << cell->GetText();
            }
            else {
                output << cell->GetValue();
            }
            if (col < printable_area.cols - 1) 
                output << "\t";
        }
        output << "\n";
    }
}

void Sheet::InvalidateCell(const Position& pos) {
    for (const auto& dependent_cell : GetDependentCells(pos)) {
        auto cell = GetCell(dependent_cell);
        dynamic_cast<Cell*>(cell)->InvalidateCache();
        InvalidateCell(dependent_cell);
    }
}

void Sheet::AddDependentCell(const Position& main_cell, const Position& dependent_cell) {
    cells_dependencies_[main_cell].insert(dependent_cell);
}

const std::set<Position> Sheet::GetDependentCells(const Position& pos)
{
    auto it = cells_dependencies_.find(pos);
    return (it != cells_dependencies_.end()) ? it->second : std::set<Position>{};
}

void Sheet::DeleteDependencies(const Position& pos) {
    cells_dependencies_.erase(pos);
}

void Sheet::UpdatePrintableSize() {
    max_row_ = 0;
    max_col_ = 0;
    for (const auto& [pos, cell] : sheet_) {
        if (cell) {
            max_row_ = std::max(max_row_, pos.row + 1);
            max_col_ = std::max(max_col_, pos.col + 1);
        }
    }
    area_is_valid_ = true;
}

bool Sheet::CellExists(Position pos) const {
    return sheet_.count(pos) > 0;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
