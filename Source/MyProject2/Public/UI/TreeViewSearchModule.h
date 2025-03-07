// TreeViewSearchModule.h
// 파트 트리뷰 검색 기능을 위한 모듈 헤더

#pragma once

#include "CoreMinimal.h"
#include "UI/LevelBasedTreeView.h"
#include "Widgets/Input/SSearchBox.h"

/**
 * 트리뷰 검색 결과 델리게이트
 * 검색 결과를 트리뷰에 전달하기 위한 델리게이트 선언
 */
DECLARE_DELEGATE_OneParam(FOnSearchResultsReady, const TArray<TSharedPtr<FPartTreeItem>>&);

/**
 * 트리뷰 검색 활성화 상태 델리게이트
 * 검색 모드의 활성화/비활성화 상태를 알리기 위한 델리게이트
 */
DECLARE_DELEGATE_OneParam(FOnSearchModeChanged, bool);

/**
 * 트리뷰 검색 모듈 클래스
 * 파트 트리뷰의 검색 기능을 담당하는 모듈 클래스입니다.
 */
class MYPROJECT2_API STreeViewSearchWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STreeViewSearchWidget)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TMap<FString, TSharedPtr<FPartTreeItem>>& InPartNoToItemMap);
    /**
     * 생성자
     * @param InPartNoToItemMap - 파트 번호별 항목 맵 참조
     */
    STreeViewSearchWidget(const TMap<FString, TSharedPtr<FPartTreeItem>>& InPartNoToItemMap);
    
    /**
     * 소멸자
     */
    ~STreeViewSearchWidget();
    
    /**
     * 검색 위젯 생성 함수
     * @return 생성된 검색 위젯
     */
    TSharedRef<SWidget> CreateSearchWidget();
    
    /**
     * 검색 텍스트 변경 이벤트 핸들러
     * @param InText - 입력된 검색 텍스트
     */
    void OnSearchTextChanged(const FText& InText);
    
    /**
     * 검색 텍스트 확정 이벤트 핸들러
     * @param InText - 확정된 검색 텍스트
     * @param CommitType - 텍스트 확정 유형
     */
    void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);
    
    /**
     * 검색 결과 델리게이트 설정
     * @param InDelegate - 설정할 델리게이트
     */
    void SetOnSearchResultsReadyDelegate(FOnSearchResultsReady InDelegate);
    
    /**
     * 검색 모드 변경 델리게이트 설정
     * @param InDelegate - 설정할 델리게이트
     */
    void SetOnSearchModeChangedDelegate(FOnSearchModeChanged InDelegate);
    
    /**
     * 검색 상태 반환 함수
     * @return 현재 검색 중인지 여부
     */
    bool IsSearching() const;
    
    /**
     * 검색 텍스트 반환 함수
     * @return 현재 검색 텍스트
     */
    const FString& GetSearchText() const;
    
    /**
     * 검색 결과 반환 함수
     * @return 검색 결과 항목 배열
     */
    const TArray<TSharedPtr<FPartTreeItem>>& GetSearchResults() const;
    
    /**
     * 검색 실행 함수
     * @param InSearchText - 검색할 텍스트
     */
    void PerformSearch(const FString& InSearchText);
    
    /**
     * 검색 초기화 함수
     * 검색 상태 및 결과를 초기화합니다.
     */
    void ResetSearch();

private:
    /**
     * 항목이 검색어와 일치하는지 확인하는 함수
     * @param Item - 확인할 항목
     * @param InSearchText - 검색어 (소문자로 변환된 상태)
     * @return 일치 여부
     */
    bool DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText);
    
    /**
     * 검색 결과를 필터링하는 함수
     * 너무 많은 결과가 나올 경우 제한하거나 정렬합니다.
     */
    void FilterSearchResults();

private:
    /** 검색 상태 플래그 */
    bool bIsSearching;
    
    /** 현재 검색 텍스트 */
    FString SearchText;
    
    /** 검색 결과 항목 배열 */
    TArray<TSharedPtr<FPartTreeItem>> SearchResults;
    
    /** 파트 번호별 항목 맵 참조 */
    const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap;
    
    /** 검색 결과 준비 델리게이트 */
    FOnSearchResultsReady OnSearchResultsReadyDelegate;
    
    /** 검색 모드 변경 델리게이트 */
    FOnSearchModeChanged OnSearchModeChangedDelegate;
    
    /** 검색 결과 최대 표시 수 */
    static constexpr int32 MaxSearchResultsToDisplay = 100;
};

/**
 * 트리뷰 검색 모듈 인터페이스 클래스
 * SLevelBasedTreeView와 SFTreeViewSearchModule 간의 통신을 위한 인터페이스입니다.
 */
class MYPROJECT2_API ITreeViewSearchHandler
{
public:
    /**
     * 가상 소멸자
     */
    virtual ~ITreeViewSearchHandler() = default;
    
    /**
     * 최상위 항목 접기 함수
     * 검색 시작 시 호출됩니다.
     */
    virtual void FoldLevelZeroItems() = 0;
    
    /**
     * 항목까지의 경로 펼치기 함수
     * @param Item - 펼칠 경로의 대상 항목
     */
    virtual void ExpandPathToItem(const TSharedPtr<FPartTreeItem>& Item) = 0;
    
    /**
     * 항목 선택 함수
     * @param Item - 선택할 항목
     * @param bScrollIntoView - 선택한 항목으로 스크롤할지 여부
     */
    virtual void SelectItem(const TSharedPtr<FPartTreeItem>& Item, bool bScrollIntoView = true) = 0;
    
    /**
     * 항목의 부모 찾기 함수
     * @param ChildItem - 자식 항목
     * @return 부모 항목
     */
    virtual TSharedPtr<FPartTreeItem> FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem) = 0;
    
    /**
     * 한 항목이 다른 항목의 자식인지 확인하는 함수
     * @param PotentialChild - 잠재적 자식 항목
     * @param PotentialParent - 잠재적 부모 항목
     * @return 자식 여부
     */
    virtual bool IsChildOf(const TSharedPtr<FPartTreeItem>& PotentialChild, const TSharedPtr<FPartTreeItem>& PotentialParent) = 0;
};