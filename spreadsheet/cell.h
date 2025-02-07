#pragma once

#include "common.h"
#include "formula.h"
#include <optional>
#include <functional>     
#include <unordered_set> 
#include <cmath> 

enum class CellType
{
    EMPTY,
    TEXT,
    FORMULA,
    ERROR
};

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(const std::string& text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const;

    bool IsCyclicDependent(const Cell* start_cell_ptr, const Position& end_pos) const;
    void InvalidateCache();
    bool IsCacheValid() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;

    class Impl {
    public:
        virtual ~Impl() = default;
        virtual CellType GetType() const = 0;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
        CellType GetType() const override
        {
            return CellType::EMPTY;
        }
        CellInterface::Value GetValue() const override { return ""; }
        std::string GetText() const override { return ""; }
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text) : text_(text) {}
        CellType GetType() const override
        {
            return CellType::TEXT;
        }
        CellInterface::Value GetValue() const override
        {
            if (text_[0] == '\'')
                return text_.substr(1);
            return text_;
        }
        std::string GetText() const override { return text_; }

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl 
    {
    public:
        FormulaImpl(SheetInterface& sheet, std::string formula) : sheet_(sheet), formula_(ParseFormula(formula)) {}
        CellType GetType() const override
        {
            return CellType::FORMULA;
        }
        CellInterface::Value GetValue() const override
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

        std::string GetText() const override
        {
            return "=" + formula_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const
        {
            return formula_.get()->GetReferencedCells();
        }
        void InvalidateCache()
        {
            cached_value_.reset();
        }
        bool IsCacheValid() const
        {
            return cached_value_.has_value();
        }

    private:
        SheetInterface& sheet_;
        std::unique_ptr<FormulaInterface> formula_;
        std::optional<CellInterface::Value> cached_value_;
        
    };
};
