// ImportedNodeManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/**
 * 임포트된 노드 관리 클래스
 * 파트 번호와 해당하는 액터 참조를 관리합니다.
 */
class MYPROJECT2_API FImportedNodeManager
{
public:
	/** 싱글턴 인스턴스 가져오기 */
	static FImportedNodeManager& Get();
    
	/** 임포트된 노드 등록 */
	void RegisterImportedNode(const FString& PartNo, AActor* Actor);
    
	/** 노드 임포트 여부 확인 */
	bool IsNodeImported(const FString& PartNo) const;
    
	/** 임포트된 액터 가져오기 */
	AActor* GetImportedActor(const FString& PartNo) const;
    
	/** 임포트된 노드 등록 해제 */
	void RemoveImportedNode(const FString& PartNo);
    
	/** 임포트된 노드 수 가져오기 */
	int32 GetImportedNodeCount() const { return ImportedNodes.Num(); }
    
	/** 모든 임포트된 노드 가져오기 */
	const TMap<FString, TWeakObjectPtr<AActor>>& GetAllImportedNodes() const { return ImportedNodes; }
    
	/** 모든 레벨 액터에서 임포트된 노드 초기화 */
	void InitializeFromLevelActors();
    
	/** 임포트 태그 상수 */
	static const FName ImportedTag;
    
private:
	/** 생성자 (싱글턴) */
	FImportedNodeManager();
    
	/** 싱글턴 인스턴스 */
	static FImportedNodeManager* Instance;
    
	/** 임포트된 노드 맵 (파트 번호 -> 액터) */
	TMap<FString, TWeakObjectPtr<AActor>> ImportedNodes;
};