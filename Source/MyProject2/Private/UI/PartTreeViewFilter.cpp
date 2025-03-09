// Source/MyProject2/Private/UI/PartTreeViewFilter.cpp
#include "UI/PartTreeViewFilter.h"
#include "ServiceLocator.h"
#include "UI/PartImageManager.h"

//=================================================================
// FImageFilter 구현
//=================================================================
bool FImageFilter::PassesFilter(const TSharedPtr<FPartTreeItem>& Item) const
{
    if (!bEnabled || !Item.IsValid())
        return true; // 필터가 비활성화되었거나 항목이 유효하지 않으면 항상 통과

    // 이미지가 있는 경우 또는 자식 중에 이미지가 있는 경우 통과
    return FServiceLocator::GetImageManager()->HasImage(Item->PartNo) || 
           FServiceLocator::GetImageManager()->HasChildWithImage(Item);
}

//=================================================================
// FDuplicateFilter 구현
//=================================================================
bool FDuplicateFilter::PassesFilter(const TSharedPtr<FPartTreeItem>& Item) const
{
    if (!bEnabled || !Item.IsValid())
        return true; // 필터가 비활성화되었거나 항목이 유효하지 않으면 항상 통과

    FString NormalizedName = Item->PartNo.TrimStartAndEnd();
    
    // 이미 처리한 파트 번호인지 확인
    if (ProcessedPartNumbers.Contains(NormalizedName))
        return false; // 중복된 파트 번호는 필터링
    
    // 처리한 파트 번호 목록에 추가
    ProcessedPartNumbers.Add(NormalizedName);
    return true;
}

//=================================================================
// FPartTreeViewFilterManager 구현
//=================================================================
FPartTreeViewFilterManager::FPartTreeViewFilterManager()
{
    // 기본 필터 추가
    AddFilter(MakeShared<FImageFilter>());
    AddFilter(MakeShared<FDuplicateFilter>());
}

FPartTreeViewFilterManager::~FPartTreeViewFilterManager()
{
    Filters.Empty();
}

bool FPartTreeViewFilterManager::PassesAllFilters(const TSharedPtr<FPartTreeItem>& Item) const
{
    // 모든 활성화된 필터를 통과해야 함
    for (const auto& Filter : Filters)
    {
        if (Filter->IsEnabled() && !Filter->PassesFilter(Item))
            return false;
    }
    return true;
}

void FPartTreeViewFilterManager::SetFilterEnabled(const FString& FilterName, bool bEnabled)
{
    for (auto& Filter : Filters)
    {
        if (Filter->GetFilterName() == FilterName)
        {
            // 필터 상태가 변경되면 필터 초기화
            if (Filter->IsEnabled() != bEnabled)
            {
                Filter->Reset();
                Filter->SetEnabled(bEnabled);
            }
            break;
        }
    }
}

bool FPartTreeViewFilterManager::IsFilterEnabled(const FString& FilterName) const
{
    for (const auto& Filter : Filters)
    {
        if (Filter->GetFilterName() == FilterName)
            return Filter->IsEnabled();
    }
    return false;
}

void FPartTreeViewFilterManager::AddFilter(TSharedPtr<IPartTreeViewFilter> Filter)
{
    if (Filter.IsValid())
    {
        Filters.Add(Filter);
    }
}

void FPartTreeViewFilterManager::ResetFilters()
{
    for (auto& Filter : Filters)
    {
        Filter->Reset();
    }
}