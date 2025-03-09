// PartTreeViewFilter.h 파일 생성
#pragma once

#include "CoreMinimal.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem을 위한 포함

/**
 * 트리뷰 필터 인터페이스
 * 모든 필터는 이 인터페이스를 구현해야 합니다.
 */
class IPartTreeViewFilter
{
public:
    virtual ~IPartTreeViewFilter() {}

    /** 
     * 필터 이름 반환
     * @return 필터 이름
     */
    virtual FString GetFilterName() const = 0;

    /**
     * 필터 설명 반환
     * @return 필터 설명
     */
    virtual FString GetFilterDescription() const = 0;

    /**
     * 항목 필터링 여부 결정
     * @param Item - 필터링할 항목
     * @return true면 항목이 표시됨, false면 항목이 필터링됨
     */
    virtual bool PassesFilter(const TSharedPtr<FPartTreeItem>& Item) const = 0;

    /**
     * 활성화 상태 설정
     * @param bNewEnabled - 새 활성화 상태
     */
    virtual void SetEnabled(bool bNewEnabled) { bEnabled = bNewEnabled; }

    /**
     * 활성화 상태 반환
     * @return 필터 활성화 여부
     */
    virtual bool IsEnabled() const { return bEnabled; }

    /**
     * 필터 초기화/리셋
     */
    virtual void Reset() {}

protected:
    bool bEnabled = false;
};

/**
 * 이미지 필터 - 이미지가 있는 노드만 표시
 */
class FImageFilter : public IPartTreeViewFilter
{
public:
    FImageFilter() {}

    virtual FString GetFilterName() const override { return TEXT("이미지 있는 노드"); }
    virtual FString GetFilterDescription() const override { return TEXT("이미지가 있는 노드만 표시합니다"); }

    virtual bool PassesFilter(const TSharedPtr<FPartTreeItem>& Item) const override;
};

/**
 * 중복 노드 필터 - 중복된 파트 번호를 필터링
 */
class FDuplicateFilter : public IPartTreeViewFilter
{
public:
    FDuplicateFilter() {}

    virtual FString GetFilterName() const override { return TEXT("중복 노드 제거"); }
    virtual FString GetFilterDescription() const override { return TEXT("중복된 파트 번호를 필터링합니다"); }

    virtual bool PassesFilter(const TSharedPtr<FPartTreeItem>& Item) const override;
    virtual void Reset() override { ProcessedPartNumbers.Empty(); }

private:
    mutable TSet<FString> ProcessedPartNumbers;
};

/**
 * 필터 관리자 클래스
 */
class FPartTreeViewFilterManager
{
public:
    FPartTreeViewFilterManager();
    ~FPartTreeViewFilterManager();

    /**
     * 항목이 모든 필터를 통과하는지 확인
     * @param Item - 확인할 항목
     * @return 모든 활성화된 필터를 통과하면 true
     */
    bool PassesAllFilters(const TSharedPtr<FPartTreeItem>& Item) const;

    /**
     * 필터 활성화/비활성화
     * @param FilterName - 필터 이름
     * @param bEnabled - 활성화 여부
     */
    void SetFilterEnabled(const FString& FilterName, bool bEnabled);

    /**
     * 필터가 활성화되었는지 확인
     * @param FilterName - 필터 이름
     * @return 필터 활성화 여부
     */
    bool IsFilterEnabled(const FString& FilterName) const;

    /**
     * 필터 배열 가져오기
     * @return 필터 배열
     */
    const TArray<TSharedPtr<IPartTreeViewFilter>>& GetFilters() const { return Filters; }

    /**
     * 필터 추가
     * @param Filter - 추가할 필터
     */
    void AddFilter(TSharedPtr<IPartTreeViewFilter> Filter);

    /**
     * 모든 필터 초기화
     */
    void ResetFilters();

private:
    // 필터 배열
    TArray<TSharedPtr<IPartTreeViewFilter>> Filters;
};