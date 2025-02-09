
#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


bool IsInvalidFormula(const std::string& text) {
    return text == "="; 
}

bool IsFormula(const std::string& text) {
    return !text.empty() && text[0] == '=';
}


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

    if (!IsFormula(text) || IsInvalidFormula(text))
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
    if (!dynamic_cast<FormulaImpl*>(impl_.get())) {
        return {};
    }
    return static_cast<FormulaImpl*>(impl_.get())->GetReferencedCells();
}


bool IsDirectCycle(const Cell* ref_cell_ptr, const Position& referenced_cell_pos, const Cell* start_cell_ptr, const Position& end_pos) 
{
    return referenced_cell_pos == end_pos || ref_cell_ptr == start_cell_ptr;
}

bool Cell::IsCyclicDependent(const Cell* start_cell_ptr, const Position& end_pos) const {
    for (const auto& referenced_cell_pos : GetReferencedCells()) 
    {
        const Cell* ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));

        if (!ref_cell_ptr) 
        {
            sheet_.SetCell(referenced_cell_pos, "");
            ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));
            if (!ref_cell_ptr) {
                continue;
            }
        }

        if (IsDirectCycle(ref_cell_ptr, referenced_cell_pos, start_cell_ptr, end_pos) ||
            ref_cell_ptr->IsCyclicDependent(start_cell_ptr, end_pos)) {
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

CellType Cell::EmptyImpl::GetType() const
{
    return CellType::EMPTY;
}

CellInterface::Value Cell::EmptyImpl::GetValue() const
{
    return "";
}

std::string Cell::EmptyImpl::GetText() const
{
    return "";
}

CellType Cell::TextImpl::GetType() const
{
    return CellType::TEXT;
}

CellInterface::Value Cell::TextImpl::GetValue() const
{
    if (text_[0] == '\'')
        return text_.substr(1);
    return text_;
}

std::string Cell::TextImpl::GetText() const
{
    return text_;
}

CellType Cell::FormulaImpl::GetType() const
{
    return CellType::FORMULA;
}

CellInterface::Value Cell::FormulaImpl::GetValue() const
{
    FormulaInterface::Value eval_result = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(eval_result))
    {
        double result = std::get<double>(eval_result);
        if (std::isinf(result))
        {
            return FormulaError(FormulaError::Category::Arithmetic);
        }
        return result;
    }
    return std::get<FormulaError>(eval_result);
}

std::string Cell::FormulaImpl::GetText() const
{
    return "=" + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
    return formula_.get()->GetReferencedCells();
}

void Cell::FormulaImpl::InvalidateCache()
{
    cached_value_.reset();
}

bool Cell::FormulaImpl::IsCacheValid() const
{
    return cached_value_.has_value();
}
