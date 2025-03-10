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

int32 FTreeViewUtils::CreateAndGroupItems(
    const TArray<TArray<FString>>& ExcelData, 
    int32 PartNoColIdx, 
    int32 NextPartColIdx, 
    int32 LevelColIdx,
    TMap<FString, TSharedPtr<FPartTreeItem>>& OutPartNoToItemMap,
    TMap<int32, TArray<TSharedPtr<FPartTreeItem>>>& OutLevelToItemsMap,
    int32& OutMaxLevel,
    TArray<TSharedPtr<FPartTreeItem>>& OutRootItems)
{
    // 필요한 열 인덱스 찾기
    const TArray<FString>& HeaderRow = ExcelData[0];
    
    int32 SNColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("S/N");
    });
    
    int32 TypeColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Type");
    });
    
    int32 PartRevColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Rev");
    });
    
    int32 PartStatusColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Status");
    });
    
    int32 LatestColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Latest");
    });
    
    int32 NomenclatureColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Nomenclature");
    });
    
    int32 InstanceIDTotalColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Instance ID 총수량(ALL DB)");
    });
    
    int32 QtyColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Qty");
    });
    
    // 인덱스를 찾지 못했을 때 기본값 설정
    SNColIdx = (SNColIdx != INDEX_NONE) ? SNColIdx : 0;
    TypeColIdx = (TypeColIdx != INDEX_NONE) ? TypeColIdx : 2;
    PartRevColIdx = (PartRevColIdx != INDEX_NONE) ? PartRevColIdx : 4;
    PartStatusColIdx = (PartStatusColIdx != INDEX_NONE) ? PartStatusColIdx : 5;
    LatestColIdx = (LatestColIdx != INDEX_NONE) ? LatestColIdx : 6;
    NomenclatureColIdx = (NomenclatureColIdx != INDEX_NONE) ? NomenclatureColIdx : 7;
    InstanceIDTotalColIdx = (InstanceIDTotalColIdx != INDEX_NONE) ? InstanceIDTotalColIdx : 11;
    QtyColIdx = (QtyColIdx != INDEX_NONE) ? QtyColIdx : 12;
    
    // 출력 맵 초기화
    OutPartNoToItemMap.Empty();
    OutLevelToItemsMap.Empty();
    OutMaxLevel = 0;
    OutRootItems.Empty();
    
    // 모든 행 처리
    int32 ValidItemCount = 0;
    
    for (int32 i = 1; i < ExcelData.Num(); ++i)
    {
        const TArray<FString>& Row = ExcelData[i];
        if (Row.Num() <= FMath::Max3(PartNoColIdx, NextPartColIdx, LevelColIdx))
            continue;
            
        FString PartNo = Row[PartNoColIdx].TrimStartAndEnd();
        FString NextPart = Row[NextPartColIdx].TrimStartAndEnd();
        
        // 레벨 파싱
        FString LevelStr = Row[LevelColIdx].TrimStartAndEnd();
        int32 Level = 0;
        
        if (!LevelStr.IsEmpty() && !LevelStr.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
        {
            Level = FCString::Atoi(*LevelStr);
        }
        
        if (!PartNo.IsEmpty())
        {
            ValidItemCount++;
            
            // 파트 항목 생성
            TSharedPtr<FPartTreeItem> Item = MakeShared<FPartTreeItem>(PartNo, NextPart, Level);
            
            // 추가 필드 설정
            Item->Type = (TypeColIdx < Row.Num()) ? Row[TypeColIdx].TrimStartAndEnd() : TEXT("");
            Item->SN = (SNColIdx < Row.Num()) ? Row[SNColIdx].TrimStartAndEnd() : TEXT("");
            Item->PartRev = (PartRevColIdx < Row.Num()) ? Row[PartRevColIdx].TrimStartAndEnd() : TEXT("");
            Item->PartStatus = (PartStatusColIdx < Row.Num()) ? Row[PartStatusColIdx].TrimStartAndEnd() : TEXT("");
            Item->Latest = (LatestColIdx < Row.Num()) ? Row[LatestColIdx].TrimStartAndEnd() : TEXT("");
            Item->Nomenclature = (NomenclatureColIdx < Row.Num()) ? Row[NomenclatureColIdx].TrimStartAndEnd() : TEXT("");
            Item->InstanceIDTotalAllDB = (InstanceIDTotalColIdx < Row.Num()) ? Row[InstanceIDTotalColIdx].TrimStartAndEnd() : TEXT("");
            Item->Qty = (QtyColIdx < Row.Num()) ? Row[QtyColIdx].TrimStartAndEnd() : TEXT("");
            
            // 맵에 추가
            OutPartNoToItemMap.Add(PartNo, Item);
            
            // 레벨별 그룹에 추가
            if (!OutLevelToItemsMap.Contains(Level))
            {
                OutLevelToItemsMap.Add(Level, TArray<TSharedPtr<FPartTreeItem>>());
            }
            OutLevelToItemsMap[Level].Add(Item);
            
            // 최대 레벨 업데이트
            OutMaxLevel = FMath::Max(OutMaxLevel, Level);
            
            // NextPart가 없는 항목은 루트 후보
            if (NextPart.IsEmpty() || NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
            {
                if (Level == 0) // 레벨 0의 항목만 실제 루트로 추가
                {
                    OutRootItems.Add(Item);
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("항목 생성 완료: 유효한 항목 %d개 처리"), ValidItemCount);
    
    return ValidItemCount;
}

FFileMatchResult FTreeViewUtils::FindMatchingFileForPartNo(
    const FString& DirectoryPath,
    const FString& FilePattern,
    const FString& PartNo,
    int32 PartIndexInFileName)
{
    FFileMatchResult Result;
    
    // 디렉토리 존재 확인
    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*DirectoryPath))
    {
        Result.ErrorMessage = FString::Printf(TEXT("디렉토리가 존재하지 않습니다: %s"), *DirectoryPath);
        return Result;
    }
    
    // 폴더 내 모든 파일 찾기
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *(DirectoryPath / FilePattern), true, false);
    
    if (FoundFiles.Num() == 0)
    {
        Result.ErrorMessage = FString::Printf(TEXT("%s 디렉토리에 %s 패턴의 파일이 없습니다."), 
                                             *DirectoryPath, *FilePattern);
        return Result;
    }
    
    // 선택된 노드와 일치하는 파일 찾기
    for (const FString& FileName : FoundFiles)
    {
        // 파일명만 추출 (경로 없이)
        FString Filename = FPaths::GetBaseFilename(FileName);
        
        // 언더바로 문자열 분리
        TArray<FString> Parts;
        Filename.ParseIntoArray(Parts, TEXT("_"));
        
        // 언더바로 구분된 부분이 충분히 있는지 확인하고, 지정된 인덱스가 파트 번호인지 확인
        if (Parts.Num() > PartIndexInFileName && Parts[PartIndexInFileName] == PartNo)
        {
            // 결과 설정
            Result.FilePath = FPaths::Combine(DirectoryPath, FileName);
            Result.FileName = Filename;
            Result.bFound = true;
            return Result;
        }
    }
    
    // 일치하는 파일을 찾지 못한 경우
    Result.ErrorMessage = FString::Printf(TEXT("파트 번호(%s)와 일치하는 파일을 찾지 못했습니다."), *PartNo);
    return Result;
}

FString FTreeViewUtils::ExtractPartNoFromAssetName(const FString& AssetName, int32 PartIndex)
{
    // 언더바로 문자열 분리
    TArray<FString> Parts;
    AssetName.ParseIntoArray(Parts, TEXT("_"));
    
    // 지정된 인덱스가 배열 범위 내에 있는지 확인
    if (Parts.Num() > PartIndex)
    {
        return Parts[PartIndex];
    }
    
    return FString();
}