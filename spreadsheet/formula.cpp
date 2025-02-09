#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>
#include <cmath>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe)
{
    return output << fe.ToString();
}

double GetCellValueAsDouble(const CellInterface* cell)
{
    auto value = cell->GetValue();

    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    if (std::holds_alternative<FormulaError>(value)) {
        throw std::get<FormulaError>(value);
    }
    if (std::holds_alternative<std::string>(value)) {
        const std::string& str = std::get<std::string>(value);
        try {
            size_t pos;
            double result = std::stod(str, &pos);
            if (pos != str.size()) {
                throw FormulaError(FormulaError::Category::Value);
            }
            if (std::isinf(result) || std::isnan(result)) {
                throw FormulaError(FormulaError::Category::Arithmetic);
            }
            return result;
        }
        catch (const std::invalid_argument&) {
            throw FormulaError(FormulaError::Category::Value);
        }
        catch (const std::out_of_range&) {
            throw FormulaError(FormulaError::Category::Value);
        }
    }
    return 0.0;
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
            : ast_(ParseFormulaAST(std::move(expression))),
            referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end()){}

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute([this, &sheet](const Position& pos) 
                    {  
                    const CellInterface* cell = sheet.GetCell(pos);

                    if (!cell || (std::holds_alternative<std::string>(cell->GetValue()) &&
                        std::get<std::string>(cell->GetValue()).empty())) {
                        return 0.0;
                    }

                    return GetCellValueAsDouble(cell);
                    });
            }
            catch (const FormulaError& ex_fe) {
                return ex_fe;
            }
        }
        std::string GetExpression() const override
        {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }

        std::vector<Position> GetReferencedCells() const override
        {
            std::vector<Position> result;
            std::set<Position> unique_ref_cells(referenced_cells_.begin(), referenced_cells_.end());
            for (const auto& cell : unique_ref_cells)
            {
                result.push_back(cell);
            }
            return result;
        }
    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try
    {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (const std::exception&)
    {
        throw FormulaException("Formula parse error");
    }
}
