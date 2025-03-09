// DatasmithSceneManager.cpp
#include "DatasmithSceneManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstance.h"
#include "UObject/UObjectGlobals.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "GameFramework/Actor.h"

FDatasmithSceneManager::FDatasmithSceneManager()
    : DatasmithScene(nullptr)
{
}

bool FDatasmithSceneManager::SetDatasmithScene(UObject* InDatasmithScene)
{
    if (!InDatasmithScene || !InDatasmithScene->GetClass()->GetName().Contains(TEXT("DatasmithScene")))
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 데이터스미스 씬 객체가 아닙니다."));
        return false;
    }
    
    DatasmithScene = InDatasmithScene;
    UE_LOG(LogTemp, Display, TEXT("데이터스미스 씬 설정 완료: %s"), *InDatasmithScene->GetName());
    return true;
}

TArray<UMaterialInterface*> FDatasmithSceneManager::FindTransparentMaterials()
{
    TArray<UMaterialInterface*> TransparentMaterials;
    
    // DatasmithScene이 유효한지 확인
    if (!DatasmithScene.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 DatasmithScene 객체가 없습니다."));
        return TransparentMaterials;
    }
    
    // 메터리얼 맵 가져오기
    TMap<FName, UMaterialInterface*> Materials = GetMaterialMap();
    
    // 모든 메터리얼 확인
    for (const auto& Pair : Materials)
    {
        UMaterialInterface* Material = Pair.Value;
        if (HasTransparency(Material))
        {
            TransparentMaterials.Add(Material);
            UE_LOG(LogTemp, Display, TEXT("투명 메터리얼 발견: %s"), *Material->GetName());
        }
    }
    
    return TransparentMaterials;
}

TArray<UStaticMesh*> FDatasmithSceneManager::FindMeshesWithTransparentMaterials()
{
    TArray<UStaticMesh*> TransparentMeshes;
    
    // DatasmithScene이 유효한지 확인
    if (!DatasmithScene.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 DatasmithScene 객체가 없습니다."));
        return TransparentMeshes;
    }
    
    // 스태틱 메시 맵 가져오기
    TMap<FName, UStaticMesh*> StaticMeshes = GetStaticMeshMap();
    
    // 투명 메터리얼 목록 가져오기
    TArray<UMaterialInterface*> TransparentMaterials = FindTransparentMaterials();
    
    // 각 메시의 메터리얼 확인
    for (const auto& Pair : StaticMeshes)
    {
        UStaticMesh* Mesh = Pair.Value;
        if (!Mesh)
            continue;
        
        bool bHasTransparentMaterial = false;
        
        // 메시의 모든 메터리얼 섹션 확인
        for (int32 i = 0; i < Mesh->GetStaticMaterials().Num(); ++i)
        {
            FStaticMaterial& StaticMaterial = Mesh->GetStaticMaterials()[i];
            UMaterialInterface* Material = StaticMaterial.MaterialInterface;
            
            if (TransparentMaterials.Contains(Material) || HasTransparency(Material))
            {
                bHasTransparentMaterial = true;
                UE_LOG(LogTemp, Verbose, TEXT("투명 메터리얼이 있는 메시: %s, 메터리얼: %s"), 
                       *Mesh->GetName(), Material ? *Material->GetName() : TEXT("None"));
                break;
            }
        }
        
        if (bHasTransparentMaterial)
        {
            TransparentMeshes.Add(Mesh);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("투명 메터리얼을 사용하는 메시: %d개"), TransparentMeshes.Num());
    return TransparentMeshes;
}

void FDatasmithSceneManager::LogSceneInfo()
{
    if (!DatasmithScene.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 DatasmithScene 객체가 없습니다."));
        return;
    }
    
    // 메터리얼 맵 가져오기
    TMap<FName, UMaterialInterface*> Materials = GetMaterialMap();
    UE_LOG(LogTemp, Display, TEXT("데이터스미스 씬 메터리얼 수: %d"), Materials.Num());
    
    // 스태틱 메시 맵 가져오기
    TMap<FName, UStaticMesh*> StaticMeshes = GetStaticMeshMap();
    UE_LOG(LogTemp, Display, TEXT("데이터스미스 씬 스태틱 메시 수: %d"), StaticMeshes.Num());
    
    // 모든 메터리얼 정보 로깅
    for (const auto& Pair : Materials)
    {
        UMaterialInterface* Material = Pair.Value;
        if (Material)
        {
            UE_LOG(LogTemp, Verbose, TEXT("메터리얼: %s, 블렌드 모드: %d"), 
                   *Material->GetName(), (int32)Material->GetBlendMode());
        }
    }
}

AActor* FDatasmithSceneManager::FindDatasmithSceneActor()
{
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Warning, TEXT("에디터 월드를 찾을 수 없습니다."));
        return nullptr;
    }
    
    for (TActorIterator<AActor> It(EditorWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetClass()->GetName().Contains(TEXT("DatasmithSceneActor")))
        {
            UE_LOG(LogTemp, Display, TEXT("DatasmithSceneActor 찾음: %s"), *Actor->GetName());
            return Actor;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("DatasmithSceneActor를 찾을 수 없습니다."));
    return nullptr;
}

void FDatasmithSceneManager::RenameActors(TSubclassOf<AActor> ActorClass, const FString& NewName)
{
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
        return;
        
    // ActorClass 타입의 모든 액터 찾기
    TArray<AActor*> FoundActors;
    for (TActorIterator<AActor> It(EditorWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->IsA(ActorClass))
        {
            FoundActors.Add(Actor);
        }
    }
    
    // 액터 이름 변경
    for (int32 i = 0; i < FoundActors.Num(); ++i)
    {
        AActor* Actor = FoundActors[i];
        
        // 기본 이름 또는 인덱스를 추가한 이름
        FString ActorName = (FoundActors.Num() > 1) ? 
            FString::Printf(TEXT("%s_%d"), *NewName, i) : NewName;
            
        // 특수 문자 제거 또는 대체
        FString SafeActorName = ActorName;
        SafeActorName.ReplaceInline(TEXT(" "), TEXT("_"));
        SafeActorName.ReplaceInline(TEXT("-"), TEXT("_"));
        
        // 이름 변경
        Actor->Rename(*SafeActorName);
        Actor->SetActorLabel(*ActorName);
        
        UE_LOG(LogTemp, Display, TEXT("액터 이름 변경: %s -> %s (Label: %s)"), 
               *Actor->GetName(), *SafeActorName, *ActorName);
    }
}

void FDatasmithSceneManager::SeparateTransparentActors(const FString& PartNo)
{
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
        return;
        
    // 투명 메시 리스트 구하기
    TArray<UStaticMesh*> TransparentMeshes = FindMeshesWithTransparentMaterials();
    if (TransparentMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("투명 메시가 없습니다."));
        return;
    }
    
    // 투명 메시를 사용하는 액터 찾기
    TArray<AStaticMeshActor*> TransparentActors;
    for (TActorIterator<AStaticMeshActor> It(EditorWorld); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        if (Actor && Actor->GetStaticMeshComponent())
        {
            UStaticMesh* ActorMesh = Actor->GetStaticMeshComponent()->GetStaticMesh();
            if (TransparentMeshes.Contains(ActorMesh))
            {
                TransparentActors.Add(Actor);
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("투명 메시를 사용하는 액터: %d개"), TransparentActors.Num());
    
    // 투명 액터를 위한 부모 액터 생성
    if (TransparentActors.Num() > 0)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = *FString::Printf(TEXT("%s_Transparent"), *PartNo);
        SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
        
        AActor* ParentActor = EditorWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
        if (ParentActor)
        {
            ParentActor->SetActorLabel(*FString::Printf(TEXT("%s_Transparent"), *PartNo));
            
            // 투명 액터를 새 부모 액터에 연결
            for (AStaticMeshActor* Actor : TransparentActors)
            {
                FTransform RelativeTransform = Actor->GetActorTransform();
                Actor->AttachToActor(ParentActor, FAttachmentTransformRules::KeepWorldTransform);
            }
            
            UE_LOG(LogTemp, Display, TEXT("투명 액터 그룹화 완료: %s"), *ParentActor->GetName());
        }
    }
}

AActor* FDatasmithSceneManager::GetFirstChildActor()
{
    AActor* SceneActor = FindDatasmithSceneActor();
    if (!SceneActor)
        return nullptr;
        
    TArray<AActor*> ChildActors;
    SceneActor->GetAttachedActors(ChildActors);
    
    if (ChildActors.Num() > 0)
    {
        return ChildActors[0];
    }
    
    return nullptr;
}

bool FDatasmithSceneManager::HasTransparency(UMaterialInterface* Material)
{
    if (!Material)
        return false;
        
    // 1. 메터리얼 블렌드 모드 확인
    EBlendMode BlendMode = Material->GetBlendMode();
    if (BlendMode != BLEND_Opaque)
    {
        return true;
    }
    
    // 2. 이름에서 알파 값 확인
    FString MaterialName = Material->GetName();
    float Alpha = ExtractAlphaFromColorName(MaterialName);
    
    // 알파값이 1.0보다 작으면 투명도 있음
    return Alpha < 1.0f;
}

float FDatasmithSceneManager::GetAlphaValue(const FString& AlphaHex)
{
    if (AlphaHex.Len() != 2)
        return 1.0f;
    
    if (uint32 AlphaInt = FParse::HexNumber(*AlphaHex))
    {
        return AlphaInt / 255.0f;
    }
    
    return 1.0f;
}

float FDatasmithSceneManager::ExtractAlphaFromColorName(const FString& ColorName)
{
    // 색상 이름이 "color_RRGGBBAA" 형식인지 확인
    if (ColorName.StartsWith(TEXT("color_")) && ColorName.Len() >= 15)
    {
        // 마지막 2자리가 알파 값
        FString AlphaHex = ColorName.Right(2);
        return GetAlphaValue(AlphaHex);
    }
    
    return 1.0f; // 기본값은 완전 불투명
}

TMap<FName, UMaterialInterface*> FDatasmithSceneManager::GetMaterialMap()
{
    TMap<FName, UMaterialInterface*> Materials;
    
    if (!DatasmithScene.IsValid())
        return Materials;
        
    // 리플렉션을 사용하여 Materials 속성 접근
    UClass* DatasmithSceneClass = DatasmithScene->GetClass();
    FProperty* MaterialsProperty = DatasmithSceneClass->FindPropertyByName(TEXT("Materials"));
    
    if (MaterialsProperty)
    {
        // TMap<FName, TSoftObjectPtr<UMaterialInterface>> 타입 가정
        FMapProperty* MapProperty = CastField<FMapProperty>(MaterialsProperty);
        if (MapProperty)
        {
            FScriptMapHelper MapHelper(MapProperty, MaterialsProperty->ContainerPtrToValuePtr<void>(DatasmithScene.Get()));
            
            for (int32 i = 0; i < MapHelper.Num(); ++i)
            {
                FName Key;
                uint8* KeyPtr = MapHelper.GetKeyPtr(i);
                FNameProperty* KeyProp = CastField<FNameProperty>(MapProperty->KeyProp);
                if (KeyProp)
                {
                    Key = KeyProp->GetPropertyValue(KeyPtr);
                }
                
                UMaterialInterface* Material = nullptr;
                uint8* ValuePtr = MapHelper.GetValuePtr(i);
                FObjectProperty* ValueProp = CastField<FObjectProperty>(MapProperty->ValueProp);
                if (ValueProp)
                {
                    Material = Cast<UMaterialInterface>(ValueProp->GetObjectPropertyValue(ValuePtr));
                }
                
                if (!Key.IsNone() && Material)
                {
                    Materials.Add(Key, Material);
                }
            }
        }
    }
    
    return Materials;
}

TMap<FName, UStaticMesh*> FDatasmithSceneManager::GetStaticMeshMap()
{
    TMap<FName, UStaticMesh*> StaticMeshes;
    
    if (!DatasmithScene.IsValid())
        return StaticMeshes;
        
    // 리플렉션을 사용하여 StaticMeshes 속성 접근
    UClass* DatasmithSceneClass = DatasmithScene->GetClass();
    FProperty* StaticMeshesProperty = DatasmithSceneClass->FindPropertyByName(TEXT("StaticMeshes"));
    
    if (StaticMeshesProperty)
    {
        // TMap<FName, TSoftObjectPtr<UStaticMesh>> 타입 가정
        FMapProperty* MapProperty = CastField<FMapProperty>(StaticMeshesProperty);
        if (MapProperty)
        {
            FScriptMapHelper MapHelper(MapProperty, StaticMeshesProperty->ContainerPtrToValuePtr<void>(DatasmithScene.Get()));
            
            for (int32 i = 0; i < MapHelper.Num(); ++i)
            {
                FName Key;
                uint8* KeyPtr = MapHelper.GetKeyPtr(i);
                FNameProperty* KeyProp = CastField<FNameProperty>(MapProperty->KeyProp);
                if (KeyProp)
                {
                    Key = KeyProp->GetPropertyValue(KeyPtr);
                }
                
                UStaticMesh* Mesh = nullptr;
                uint8* ValuePtr = MapHelper.GetValuePtr(i);
                FObjectProperty* ValueProp = CastField<FObjectProperty>(MapProperty->ValueProp);
                if (ValueProp)
                {
                    Mesh = Cast<UStaticMesh>(ValueProp->GetObjectPropertyValue(ValuePtr));
                }
                
                if (!Key.IsNone() && Mesh)
                {
                    StaticMeshes.Add(Key, Mesh);
                }
            }
        }
    }
    
    return StaticMeshes;
}

TArray<AActor*> FDatasmithSceneManager::FindTransparentActorsInWorld()
{
    TArray<AActor*> TransparentActors;
    
    // 투명 메시 리스트 구하기
    TArray<UStaticMesh*> TransparentMeshes = FindMeshesWithTransparentMaterials();
    if (TransparentMeshes.Num() == 0)
        return TransparentActors;
    
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
        return TransparentActors;
    
    // 투명 메시를 사용하는 스태틱 메시 액터 찾기
    for (TActorIterator<AStaticMeshActor> It(EditorWorld); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        if (Actor && Actor->GetStaticMeshComponent())
        {
            UStaticMesh* ActorMesh = Actor->GetStaticMeshComponent()->GetStaticMesh();
            if (TransparentMeshes.Contains(ActorMesh))
            {
                TransparentActors.Add(Actor);
            }
        }
    }
    
    return TransparentActors;
}