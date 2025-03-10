// DatasmithSceneManager.h
#pragma once

#include "CoreMinimal.h"
#include "ImportSettings.h"
#include "Materials/MaterialInterface.h"

class UDatasmithImportOptions;
class UDatasmithImportFactory;

/**
 * 데이터스미스 씬과 관련된 작업을 관리하는 클래스
 * 투명 메터리얼 감지 및 해당 메시 관리 등의 기능 제공
 */
class FDatasmithSceneManager
{
public:
	/**
	 * 기본 생성자
	 */
	FDatasmithSceneManager();
    
	/**
	 * 데이터스미스 씬 객체 설정
	 * @param InDatasmithScene - 데이터스미스 씬 객체
	 * @return 성공 여부
	 */
	bool SetDatasmithScene(UObject* InDatasmithScene);
    
	/**
	 * 투명도를 가진 메터리얼 모두 찾기
	 * @return 투명도가 있는 메터리얼 배열
	 */
	TArray<UMaterialInterface*> FindTransparentMaterials();
    
	/**
	 * 투명 메터리얼을 사용하는 스태틱 메시 찾기
	 * @return 투명 메터리얼을 사용하는 스태틱 메시 배열
	 */
	TArray<UStaticMesh*> FindMeshesWithTransparentMaterials();
    
	/**
	 * 씬의 메시 또는 메터리얼 정보 출력
	 */
	void LogSceneInfo();
    
	/**
	 * 데이터스미스 씬 액터 설정
	 * @param InSceneActor - 데이터스미스 씬 액터
	 */
	void SetDatasmithSceneActor(AActor* InSceneActor);
	
	/**
	 * 저장된 데이터스미스 씬 액터 반환
	 * @return 저장된 데이터스미스 씬 액터
	 */
	AActor* GetDatasmithSceneActor() const;
    
	/**
	 * 레벨에 스폰된 데이터스미스 씬 액터 찾기 (월드 검색)
	 * @return 데이터스미스 씬 액터
	 */
	AActor* FindDatasmithSceneActor();

	/**
	 * 임포트된 DatasmithScene 객체로부터 SceneActor 찾기
	 * @param DatasmithScene - 임포트된 DatasmithScene 객체
	 * @return 찾은 DatasmithSceneActor 또는 nullptr
	 */
	AActor* FindDatasmithSceneActorFromImport(UObject* InDatasmithScene);
    
	/**
	 * 액터 이름 변경
	 * @param ActorClass - 이름을 변경할 액터 클래스
	 * @param NewName - 새 이름
	 */
	void RenameActors(TSubclassOf<AActor> ActorClass, const FString& NewName);
    
	/**
	 * 투명 메터리얼을 사용하는 액터를 별도 그룹으로 분리
	 * @param PartNo - 파트 번호 (그룹 이름에 사용)
	 */
	void SeparateTransparentActors(const FString& PartNo);
    
	/**
	 * 데이터스미스 씬 액터의 자식들 중 첫 번째 자식 찾기
	 * @return 첫 번째 자식 액터
	 */
	AActor* GetFirstChildActor();

	/**
	 * 액터의 이름을 변경하고 자식 액터를 정리
	 * @param TargetActor - 처리할 타겟 액터
	 * @param PartNo - 파트 번호 (이름에 사용)
	 * @return 처리된 액터
	 */
	AActor* RenameAndCleanupActor(AActor* TargetActor, const FString& PartNo);

	/**
	 * Datasmith 임포트 옵션 생성
	 * @param FilePath - 임포트할 파일 경로
	 * @return 설정된 임포트 옵션
	 */
	UDatasmithImportOptions* CreateImportOptions(const FString& FilePath);

	/**
	 * 3DXML 파일 임포트 및 결과 반환
	 * @param FilePath - 임포트할 파일 경로
	 * @param DestinationPath - 대상 경로 (콘텐츠 브라우저)
	 * @return 임포트된 객체 배열
	 */
	TArray<UObject*> ImportDatasmithFile(const FString& FilePath, const FString& DestinationPath);

	/**
	 * 3DXML 파일 임포트 및 처리 통합 함수
	 * @param FilePath - 임포트할 파일 경로
	 * @param PartNo - 파트 번호
	 * @param CurrentIndex - 현재 처리 중인 노드 인덱스 (기본값 1)
	 * @param TotalCount - 총 처리할 노드 수 (기본값 1)
	 * @return 처리된 결과 액터
	 */
	AActor* ImportAndProcessDatasmith(const FString& FilePath, const FString& PartNo, int32 CurrentIndex = 1, int32 TotalCount = 1);

	/**
	 * StaticMesh 액터만 유지하고 다른 자식 액터 제거
	 * @param RootActor - 루트 액터
	 */
	void CleanupNonStaticMeshActors(AActor* RootActor);

	/**
	 * 투명 메시 액터들을 제거하는 함수
	 * @param RootActor - 루트 액터
	 */
	void RemoveTransparentMeshActors(AActor* RootActor);

	void SetImportSettings(const FImportSettings& InSettings);
	

private:
	/** 데이터스미스 씬 객체 */
	TWeakObjectPtr<UObject> DatasmithScene;
	
	/** 데이터스미스 씬 액터 */
	TWeakObjectPtr<AActor> DatasmithSceneActor;
	
	/** Import Settings**/
	FImportSettings ImportSettings;
    
	/** 투명도 검사 헬퍼 함수 */
	bool HasTransparency(UMaterialInterface* Material);
    
	/** 알파 값 추출 헬퍼 함수 (16진수 문자열에서) */
	float GetAlphaValue(const FString& AlphaHex);
    
	/** 색상 이름에서 알파값 추출 */
	float ExtractAlphaFromColorName(const FString& ColorName);
    
	/** 메터리얼 맵 반환 */
	TMap<FName, UMaterialInterface*> GetMaterialMap();
    
	/** 스태틱 메시 맵 반환 */
	TMap<FName, UStaticMesh*> GetStaticMeshMap();
    
	/** 투명 메터리얼을 사용하는 월드 액터 찾기 */
	TArray<AActor*> FindTransparentActorsInWorld();

	/**
	 * 액터의 모든 하위 구조에서 StaticMeshActor 찾기
	 * @param RootActor - 루트 액터
	 * @param OutStaticMeshActors - 결과 StaticMeshActor 배열
	 */
	void FindAllStaticMeshActors(AActor* RootActor, TArray<AStaticMeshActor*>& OutStaticMeshActors);

	/**
	 * StaticMeshActor를 제외한 자식 액터들을 재귀적으로 제거
	 * @param Actor - 처리할 액터
	 * @param OutRemovedCount - 제거된 액터 수
	 */
	void RemoveNonStaticMeshChildren(AActor* Actor, int32& OutRemovedCount);
};