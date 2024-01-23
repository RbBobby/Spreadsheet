#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

using namespace std::literals;

class Cell::Impl {
public:
    virtual ~Impl() = default;
    
    virtual Cell::Value GetValue(SheetInterface& sheet) const = 0;

    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }
};

class Cell::EmptyImpl: public Impl {
public:
    EmptyImpl() = default;
    
    Cell::Value GetValue(SheetInterface& sheet) const override {
        return null;
    }
    
    std::string GetText() const override{
        return null;
    }

private:
    std::string null = "";
};

class Cell::TextImpl: public Impl {
public:
    explicit TextImpl(std::string& text) 
        : text_(std::move(text)) {}
        
    Cell::Value GetValue(SheetInterface& sheet) const override {
        return text_;
    }
    
    std::string GetText() const override {
        return text_;
    }
private:
    const std::string text_;
};

class Cell::FormulaImpl: public Impl {
public:
    explicit FormulaImpl(std::string formula) 
        : formula_(std::move(ParseFormula(std::move(formula))))
    {
    }    
    Cell::Value GetValue(SheetInterface& sheet) const override {
        Cell::Value value;
        FormulaInterface::Value formula = formula_.get()->Evaluate(sheet);
        if (std::holds_alternative<double>(formula)) {
            value = std::get<double>(formula); // обработка значения типа double
        } else{
            value = std::get<FormulaError>(formula);
        }
        return value;
    }
    
    std::string GetText() const override {
        return '=' + formula_->GetExpression();
    }
    
    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> formula_;
};

// Реализуйте следующие методы

Cell::Cell(Sheet& sheet) 
    : impl_(std::make_unique<EmptyImpl>(EmptyImpl{})) 
    , sheet_(sheet){}

Cell::~Cell() = default;

bool Cell::IsModified() const {
    return !cache_.exists_;
}

void Cell::SetCache(Value&& val) const {
    cache_.value_ = std::forward<Value>(val);
    cache_.exists_ = false;
}

void Cell::Set(std::string text) {
    if(text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if(text.front() == '=' && text.size() > 1u) {
        try {
            impl_ = std::make_unique<FormulaImpl>(FormulaImpl{text.substr(1u)});
            for(const auto& ref_cell : impl_->GetReferencedCells()) {
                auto cell_ptr = sheet_.GetCell(ref_cell);
                if(!cell_ptr) {
                    sheet_.SetCell(ref_cell, ""s);
                }
            }
        }
        catch(...) {
            throw FormulaException{"FormulaException"};
        }
    }
    else {
        impl_ = std::make_unique<TextImpl>(TextImpl{text});
    }
    cache_.exists_ = true;
}

void Cell::Clear() {
    impl_.reset(nullptr);
    cache_.exists_ = true;
}

struct CellValueVisitor {
    Cell::Value operator() (std::string str) const {
        return str.front() == '\'' ? str.substr(1u) : str;
    }
    Cell::Value operator() (double d) const {
        return d;
    }
    Cell::Value operator() (FormulaError fe) const {
        return fe;
    }
};

Cell::Value Cell::GetValue() const {
    const auto& dependency = GetReferencedCells();
    if(cache_.exists_ || 
        std::any_of(dependency.begin(),
                    dependency.end(), 
                    [this] (const auto& pos) {
                        const Cell* cell_ptr_ = reinterpret_cast<const Cell*>(sheet_.GetCell(pos));
                        return cell_ptr_->IsModified();})) {
        
        auto value = impl_ ? std::visit(CellValueVisitor(), impl_->GetValue(sheet_)) : Cell::Value{};
        SetCache(std::move(value));
    }
    return cache_.value_;
}

std::string Cell::GetText() const {
    if(impl_) {
        return impl_->GetText();
    }
    return "";
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

std::string_view FormulaError::ToString() const {
        switch(category_) {
        case FormulaError::Category::Ref: {
            return "#REF!";
        }
        case FormulaError::Category::Value: {
            return "#VALUE!";
        }
        case FormulaError::Category::Div0: {
            return "#ARITHM!";
        }
        default:
            throw std::runtime_error{""s};
    }
}
