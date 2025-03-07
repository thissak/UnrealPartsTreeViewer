// TreeViewSearchModule.cpp
// 파트 트리뷰 검색 기능을 위한 모듈 구현

#include "UI/TreeViewSearchModule.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Engine/Engine.h"

STreeViewSearchWidget::STreeViewSearchWidget(const TMap<FString, TSharedPtr<FPartTreeItem>>& InPartNoToItemMap)
    : bIsSearching(false)
    , SearchText("")
    , PartNoToItemMap(InPartNoToItemMap)
{
}

STreeViewSearchWidget::~STreeViewSearchWidget()
{
}

TSharedRef<SWidget> STreeViewSearchWidget::CreateSearchWidget()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(4.0f))
        [
            SNew(SVerticalBox)
            
            // 검색창 설명 텍스트
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Search Parts"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            
            // 검색 입력창
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SSearchBox)
                    .HintText(FText::FromString("Enter text to search..."))
                    .OnTextChanged(this, &STreeViewSearchWidget::OnSearchTextChanged)
                    .OnTextCommitted(this, &STreeViewSearchWidget::OnSearchTextCommitted)
                ]
            ]
            
            // 검색 결과 정보
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText {
                    if (bIsSearching && !SearchText.IsEmpty())
                    {
                        return FText::Format(FText::FromString("Found {0} matches for '{1}'"), 
                            FText::AsNumber(SearchResults.Num()), FText::FromString(SearchText));
                    }
                    return FText::GetEmpty();
                })
                .Visibility_Lambda([this]() -> EVisibility {
                    return (bIsSearching && !SearchText.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
                })
            ]
        ];
}

void STreeViewSearchWidget::OnSearchTextChanged(const FText& InText)
{
    // 새 검색어
    FString NewSearchText = InText.ToString();
    
    // 이전에 빈 상태에서 첫 글자 입력 시 트리 접기 실행
    if (SearchText.IsEmpty() && !NewSearchText.IsEmpty() && OnSearchModeChangedDelegate.IsBound())
    {
        // 검색 모드 시작 알림
        OnSearchModeChangedDelegate.Execute(true);
    }
    
    SearchText = NewSearchText;
    
    if (SearchText.IsEmpty())
    {
        // 검색어가 비었으면 검색 중지
        bIsSearching = false;
        
        if (OnSearchModeChangedDelegate.IsBound())
        {
            // 검색 모드 종료 알림
            OnSearchModeChangedDelegate.Execute(false);
        }
    }
    else
    {
        // 검색 상태로 설정
        bIsSearching = true;
        
        // 검색 실행
        PerformSearch(SearchText);
    }
}

void STreeViewSearchWidget::OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType)
{
    if (CommitType == ETextCommit::OnEnter)
    {
        // Enter 키로 검색 실행
        SearchText = InText.ToString();
        if (!SearchText.IsEmpty())
        {
            PerformSearch(SearchText);
        }
    }
}

void STreeViewSearchWidget::SetOnSearchResultsReadyDelegate(FOnSearchResultsReady InDelegate)
{
    OnSearchResultsReadyDelegate = InDelegate;
}

void STreeViewSearchWidget::SetOnSearchModeChangedDelegate(FOnSearchModeChanged InDelegate)
{
    OnSearchModeChangedDelegate = InDelegate;
}

bool STreeViewSearchWidget::IsSearching() const
{
    return bIsSearching;
}

const FString& STreeViewSearchWidget::GetSearchText() const
{
    return SearchText;
}

const TArray<TSharedPtr<FPartTreeItem>>& STreeViewSearchWidget::GetSearchResults() const
{
    return SearchResults;
}

void STreeViewSearchWidget::PerformSearch(const FString& InSearchText)
{
    // 검색 결과 초기화
    SearchResults.Empty();
    
    // 검색어를 소문자로 변환 (대소문자 구분 없는 검색)
    FString LowerSearchText = InSearchText.ToLower();
    UE_LOG(LogTemp, Display, TEXT("검색 시작: '%s'"), *InSearchText);
    
    // 모든 항목을 순회하며 검색
    for (const auto& Pair : PartNoToItemMap)
    {
        TSharedPtr<FPartTreeItem> Item = Pair.Value;
        if (DoesItemMatchSearch(Item, LowerSearchText))
        {
            SearchResults.Add(Item);
        }
    }
    
    // 검색 결과가 너무 많을 경우 필터링
    FilterSearchResults();
    
    UE_LOG(LogTemp, Display, TEXT("검색 결과: %d개 항목 발견"), SearchResults.Num());
    
    // 검색 결과 준비 델리게이트 호출
    if (OnSearchResultsReadyDelegate.IsBound() && SearchResults.Num() > 0)
    {
        OnSearchResultsReadyDelegate.Execute(SearchResults);
    }
}

void STreeViewSearchWidget::ResetSearch()
{
    SearchText = "";
    bIsSearching = false;
    SearchResults.Empty();
    
    if (OnSearchModeChangedDelegate.IsBound())
    {
        // 검색 모드 종료 알림
        OnSearchModeChangedDelegate.Execute(false);
    }
}

bool STreeViewSearchWidget::DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText)
{
    // 대소문자 구분 없이 검색하기 위해 모든 문자열을 소문자로 변환
    FString LowerPartNo = Item->PartNo.ToLower();
    FString LowerType = Item->Type.ToLower();
    FString LowerNomenclature = Item->Nomenclature.ToLower();
    
    // 파트 번호, 유형, 명칭에서 검색
    return LowerPartNo.Contains(InSearchText) || 
           LowerType.Contains(InSearchText) || 
           LowerNomenclature.Contains(InSearchText);
}

void STreeViewSearchWidget::FilterSearchResults()
{
    // 검색 결과가 너무 많으면 제한
    if (SearchResults.Num() > MaxSearchResultsToDisplay)
    {
        // 레벨 순으로 정렬 (낮은 레벨이 먼저 오도록)
        SearchResults.Sort([](const TSharedPtr<FPartTreeItem>& A, const TSharedPtr<FPartTreeItem>& B) {
            return A->Level < B->Level;
        });
        
        // 최대 표시 수로 제한
        SearchResults.SetNum(MaxSearchResultsToDisplay);
        
        UE_LOG(LogTemp, Warning, TEXT("검색 결과가 너무 많아 %d개로 제한됨"), MaxSearchResultsToDisplay);
    }
}