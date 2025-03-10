// ImportedNodeManager.cpp
#include "ImportedNodeManager.h"
#include "EngineUtils.h"

// 정적 멤버 초기화
FImportedNodeManager* FImportedNodeManager::Instance = nullptr;
const FName FImportedNodeManager::ImportedTag = FName("Imported3DXML");

FImportedNodeManager& FImportedNodeManager::Get()
{
    if (!Instance)
    {
        Instance = new FImportedNodeManager();
    }
    return *Instance;
}

FImportedNodeManager::FImportedNodeManager()
{
    // 에디터 모드에서 시작 시 초기화
#if WITH_EDITOR
    InitializeFromLevelActors();
#endif
}

void FImportedNodeManager::RegisterImportedNode(const FString& PartNo, AActor* Actor)
{
    if (!Actor || PartNo.IsEmpty())
        return;
        
    // 매니저에 등록
    ImportedNodes.Add(PartNo, Actor);
    
    // 태그 추가
    Actor->Tags.AddUnique(ImportedTag);
    Actor->Tags.AddUnique(FName(*FString::Printf(TEXT("ImportedPart_%s"), *PartNo)));
    
    UE_LOG(LogTemp, Display, TEXT("노드 임포트 등록: %s"), *PartNo);
}

bool FImportedNodeManager::IsNodeImported(const FString& PartNo) const
{
    if (PartNo.IsEmpty())
        return false;
        
    const TWeakObjectPtr<AActor>* ActorPtr = ImportedNodes.Find(PartNo);
    
    // 액터 포인터가 있고 유효한지 확인
    if (ActorPtr && ActorPtr->IsValid())
    {
        return true;
    }
    
    // 참조가 유효하지 않으면 맵에서 제거 (const 우회)
    if (ActorPtr)
    {
        FImportedNodeManager* MutableThis = const_cast<FImportedNodeManager*>(this);
        MutableThis->ImportedNodes.Remove(PartNo);
    }
    
    return false;
}

AActor* FImportedNodeManager::GetImportedActor(const FString& PartNo) const
{
    if (PartNo.IsEmpty())
        return nullptr;
        
    const TWeakObjectPtr<AActor>* ActorPtr = ImportedNodes.Find(PartNo);
    if (ActorPtr && ActorPtr->IsValid())
    {
        return ActorPtr->Get();
    }
    
    return nullptr;
}

void FImportedNodeManager::RemoveImportedNode(const FString& PartNo)
{
    // 액터에서 태그 제거
    AActor* Actor = GetImportedActor(PartNo);
    if (Actor)
    {
        Actor->Tags.Remove(ImportedTag);
        Actor->Tags.Remove(FName(*FString::Printf(TEXT("ImportedPart_%s"), *PartNo)));
    }
    
    // 매니저에서 제거
    ImportedNodes.Remove(PartNo);
}

void FImportedNodeManager::InitializeFromLevelActors()
{
    // 기존 맵 초기화
    ImportedNodes.Empty();
    
    // 에디터 월드 가져오기
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!EditorWorld)
        return;
        
    // 모든 액터 탐색
    int32 ImportedCount = 0;
    for (TActorIterator<AActor> It(EditorWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->ActorHasTag(ImportedTag))
        {
            // ImportedPart_ 태그 찾기
            for (const FName& Tag : Actor->Tags)
            {
                FString TagStr = Tag.ToString();
                if (TagStr.StartsWith(TEXT("ImportedPart_")))
                {
                    // 태그에서 파트 번호 추출
                    FString PartNo = TagStr.RightChop(13); // "ImportedPart_" 부분 제거
                    
                    // 맵에 추가
                    ImportedNodes.Add(PartNo, Actor);
                    ImportedCount++;
                    break;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("레벨에서 임포트된 노드 %d개 초기화 완료"), ImportedCount);
}