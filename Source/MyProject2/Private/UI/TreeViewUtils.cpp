// TreeViewUtils.cpp
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 구현

#include "UI/TreeViewUtils.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체 정의를 위해 필요
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

// CSV 파일 읽기 함수
bool FTreeViewUtils::ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows)
{
    // 파일을 줄 단위로 읽기
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("파일 로드 실패: %s"), *FilePath);
        return false;
    }
    
    for (const FString& Line : Lines)
    {
        if (Line.IsEmpty())
            continue;
            
        TArray<FString> Cells;
        
        // CSV 파싱 (쉼표로 구분된 셀)
        bool bInQuotes = false;
        FString CurrentCell;
        
        for (int32 i = 0; i < Line.Len(); i++)
        {
            TCHAR CurrentChar = Line[i];
            
            if (CurrentChar == TEXT('"'))
            {
                bInQuotes = !bInQuotes;
            }
            else if (CurrentChar == TEXT(',') && !bInQuotes)
            {
                Cells.Add(CurrentCell);
                CurrentCell.Empty();
            }
            else
            {
                CurrentCell.AppendChar(CurrentChar);
            }
        }
        
        // 마지막 셀 추가
        Cells.Add(CurrentCell);
        
        OutRows.Add(Cells);
    }
    
    return OutRows.Num() > 0;
}

// 항목이 검색어와 일치하는지 확인하는 함수
bool FTreeViewUtils::DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText)
{
    if (!Item.IsValid())
        return false;
        
    // 대소문자 구분 없이 검색하기 위해 모든 문자열을 소문자로 변환
    FString LowerPartNo = Item->PartNo.ToLower();
    FString LowerType = Item->Type.ToLower();
    FString LowerNomenclature = Item->Nomenclature.ToLower();
    
    // 파트 번호, 유형, 명칭에서 검색
    return LowerPartNo.Contains(InSearchText) || 
           LowerType.Contains(InSearchText) || 
           LowerNomenclature.Contains(InSearchText);
}

// 한 항목이 다른 항목의 자식인지 확인하는 함수
bool FTreeViewUtils::IsChildOf(const TSharedPtr<FPartTreeItem>& PotentialChild, const TSharedPtr<FPartTreeItem>& PotentialParent)
{
    if (!PotentialChild.IsValid() || !PotentialParent.IsValid())
    {
        return false;
    }
    
    // 직접적인 자식인지 확인
    for (const auto& DirectChild : PotentialParent->Children)
    {
        if (DirectChild == PotentialChild)
        {
            return true;
        }
        
        // 재귀적으로 하위 자식들 확인
        if (IsChildOf(PotentialChild, DirectChild))
        {
            return true;
        }
    }
    
    return false;
}

// 항목의 부모 항목 찾기 함수
TSharedPtr<FPartTreeItem> FTreeViewUtils::FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem, const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap)
{
    if (!ChildItem.IsValid() || ChildItem->NextPart.IsEmpty() || ChildItem->NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
    {
        return nullptr;
    }
    
    // NextPart를 기반으로 부모 항목 찾기
    const TSharedPtr<FPartTreeItem>* ParentPtr = PartNoToItemMap.Find(ChildItem->NextPart);
    return ParentPtr ? *ParentPtr : nullptr;
}

FText FTreeViewUtils::GetFormattedMetadata(const TSharedPtr<FPartTreeItem>& Item)
{
    if (!Item.IsValid())
    {
        return FText::FromString("No item selected");
    }
    
    // 선택된 항목의 메타데이터 구성
    FString MetadataText = FString::Printf(
        TEXT("S/N: %s\n")
        TEXT("Level: %d\n")
        TEXT("Type: %s\n")
        TEXT("Part No: %s\n")
        TEXT("Part Rev: %s\n")
        TEXT("Part Status: %s\n")
        TEXT("Latest: %s\n")
        TEXT("Nomenclature: %s\n")
        TEXT("Instance ID 총수량(ALL DB): %s\n")
        TEXT("Qty: %s\n")
        TEXT("NextPart: %s"),
        *GetSafeString(Item->SN),
        Item->Level,
        *GetSafeString(Item->Type),
        *GetSafeString(Item->PartNo),
        *GetSafeString(Item->PartRev),
        *GetSafeString(Item->PartStatus),
        *GetSafeString(Item->Latest),
        *GetSafeString(Item->Nomenclature),
        *GetSafeString(Item->InstanceIDTotalAllDB),
        *GetSafeString(Item->Qty),
        *GetSafeString(Item->NextPart)
    );
    
    return FText::FromString(MetadataText);
}

FString FTreeViewUtils::GetSafeString(const FString& InStr)
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
}   