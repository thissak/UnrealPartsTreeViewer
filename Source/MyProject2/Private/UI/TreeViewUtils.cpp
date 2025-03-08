// TreeViewUtils.cpp
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 구현

#include "UI/TreeViewUtils.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "UI/LevelBasedTreeView.h"

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

// 문자열 유틸리티 함수
FString FTreeViewUtils::GetSafeString(const FString& InStr)
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
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

// 필요한 열 인덱스 찾기 함수
void FTreeViewUtils::FindColumnIndices(
    const TArray<FString>& HeaderRow,
    int32& OutPartNoColIdx,
    int32& OutNextPartColIdx,
    int32& OutLevelColIdx,
    int32& OutSNColIdx,
    int32& OutTypeColIdx,
    int32& OutPartRevColIdx,
    int32& OutPartStatusColIdx,
    int32& OutLatestColIdx,
    int32& OutNomenclatureColIdx,
    int32& OutInstanceIDTotalColIdx,
    int32& OutQtyColIdx)
{
    // 파트 번호 열 인덱스 찾기
    OutPartNoColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("PartNo") || HeaderName == TEXT("Part No");
    });
    
    // 상위 파트 번호 열 인덱스 찾기
    OutNextPartColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("NextPart");
    });
    
    // 레벨 열 인덱스 찾기
    OutLevelColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Level");
    });
    
    // S/N 열 인덱스 찾기
    OutSNColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("S/N");
    });
    
    // 타입 열 인덱스 찾기
    OutTypeColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Type");
    });
    
    // 파트 리비전 열 인덱스 찾기
    OutPartRevColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Rev");
    });
    
    // 파트 상태 열 인덱스 찾기
    OutPartStatusColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Status");
    });
    
    // 최신 여부 열 인덱스 찾기
    OutLatestColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Latest");
    });
    
    // 명칭 열 인덱스 찾기
    OutNomenclatureColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Nomenclature");
    });
    
    // 인스턴스 ID 총수량 열 인덱스 찾기
    OutInstanceIDTotalColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Instance ID 총수량(ALL DB)");
    });
    
    // 수량 열 인덱스 찾기
    OutQtyColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Qty");
    });
    
    // 인덱스를 찾지 못했을 때 기본값 설정
    OutPartNoColIdx = (OutPartNoColIdx != INDEX_NONE) ? OutPartNoColIdx : 3;      // D열 (Part No)
    OutNextPartColIdx = (OutNextPartColIdx != INDEX_NONE) ? OutNextPartColIdx : 13; // N열 (NextPart)
    OutLevelColIdx = (OutLevelColIdx != INDEX_NONE) ? OutLevelColIdx : 1;         // B열 (Level)
    OutSNColIdx = (OutSNColIdx != INDEX_NONE) ? OutSNColIdx : 0;                  // A열 (S/N)
    OutTypeColIdx = (OutTypeColIdx != INDEX_NONE) ? OutTypeColIdx : 2;            // C열 (Type)
    OutPartRevColIdx = (OutPartRevColIdx != INDEX_NONE) ? OutPartRevColIdx : 4;    // E열 (Part Rev)
    OutPartStatusColIdx = (OutPartStatusColIdx != INDEX_NONE) ? OutPartStatusColIdx : 5; // F열 (Part Status)
    OutLatestColIdx = (OutLatestColIdx != INDEX_NONE) ? OutLatestColIdx : 6;      // G열 (Latest)
    OutNomenclatureColIdx = (OutNomenclatureColIdx != INDEX_NONE) ? OutNomenclatureColIdx : 7; // H열 (Nomenclature)
    OutInstanceIDTotalColIdx = (OutInstanceIDTotalColIdx != INDEX_NONE) ? OutInstanceIDTotalColIdx : 11; // L열 (Instance ID 총수량)
    OutQtyColIdx = (OutQtyColIdx != INDEX_NONE) ? OutQtyColIdx : 12;              // M열 (Qty)
}