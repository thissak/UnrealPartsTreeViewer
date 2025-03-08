// PartImageManager.h
// FA-50M 파트 이미지 관리 클래스

#pragma once

#include "CoreMinimal.h"

// 전방 선언
struct FPartTreeItem;
class UTexture2D;

/**
 * 파트 이미지 매니저 클래스
 * 파트 이미지 데이터 관리에 집중한 클래스입니다.
 */
class MYPROJECT2_API FPartImageManager
{
public:
	FPartImageManager();
	~FPartImageManager();

	/** 인스턴스 초기화 (모듈 시작 시 호출) */
	void Initialize();
	
	/**
	 * 이미지 브러시 생성 함수
	 * @param Texture - 텍스처 (nullptr일 경우 빈 브러시 생성)
	 * @return 생성된 브러시
	 */
	TSharedPtr<FSlateBrush> CreateImageBrush(UTexture2D* Texture);

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

	/** 파트 번호에 대한 이미지 경로 가져오기
	 * @param PartNo - 파트 번호
	 * @return 이미지 경로, 없으면 빈 문자열
	 */
	FString GetImagePathForPart(const FString& PartNo) const;

	/**
	 * 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
	 * @param Item - 확인할 항목
	 * @return 이미지가 있으면 true, 없으면 false
	 */
	bool HasChildWithImage(TSharedPtr<FPartTreeItem> Item);

private:
	/** 이미지가 있는 파트 번호 집합 */
	TSet<FString> PartsWithImageSet;

	/** 파트 번호별 이미지 경로 맵 */
	TMap<FString, FString> PartNoToImagePathMap;

	/** 초기화 여부 */
	bool bIsInitialized;
};