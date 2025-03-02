// LevelBasedTreeView.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

/**
 * 트리 아이템 데이터 구조체
 * 파트 번호, 다음 파트 번호, 레벨 정보를 포함
 */
struct FPartTreeItem
{
    // 파트 번호
    FString PartNo;
    
    // 다음 파트 번호 (상위 항목)
    FString NextPart;
    
    // 레벨 (계층 구조)
    int32 Level;
    
    // 자식 항목 배열
    TArray<TSharedPtr<FPartTreeItem>> Children;
    
    // 생성자
    FPartTreeItem(const FString& InPartNo, const FString& InNextPart, int32 InLevel)
        : PartNo(InPartNo)
        , NextPart(InNextPart)
        , Level(InLevel)
    {
    }
};

/**
 * 레벨 기반 트리뷰 위젯 클래스
 * CSV 파일에서 파트 데이터를 로드하고 계층적 트리뷰로 표시
 */
class MYPROJECT2_API SLevelBasedTreeView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLevelBasedTreeView)
        : _ExcelFilePath("")
    {}
        SLATE_ARGUMENT(FString, ExcelFilePath)
    SLATE_END_ARGS()

    /**
     * 위젯 생성 함수
     */
    void Construct(const FArguments& InArgs);
    
    /**
     * 엑셀 파일 로드 및 트리뷰 구성 함수
     */
    bool BuildTreeView(const FString& FilePath);
    
private:
    // 트리뷰 위젯
    TSharedPtr<STreeView<TSharedPtr<FPartTreeItem>>> TreeView;
    
    // 모든 트리 항목
    TArray<TSharedPtr<FPartTreeItem>> AllRootItems;
    
    // 파트 번호를 키로 하는 트리 아이템 맵
    TMap<FString, TSharedPtr<FPartTreeItem>> PartNoToItemMap;
    
    // 레벨별 항목 맵 (레벨을 키로 사용)
    TMap<int32, TArray<TSharedPtr<FPartTreeItem>>> LevelToItemsMap;
    
    // 최대 레벨
    int32 MaxLevel;
    
    /**
     * CSV 파일 읽기 함수
     */
    bool ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows);
    
    /**
     * 아이템 생성 및 레벨별 그룹화
     */
    void CreateAndGroupItems(const TArray<TArray<FString>>& ExcelData, int32 PartNoColIdx, int32 NextPartColIdx, int32 LevelColIdx);
    
    /**
     * 레벨과 NextPart 정보를 기반으로 트리 구축
     */
    void BuildTreeStructure();
    
    /**
     * 트리 항목 행 생성 함수
     */
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    
    /**
     * 자식 항목 가져오기 함수
     */
    void OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren);
};

/**
 * 트리뷰 위젯 생성 헬퍼 함수
 */
MYPROJECT2_API void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutWidget, const FString& ExcelFilePath);