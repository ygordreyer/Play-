#pragma once

#include "Types.h"
#include <vector>
#include <functional>

class CMemoryValidation
{
public:
    enum VALIDATION_TYPE
    {
        EQUAL,              // ==
        NOT_EQUAL,          // !=
        LESS_THAN,         // <
        GREATER_THAN,      // >
        LESS_EQUAL,        // <=
        GREATER_EQUAL,     // >=
        BIT_ALL_SET,       // (value & mask) == mask
        BIT_ANY_SET,       // (value & mask) != 0
        BIT_NONE_SET,      // (value & mask) == 0
        RANGE_INSIDE,      // value >= min && value <= max
        RANGE_OUTSIDE      // value < min || value > max
    };

    struct VALIDATION_CONDITION
    {
        uint32 address;
        uint32 size;
        uint32 value;      // For single value comparisons
        uint32 mask;       // For bit operations
        uint32 minValue;   // For range checks
        uint32 maxValue;   // For range checks
        VALIDATION_TYPE type;
        bool active;
    };

    using ValidationCallback = std::function<void(const VALIDATION_CONDITION&, bool)>;

    CMemoryValidation();
    virtual ~CMemoryValidation() = default;

    // Condition management
    void AddCondition(const VALIDATION_CONDITION& condition);
    void RemoveCondition(uint32 address);
    void ClearConditions();
    
    // Validation
    bool ValidateAddress(uint32 address, uint32 value);
    bool ValidateRange(uint32 startAddress, uint32 endAddress);
    bool ValidateAll();
    
    // Callbacks
    void AddValidationCallback(ValidationCallback callback);
    void RemoveAllCallbacks();

private:
    std::vector<VALIDATION_CONDITION> m_conditions;
    std::vector<ValidationCallback> m_callbacks;

    bool ValidateCondition(const VALIDATION_CONDITION& condition, uint32 value);
    void NotifyCallbacks(const VALIDATION_CONDITION& condition, bool result);
};