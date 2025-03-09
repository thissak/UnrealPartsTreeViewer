// DatasmithSceneManager.h
#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"

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
	 * 레벨에 스폰된 데이터스미스 씬 액터 찾기
	 * @return 데이터스미스 씬 액터
	 */
	AActor* FindDatasmithSceneActor();
    
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

private:
	/** 데이터스미스 씬 객체 */
	TWeakObjectPtr<UObject> DatasmithScene;
    
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
};