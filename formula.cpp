#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        : ast_(ParseFormulaAST(std::move(expression)))
    {
        auto cells = ast_.GetCells();
        cells.unique();
        cells.sort();
        cells_ = {cells.begin(), cells.end()};
    }
    
    Value Evaluate(const SheetInterface& arg) const override {
        try {
            return ast_.Execute(arg);
        }
        catch(const FormulaError& err) {
            return err;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream oss;
        ast_.PrintFormula(oss);
        return oss.str();
    }
    
    std::vector<Position> GetReferencedCells() const override {
        return cells_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> cells_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
       return std::make_unique<Formula>(std::move(expression));
}