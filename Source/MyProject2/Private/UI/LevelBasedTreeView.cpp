// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject2/Public/UI/LevelBasedTreeView.h"
#include "SlateOptMacros.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLevelBasedTreeView::Construct(const FArguments& InArgs)
{
    MaxLevel = 0;
    
    // 위젯 구성
    ChildSlot
    [
        SAssignNew(TreeView, STreeView<TSharedPtr<FPartTreeItem>>)
        .ItemHeight(24.0f)
        .TreeItemsSource(&AllRootItems)
        .OnGenerateRow(this, &SLevelBasedTreeView::OnGenerateRow)
        .OnGetChildren(this, &SLevelBasedTreeView::OnGetChildren)
        .HeaderRow
        (
            SNew(SHeaderRow)
            + SHeaderRow::Column("PartNo")
            .DefaultLabel(FText::FromString("Part No"))
            .FillWidth(1.0f)
        )
    ];
    
    // 엑셀 파일 로드 및 트리뷰 구성
    if (!InArgs._ExcelFilePath.IsEmpty())
    {
        BuildTreeView(InArgs._ExcelFilePath);
    }
}

bool SLevelBasedTreeView::ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows)
{
    // 파일을 줄 단위로 읽기
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
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

bool SLevelBasedTreeView::BuildTreeView(const FString& FilePath)
{
    // 데이터 초기화
    AllRootItems.Empty();
    PartNoToItemMap.Empty();
    LevelToItemsMap.Empty();
    MaxLevel = 0;
    
    // CSV 파일 읽기
    TArray<TArray<FString>> ExcelData;
    if (!ReadCSVFile(FilePath, ExcelData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read CSV file: %s"), *FilePath);
        return false;
    }
    
    if (ExcelData.Num() < 2) // 헤더 + 최소 1개 이상의 데이터 행 필요
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough rows in data"));
        return false;
    }
    
    // 헤더 행
    const TArray<FString>& HeaderRow = ExcelData[0];
    
    // 필요한 열 인덱스 찾기
    int32 PartNoColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("PartNo") || HeaderName == TEXT("Part No");
    });
    
    int32 NextPartColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("NextPart");
    });
    
    int32 LevelColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Level");
    });
    
    // 인덱스를 찾지 못했으면 기본값 사용 (실제 엑셀 파일 구조 기준)
    PartNoColIdx = (PartNoColIdx != INDEX_NONE) ? PartNoColIdx : 3; // D열 (Part No)
    NextPartColIdx = (NextPartColIdx != INDEX_NONE) ? NextPartColIdx : 13; // N열 (NextPart)
    LevelColIdx = (LevelColIdx != INDEX_NONE) ? LevelColIdx : 1; // B열 (Level)
    
    // 1단계: 모든 항목 생성 및 레벨별 그룹화
    CreateAndGroupItems(ExcelData, PartNoColIdx, NextPartColIdx, LevelColIdx);
    
    // 2단계: 트리 구조 구축
    BuildTreeStructure();
    
    // 트리뷰 갱신
    if (TreeView.IsValid())
    {
        TreeView->RequestTreeRefresh();
        
        // 루트 항목 확장
        for (auto& RootItem : AllRootItems)
        {
            TreeView->SetItemExpansion(RootItem, true);
        }
    }
    
    return AllRootItems.Num() > 0;
}

TSharedRef<SWidget> SLevelBasedTreeView::GetMetadataWidget()
{
    // 메타데이터 표시를 위한 텍스트 블록 생성
    TSharedRef<STextBlock> MetadataText = SNew(STextBlock)
        .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLevelBasedTreeView::GetSelectedItemMetadata)));
    
    return MetadataText;
}

// 선택된 항목의 메타데이터를 텍스트로 반환하는 메서드
FText SLevelBasedTreeView::GetSelectedItemMetadata() const
{
    // 선택된 항목 가져오기
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    
    if (SelectedItems.Num() == 0)
    {
        return FText::FromString("No item selected");
    }
    
    TSharedPtr<FPartTreeItem> SelectedItem = SelectedItems[0];
    
    // 로컬 도우미 함수 정의
    auto SafeStr = [](const FString& Str) -> FString {
        return Str.IsEmpty() ? TEXT("N/A") : Str;
    };
    
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
        *SafeStr(SelectedItem->SN),
        SelectedItem->Level,
        *SafeStr(SelectedItem->Type),
        *SafeStr(SelectedItem->PartNo),
        *SafeStr(SelectedItem->PartRev),
        *SafeStr(SelectedItem->PartStatus),
        *SafeStr(SelectedItem->Latest),
        *SafeStr(SelectedItem->Nomenclature),
        *SafeStr(SelectedItem->InstanceIDTotalAllDB),
        *SafeStr(SelectedItem->Qty),
        *SafeStr(SelectedItem->NextPart)
    );
    
    return FText::FromString(MetadataText);
}

// LevelBasedTreeView.cpp 파일에서
FString SLevelBasedTreeView::GetSafeString(const FString& InStr) const
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
}

void SLevelBasedTreeView::OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo)
{
    // 선택 변경 시 처리할 작업 (필요한 경우)
    // GetSelectedItemMetadata() 메서드는 TAttribute를 통해 자동으로 호출되므로
    // 여기에 특별한 작업이 필요 없음
}

void SLevelBasedTreeView::CreateAndGroupItems(const TArray<TArray<FString>>& ExcelData, int32 PartNoColIdx, int32 NextPartColIdx, int32 LevelColIdx)
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
    
    // 모든 행 처리
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
            PartNoToItemMap.Add(PartNo, Item);
            
            // 레벨별 그룹에 추가
            if (!LevelToItemsMap.Contains(Level))
            {
                LevelToItemsMap.Add(Level, TArray<TSharedPtr<FPartTreeItem>>());
            }
            LevelToItemsMap[Level].Add(Item);
            
            // 최대 레벨 업데이트
            MaxLevel = FMath::Max(MaxLevel, Level);
            
            // NextPart가 없는 항목은 루트 후보
            if (NextPart.IsEmpty() || NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
            {
                if (Level == 0) // 레벨 0의 항목만 실제 루트로 추가
                {
                    AllRootItems.Add(Item);
                }
            }
        }
    }
}

void SLevelBasedTreeView::BuildTreeStructure()
{
    // 레벨 0부터 MaxLevel-1까지 순회하며 부모-자식 관계 설정
    for (int32 CurrentLevel = 0; CurrentLevel < MaxLevel; ++CurrentLevel)
    {
        // 현재 레벨의 항목이 있는지 확인
        if (!LevelToItemsMap.Contains(CurrentLevel))
            continue;
            
        // 다음 레벨의 항목이 있는지 확인
        if (!LevelToItemsMap.Contains(CurrentLevel + 1))
            continue;
            
        // 현재 레벨의 모든 항목
        const TArray<TSharedPtr<FPartTreeItem>>& CurrentLevelItems = LevelToItemsMap[CurrentLevel];
        
        // 다음 레벨의 모든 항목
        TArray<TSharedPtr<FPartTreeItem>>& NextLevelItems = LevelToItemsMap[CurrentLevel + 1];
        
        // 다음 레벨의 각 항목에 대해 부모 찾기
        for (const TSharedPtr<FPartTreeItem>& ChildItem : NextLevelItems)
        {
            // NextPart가 현재 레벨의 항목 중 하나와 일치하는지 확인
            if (!ChildItem->NextPart.IsEmpty() && !ChildItem->NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
            {
                // 직접 맵을 사용하여 부모 항목 찾기 (더 효율적)
                TSharedPtr<FPartTreeItem>* ParentItemPtr = PartNoToItemMap.Find(ChildItem->NextPart);
                
                if (ParentItemPtr && (*ParentItemPtr)->Level == CurrentLevel)
                {
                    // 부모-자식 관계 설정
                    (*ParentItemPtr)->Children.Add(ChildItem);
                }
            }
        }
    }
    
    // 루트 항목들이 설정되지 않았으면, 레벨 0의 항목들을 루트로 설정
    if (AllRootItems.Num() == 0 && LevelToItemsMap.Contains(0))
    {
        AllRootItems = LevelToItemsMap[0];
    }
}

TSharedRef<ITableRow> SLevelBasedTreeView::OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    // 각 트리 항목에 대한 행 위젯 생성
    return SNew(STableRow<TSharedPtr<FPartTreeItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(4, 0))
            [
                SNew(STextBlock)
                .Text(FText::Format(NSLOCTEXT("LevelBasedTreeView", "PartNoWithLevel", "{0} (Lv.{1})"), 
                                   FText::FromString(Item->PartNo), FText::AsNumber(Item->Level)))
            ]
        ];
}

void SLevelBasedTreeView::OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren)
{
    // 자식 항목 반환
    OutChildren = Item->Children;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// 트리뷰 위젯 생성 헬퍼 함수
void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget, TSharedPtr<SWidget>& OutMetadataWidget,const FString& ExcelFilePath)
{
    // SLevelBasedTreeView 인스턴스 생성
    TSharedPtr<SLevelBasedTreeView> TreeView = SNew(SLevelBasedTreeView)
        .ExcelFilePath(ExcelFilePath);
    
    OutTreeViewWidget = TreeView;
    
    // 메타데이터 위젯 생성
    OutMetadataWidget = SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(4.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Item Details"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(2)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    // 메타데이터 내용 표시를 위한 기본 텍스트
                    TreeView->GetMetadataWidget()
                ]
            ]
        ];
}