// Source/MyProject2/Public/ImportSettings.h
#pragma once

#include "CoreMinimal.h"
#include "ImportSettings.generated.h"

/**
 * 3DXML 파일 임포트 설정 구조체
 * 3DXML 파일 임포트 시 적용할 옵션들을 관리합니다.
 */
USTRUCT(BlueprintType)
struct FImportSettings
{
	GENERATED_BODY()

	/** 투명 재질을 가진 메시 제거 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
	bool bRemoveTransparentMeshes = true;

	/** StaticMesh 만 남기고 나머지 액터 타입 정리 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
	bool bCleanupNonStaticMeshActors = true;

	/** 임포트 성공 후 액터 선택 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
	bool bSelectActorAfterImport = true;

	/** 다시는 설정창 표시하지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
	bool bDontShowDialogAgain = false;

	/** 재질 업데이트 방식 
	 * 0 = 항상 업데이트, 1 = 기존 유지, 2 = 항상 새로 생성 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings", meta = (ClampMin = "0", ClampMax = "2"))
	int32 MaterialUpdatePolicy = 0;

	/** 기본 생성자 */
	FImportSettings()
	{
	}
};

/** 임포트 설정 저장 클래스 */
UCLASS(config = Editor)
class MYPROJECT2_API UImportSettingsManager : public UObject
{
	GENERATED_BODY()

public:
	/** 싱글턴 인스턴스 얻기 */
	static UImportSettingsManager* Get();

	/** 현재 설정 가져오기 */
	FImportSettings GetSettings() const { return CurrentSettings; }

	/** 설정 저장하기 */
	void SaveSettings(const FImportSettings& NewSettings);

	/** 기본 설정으로 초기화 */
	void ResetToDefaults();

private:
	/** 현재 설정 값 */
	UPROPERTY(config)
	FImportSettings CurrentSettings;

	/** 싱글턴 인스턴스 */
	static UImportSettingsManager* Instance;
};