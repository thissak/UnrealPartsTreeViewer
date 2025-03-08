// PartImageManager.h
// FA-50M 파트 이미지 관리 클래스

#pragma once

#include "CoreMinimal.h"

// 전방 선언
struct FPartTreeItem;
class UTexture2D;
struct FSlateBrush;

/**
 * 파트 이미지 매니저 클래스
 * 파트 이미지 관련 기능을 중앙화한 싱글톤 클래스입니다.
 */
class MYPROJECT2_API FPartImageManager
{
public:
	FPartImageManager();

	/** 인스턴스 초기화 (모듈 시작 시 호출) */
	void Initialize();

	/** 이미지 존재 여부 캐싱 함수
	 * @param PartNoToItemMap - 파트 번호별 항목 맵
	 */
	void CacheImageExistence(const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap);

	/** 이미지 있는 파트 번호 집합 가져오기 */
	const TSet<FString>& GetPartsWithImageSet() const { return PartsWithImageSet; }

	/** 파트 번호별 이미지 경로 맵 가져오기 */
	const TMap<FString, FString>& GetPartNoToImagePathMap() const { return PartNoToImagePathMap; }

	/** 이미지 존재 여부 확인 함수
	 * @param PartNo - 파트 번호
	 * @return 이미지가 있으면 true, 없으면 false
	 */
	bool HasImage(const FString& PartNo) const;

	/** 파트 이미지 로드 함수
	 * @param PartNo - 파트 번호
	 * @return 로드된 텍스처, 실패 시 nullptr
	 */
	UTexture2D* LoadPartImage(const FString& PartNo);

	/** 이미지 브러시 생성 함수
	 * @param Texture - 텍스처 (nullptr일 경우 빈 브러시 생성)
	 * @return 생성된 브러시
	 */
	TSharedPtr<FSlateBrush> CreateImageBrush(UTexture2D* Texture = nullptr);

	/**
	 * 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
	 * @param Item - 확인할 항목
	 * @return 이미지가 있으면 true, 없으면 false
	 */
	bool HasChildWithImage(TSharedPtr<FPartTreeItem> Item);
    
	/**
	 * 이미지 필터링 적용 시 항목 필터링 함수
	 * @param Item - 필터링할 항목
	 * @return 필터에 포함되면 true, 아니면 false
	 */
	bool FilterItemsByImage(TSharedPtr<FPartTreeItem> Item);

private:
	/** 이미지가 있는 파트 번호 집합 */
	TSet<FString> PartsWithImageSet;

	/** 파트 번호별 이미지 경로 맵 */
	TMap<FString, FString> PartNoToImagePathMap;

	/** 초기화 여부 */
	bool bIsInitialized;
};