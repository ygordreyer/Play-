#include "MemoryValidation.h"
#include "Log.h"

#define LOG_NAME "MemoryValidation"

CMemoryValidation::CMemoryValidation()
{
}

void CMemoryValidation::AddCondition(const VALIDATION_CONDITION& condition)
{
    // Remove any existing condition for this address
    RemoveCondition(condition.address);
    
    m_conditions.push_back(condition);
    
    CLog::GetInstance().Print(LOG_NAME, "Added validation condition for address 0x%08X\r\n", 
        condition.address);
}

void CMemoryValidation::RemoveCondition(uint32 address)
{
    auto it = std::find_if(m_conditions.begin(), m_conditions.end(),
        [address](const VALIDATION_CONDITION& c) { return c.address == address; });
    
    if (it != m_conditions.end())
    {
        m_conditions.erase(it);
        CLog::GetInstance().Print(LOG_NAME, "Removed validation condition for address 0x%08X\r\n", 
            address);
    }
}

void CMemoryValidation::ClearConditions()
{
    m_conditions.clear();
    CLog::GetInstance().Print(LOG_NAME, "Cleared all validation conditions\r\n");
}

bool CMemoryValidation::ValidateAddress(uint32 address, uint32 value)
{
    bool result = true;
    
    for (const auto& condition : m_conditions)
    {
        if (condition.active && condition.address == address)
        {
            bool conditionResult = ValidateCondition(condition, value);
            result &= conditionResult;
            NotifyCallbacks(condition, conditionResult);
        }
    }
    
    return result;
}

bool CMemoryValidation::ValidateRange(uint32 startAddress, uint32 endAddress)
{
    bool result = true;
    
    for (const auto& condition : m_conditions)
    {
        if (condition.active && 
            condition.address >= startAddress && 
            condition.address <= endAddress)
        {
            // For range validation, we use the condition's value as the current value
            bool conditionResult = ValidateCondition(condition, condition.value);
            result &= conditionResult;
            NotifyCallbacks(condition, conditionResult);
        }
    }
    
    return result;
}

bool CMemoryValidation::ValidateAll()
{
    bool result = true;
    
    for (const auto& condition : m_conditions)
    {
        if (condition.active)
        {
            bool conditionResult = ValidateCondition(condition, condition.value);
            result &= conditionResult;
            NotifyCallbacks(condition, conditionResult);
        }
    }
    
    return result;
}

void CMemoryValidation::AddValidationCallback(ValidationCallback callback)
{
    m_callbacks.push_back(callback);
}

void CMemoryValidation::RemoveAllCallbacks()
{
    m_callbacks.clear();
}

bool CMemoryValidation::ValidateCondition(const VALIDATION_CONDITION& condition, uint32 value)
{
    switch (condition.type)
    {
        case EQUAL:
            return value == condition.value;
            
        case NOT_EQUAL:
            return value != condition.value;
            
        case LESS_THAN:
            return value < condition.value;
            
        case GREATER_THAN:
            return value > condition.value;
            
        case LESS_EQUAL:
            return value <= condition.value;
            
        case GREATER_EQUAL:
            return value >= condition.value;
            
        case BIT_ALL_SET:
            return (value & condition.mask) == condition.mask;
            
        case BIT_ANY_SET:
            return (value & condition.mask) != 0;
            
        case BIT_NONE_SET:
            return (value & condition.mask) == 0;
            
        case RANGE_INSIDE:
            return value >= condition.minValue && value <= condition.maxValue;
            
        case RANGE_OUTSIDE:
            return value < condition.minValue || value > condition.maxValue;
            
        default:
            CLog::GetInstance().Print(LOG_NAME, "Unknown validation type for address 0x%08X\r\n", 
                condition.address);
            return false;
    }
}

void CMemoryValidation::NotifyCallbacks(const VALIDATION_CONDITION& condition, bool result)
{
    for (const auto& callback : m_callbacks)
    {
        callback(condition, result);
    }
}