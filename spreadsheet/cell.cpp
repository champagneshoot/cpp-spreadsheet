#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface& sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() = default;


void Cell::Set(const std::string& text)
{
    using namespace std::literals;
  
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }

    if (text[0] != '=' || (text[0] == '=' && text.size() == 1))
    {
        impl_ = std::make_unique<TextImpl>(text);
        return;
    }

    try
    {
        impl_ = std::make_unique<FormulaImpl>(sheet_, std::string{ text.begin() + 1, text.end() });
    }
    catch (...)
    {
        std::string fe_msg = "Formula parsing error"s;
        throw FormulaException(fe_msg);
    }
}

void Cell::Clear()
{
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }
std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const
{
    if (auto* formula_impl = dynamic_cast<FormulaImpl*>(impl_.get()))
    {
        return formula_impl->GetReferencedCells();
    }
    return {};
}

bool Cell::IsCyclicDependent(const Cell* start_cell_ptr, const Position& end_pos) const
{
    for (const auto& referenced_cell_pos : GetReferencedCells())
    {
        if (referenced_cell_pos == end_pos){
            return true;
        }
        const Cell* ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));
        if (!ref_cell_ptr)
        {
            sheet_.SetCell(referenced_cell_pos, "");
            ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));
        }
        if (start_cell_ptr == ref_cell_ptr){
            return true;
        }
        if (ref_cell_ptr->IsCyclicDependent(start_cell_ptr, end_pos)){
            return true;
        }
    }
    return false;
}

void Cell::InvalidateCache()
{
    if (auto* formula_impl = dynamic_cast<FormulaImpl*>(impl_.get()))
    {
        return formula_impl->InvalidateCache();
    }
}

bool Cell::IsCacheValid() const
{
    if (auto* formula_impl = dynamic_cast<FormulaImpl*>(impl_.get()))
    {
        return formula_impl->IsCacheValid();
    }
    return false;
}
