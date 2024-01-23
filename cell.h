#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"


#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
    struct CellCache {
        Value value_;
        bool exists_ = true;
    };
    
    bool IsModified() const;
    void SetCache(Value&& val) const;

public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();
    
    Value GetValue() const override;
    std::string GetText() const override;
    
    std::vector<Position> GetReferencedCells() const override;
    
private:
class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
//можете воспользоваться нашей подсказкой, но это необязательно.

    Sheet& sheet_;
    mutable CellCache cache_;
};