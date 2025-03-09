// LevelBasedTreeView.h - FA-50M 파트 계층 구조 표시를 위한 트리 뷰 위젯

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Images/SImage.h"
#include "UI/PartTreeViewFilter.h"
#include "UI/PartTreeItem.h"

// 전방 선언
class SPartMetadataWidget;
class FPartTreeViewFilterManager;

/**
 * 레벨 기반 트리뷰 위젯 클래스
 * CSV 파일에서 로드한 파트 정보를 계층 구조로 표시합니다.
 */
class MYPROJECT2_API SLevelBasedTreeView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLevelBasedTreeView)
        : _ExcelFilePath("")
        , _MetadataWidget(nullptr)
    {}
        SLATE_ARGUMENT(FString, ExcelFilePath)  // CSV 파일 경로
        SLATE_ARGUMENT(TSharedPtr<SPartMetadataWidget>, MetadataWidget) // 메타데이터 위젯
    SLATE_END_ARGS()

    //===== 초기화 및 기본 설정 =====//
    
    /** 위젯 생성 함수 */
    void Construct(const FArguments& InArgs);
    
    /** CSV 파일 로드 및 트리뷰 구성 */
    bool BuildTreeView(const FString& FilePath);

    //===== 검색 및 필터링 기능 =====//
    
    /** 이미지 필터링 활성화/비활성화 */
    void ToggleImageFiltering(bool bEnable);
    
    /** 항목과 하위 항목을 재귀적으로 펼치거나 접기 */
    void ExpandItemRecursively(TSharedPtr<FPartTreeItem> Item, bool bExpand);
    
    /** 검색 UI 위젯 반환 */
    TSharedRef<SWidget> GetSearchWidget();
    
    /** 검색어 변경 이벤트 핸들러 */
    void OnSearchTextChanged(const FText& InText);
    
    /** 검색어 확정 이벤트 핸들러 */
    void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);

    //===== UI 컴포넌트 관련 =====//
    
    /** 메타데이터 위젯 반환 */
    TSharedRef<SWidget> GetMetadataWidget();
    
    /** 선택된 항목 메타데이터 텍스트 반환 */
    FText GetSelectedItemMetadata() const;
    
    //===== 선택 및 상호작용 기능 =====//
    
    /** 특정 파트 번호와 일치하는 액터 선택 */
    void SelectActorByPartNo(const FString& PartNo);
    
    /** 선택된 노드에 3DXML 파일 임포트 */
	void ImportXMLToSelectedNode();

private:
    //===== 검색 관련 변수 =====//
    FString SearchText;               // 현재 검색어
    FTimerHandle ExpandTimerHandle;   // 확장 타이머
    bool bIsSearching;                // 검색 중 상태
    TArray<TSharedPtr<FPartTreeItem>> SearchResults; // 검색 결과 항목
    
    /** 레벨 0 항목들을 접는 헬퍼 함수 */
    void FoldLevelZeroItems();
    
    /** 검색 실행 */
    void PerformSearch(const FString& InSearchText);
    
    /** 항목까지의 경로 펼치기 */
    void ExpandPathToItem(const TSharedPtr<FPartTreeItem>& Item);

    //===== 컴포넌트 참조 =====//
    /** 메타데이터 위젯 참조 */
    TSharedPtr<SPartMetadataWidget> MetadataWidget;
	
    /** 필터 관리자 참조 */
	TSharedPtr<FPartTreeViewFilterManager> FilterManager;
    
    //===== 트리뷰 및 데이터 관련 변수 =====//
    TSharedPtr<STreeView<TSharedPtr<FPartTreeItem>>> TreeView; // 트리뷰 위젯
    TArray<TSharedPtr<FPartTreeItem>> AllRootItems;            // 모든 루트 항목
    TMap<FString, TSharedPtr<FPartTreeItem>> PartNoToItemMap;  // 파트 번호별 항목 맵
    TMap<int32, TArray<TSharedPtr<FPartTreeItem>>> LevelToItemsMap; // 레벨별 항목 맵
    int32 MaxLevel;                                            // 최대 레벨 깊이
    
    /** 트리 구조 구축 (부모-자식 관계 설정) */
    void BuildTreeStructure();

    //===== 이벤트 핸들러 =====//
    
    /** 트리뷰 항목 선택 변경 이벤트 */
    void OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo);
    
    /** 트리뷰 항목 더블클릭 이벤트 */
    void OnTreeItemDoubleClick(TSharedPtr<FPartTreeItem> Item);
    
    /** 컨텍스트 메뉴 생성 */
    TSharedPtr<SWidget> OnContextMenuOpening();

    //===== 트리뷰 델리게이트 =====//
    
    /** 트리뷰 행 생성 델리게이트 */
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    
    /** 트리뷰 자식 항목 반환 델리게이트 */
    void OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren);
};

/**
 * 트리뷰 위젯 생성 헬퍼 함수
 * @param OutTreeViewWidget - 출력 트리뷰 위젯
 * @param OutMetadataWidget - 출력 메타데이터 위젯
 * @param ExcelFilePath - CSV 파일 경로
 */
MYPROJECT2_API void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget,
    TSharedPtr<SWidget>& OutMetadataWidget, const FString& ExcelFilePath);