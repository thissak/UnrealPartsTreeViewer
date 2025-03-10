// DatasmithSceneManager.cpp
#include "DatasmithSceneManager.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "DatasmithImportFactory.h"
#include "DatasmithImportOptions.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "ImportedNodeManager.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstance.h"
#include "UObject/UObjectGlobals.h"

FDatasmithSceneManager::FDatasmithSceneManager()
    : DatasmithScene(nullptr)
    , DatasmithSceneActor(nullptr)
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

void FDatasmithSceneManager::SetDatasmithSceneActor(AActor* InSceneActor)
{
    DatasmithSceneActor = InSceneActor;
    if (InSceneActor)
    {
        UE_LOG(LogTemp, Display, TEXT("데이터스미스 씬 액터 설정 완료: %s"), *InSceneActor->GetName());
    }
}

AActor* FDatasmithSceneManager::GetDatasmithSceneActor() const
{
    return DatasmithSceneActor.Get();
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
    // 이미 저장된 씬 액터가 있고 유효하다면 반환
    if (DatasmithSceneActor.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("저장된 DatasmithSceneActor 사용: %s"), *DatasmithSceneActor->GetName());
        return DatasmithSceneActor.Get();
    }
    
    // 저장된 씬 액터가 없으면 에디터 월드에서 검색
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
            UE_LOG(LogTemp, Display, TEXT("월드에서 DatasmithSceneActor 찾음: %s"), *Actor->GetName());
            
            // 찾은 액터를 멤버 변수에 저장
            DatasmithSceneActor = Actor;
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
    AActor* SceneActor = GetDatasmithSceneActor();
    if (!SceneActor)
    {
        // 저장된 액터가 없으면 찾기 시도
        SceneActor = FindDatasmithSceneActor();
        if (!SceneActor)
            return nullptr;
    }
        
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

AActor* FDatasmithSceneManager::FindDatasmithSceneActorFromImport(UObject* InDatasmithScene)
{
    if (!InDatasmithScene)
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 DatasmithScene 객체가 아닙니다."));
        return nullptr;
    }
    
    // DatasmithScene에서 이름 가져오기
    FString SceneActorName;
    UClass* DatasmithSceneClass = InDatasmithScene->GetClass();
    FProperty* SceneActorNameProperty = DatasmithSceneClass->FindPropertyByName(TEXT("Name"));
    
    if (SceneActorNameProperty)
    {
        FStrProperty* StrProperty = CastField<FStrProperty>(SceneActorNameProperty);
        if (StrProperty)
        {
            // InDatasmithScene를 직접 사용 (TWeakObjectPtr은 사용하지 않음)
            SceneActorName = StrProperty->GetPropertyValue(SceneActorNameProperty->ContainerPtrToValuePtr<void>(InDatasmithScene));
        }
    }
    
    // DatasmithSceneActor 찾기
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Warning, TEXT("에디터 월드를 찾을 수 없습니다."));
        return nullptr;
    }
    
    AActor* SceneActor = nullptr;
    
    // 1. 임포트된 DatasmithScene의 이름으로 찾기
    if (!SceneActorName.IsEmpty())
    {
        for (TActorIterator<AActor> It(EditorWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && Actor->GetName().Contains(SceneActorName))
            {
                SceneActor = Actor;
                UE_LOG(LogTemp, Display, TEXT("임포트된 DatasmithScene의 이름으로 SceneActor 찾음: %s"), *SceneActor->GetName());
                break;
            }
        }
    }
    
    // 2. 이름으로 찾지 못한 경우 타입으로 찾기
    if (!SceneActor)
    {
        for (TActorIterator<AActor> It(EditorWorld); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && Actor->GetClass()->GetName().Contains(TEXT("DatasmithSceneActor")))
            {
                SceneActor = Actor;
                UE_LOG(LogTemp, Display, TEXT("타입으로 SceneActor 찾음: %s"), *SceneActor->GetName());
                break;
            }
        }
    }
    
    if (SceneActor)
    {
        // 찾은 액터를 멤버 변수에 저장
        SetDatasmithSceneActor(SceneActor);
    }
    
    return SceneActor;
}

AActor* FDatasmithSceneManager::RenameAndCleanupActor(AActor* TargetActor, const FString& PartNo)
{
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("유효한 타겟 액터가 아닙니다."));
        return nullptr;
    }
    
    // 액터 이름 변경
    FString SafeActorName = PartNo;
    SafeActorName.ReplaceInline(TEXT(" "), TEXT("_"));
    SafeActorName.ReplaceInline(TEXT("-"), TEXT("_"));
    
    TargetActor->Rename(*SafeActorName);
    TargetActor->SetActorLabel(*PartNo);
    
    // StaticMesh만 남기고 다른 자식 액터 제거
    CleanupNonStaticMeshActors(TargetActor);
    
    UE_LOG(LogTemp, Display, TEXT("액터 이름 변경 및 정리 완료: %s (원래 이름: %s)"), 
           *SafeActorName, *PartNo);
           
    return TargetActor;
}

UDatasmithImportOptions* FDatasmithSceneManager::CreateImportOptions(const FString& FilePath)
{
    UDatasmithImportOptions* ImportOptions = NewObject<UDatasmithImportOptions>();
    if (!ImportOptions)
    {
        UE_LOG(LogTemp, Error, TEXT("임포트 옵션 객체를 생성할 수 없습니다."));
        return nullptr;
    }
    
    // 기본 옵션 설정
    ImportOptions->BaseOptions.SceneHandling = EDatasmithImportScene::CurrentLevel;
    ImportOptions->BaseOptions.bIncludeGeometry = true;
    ImportOptions->BaseOptions.bIncludeMaterial = true;
    ImportOptions->BaseOptions.bIncludeLight = false;
    ImportOptions->BaseOptions.bIncludeCamera = false;
    ImportOptions->BaseOptions.bIncludeAnimation = false;
    
    // 충돌 정책 설정
    ImportOptions->MaterialConflictPolicy = EDatasmithImportAssetConflictPolicy::Update;
    ImportOptions->TextureConflictPolicy = EDatasmithImportAssetConflictPolicy::Update;
    ImportOptions->StaticMeshActorImportPolicy = EDatasmithImportActorPolicy::Update;
    ImportOptions->LightImportPolicy = EDatasmithImportActorPolicy::Update;
    ImportOptions->CameraImportPolicy = EDatasmithImportActorPolicy::Update;
    ImportOptions->OtherActorImportPolicy = EDatasmithImportActorPolicy::Update;
    ImportOptions->MaterialQuality = EDatasmithImportMaterialQuality::UseRealFresnelCurves;
    
    // 스태틱 메시 옵션 설정
    ImportOptions->BaseOptions.StaticMeshOptions.MinLightmapResolution = EDatasmithImportLightmapMin::LIGHTMAP_64;
    ImportOptions->BaseOptions.StaticMeshOptions.MaxLightmapResolution = EDatasmithImportLightmapMax::LIGHTMAP_1024;
    ImportOptions->BaseOptions.StaticMeshOptions.bGenerateLightmapUVs = true;
    ImportOptions->BaseOptions.StaticMeshOptions.bRemoveDegenerates = true;
    
    // 파일 정보 설정
    FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
    ImportOptions->FileName = FPaths::GetCleanFilename(FullFilePath);
    ImportOptions->FilePath = FullFilePath;
    ImportOptions->SourceUri = FullFilePath;
    
    return ImportOptions;
}

TArray<UObject*> FDatasmithSceneManager::ImportDatasmithFile(const FString& FilePath, const FString& DestinationPath)
{
    TArray<UObject*> ImportedObjects;
    
    // Datasmith 임포트 팩토리 생성
    UDatasmithImportFactory* DatasmithFactory = NewObject<UDatasmithImportFactory>();
    if (!DatasmithFactory)
    {
        UE_LOG(LogTemp, Error, TEXT("Datasmith 임포트 팩토리를 생성할 수 없습니다."));
        return ImportedObjects;
    }
    
    // 임포트 옵션 생성
    UDatasmithImportOptions* ImportOptions = CreateImportOptions(FilePath);
    if (!ImportOptions)
    {
        return ImportedObjects;
    }
    
    // 임포트 태스크 생성
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->Filename = FPaths::ConvertRelativePathToFull(FilePath);
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->Options = ImportOptions;
    ImportTask->Factory = DatasmithFactory;
    ImportTask->bSave = true;
    ImportTask->bAutomated = true;
    ImportTask->bAsync = false;  // 동기 임포트로 설정
    
    // 임포트 태스크 준비
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UAssetImportTask*> ImportTasks;
    ImportTasks.Add(ImportTask);
    
    // 프로그레스 대화상자 표시
    GWarn->BeginSlowTask(FText::FromString(TEXT("3DXML 파일 임포트 중...")), true);
    
    // 임포트 수행 (동기적으로 실행)
    AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
    
    // 프로그레스 대화상자 종료
    GWarn->EndSlowTask();
    
    UE_LOG(LogTemp, Display, TEXT("3DXML 파일 임포트 완료: %s"), *FilePath);
    
    // 임포트된 객체 반환
    ImportedObjects = ImportTask->GetObjects();
    
    return ImportedObjects;
}

AActor* FDatasmithSceneManager::ImportAndProcessDatasmith(const FString& FilePath, const FString& PartNo)
{
    // 파일 임포트
    FString DestinationPath = FString::Printf(TEXT("/Game/Datasmith/%s"), *PartNo);
    TArray<UObject*> ImportedObjects = ImportDatasmithFile(FilePath, DestinationPath);
    
    // DatasmithScene 객체 찾기
    for (UObject* Object : ImportedObjects)
    {
        if (Object && Object->GetClass()->GetName().Contains(TEXT("DatasmithScene")))
        {
            // DatasmithScene 객체를 설정
            if (SetDatasmithScene(Object))
            {
                UE_LOG(LogTemp, Display, TEXT("DatasmithScene 객체 찾음: %s"), *Object->GetName());
                
                // DatasmithSceneActor 찾기
                AActor* SceneActor = FindDatasmithSceneActorFromImport(Object);
                if (!SceneActor)
                {
                    UE_LOG(LogTemp, Warning, TEXT("DatasmithSceneActor를 찾을 수 없습니다."));
                    return nullptr;
                }
                
                // 자식 액터 찾기
                AActor* TargetActor = nullptr;
                TArray<AActor*> ChildActors;
                SceneActor->GetAttachedActors(ChildActors);
                
                if (ChildActors.Num() > 0)
                {
                    TargetActor = ChildActors[0];
                }
                
                if (!TargetActor)
                {
                    UE_LOG(LogTemp, Warning, TEXT("자식 액터를 찾을 수 없습니다."));
                    return nullptr;
                }
                
                // 액터 이름 변경 및 정리
                TargetActor = RenameAndCleanupActor(TargetActor, PartNo);

            	// 투명도가 있는 메시 찾기 및 제거
            	RemoveTransparentMeshActors(TargetActor);
            	
            	// 임포트된 노드 관리자에 등록
            	FImportedNodeManager::Get().RegisterImportedNode(PartNo, TargetActor);
                
                // DatasmithSceneActor 제거
                UE_LOG(LogTemp, Display, TEXT("DatasmithSceneActor 제거: %s"), *SceneActor->GetName());
                SceneActor->Destroy();
                
                return TargetActor;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("임포트된 DatasmithScene 객체를 찾을 수 없습니다."));
    return nullptr;
}

void FDatasmithSceneManager::RemoveTransparentMeshActors(AActor* RootActor)
{
	if (!RootActor)
		return;
    
	// 모든 StaticMesh 액터 찾기
	TArray<AStaticMeshActor*> StaticMeshActors;
	FindAllStaticMeshActors(RootActor, StaticMeshActors);
    
	UE_LOG(LogTemp, Display, TEXT("총 메시 액터 발견: %d개"), StaticMeshActors.Num());
    
	// 제거할 투명 메시 액터 목록
	TArray<AStaticMeshActor*> ActorsToRemove;
    
	// 각 메시 액터에 대해 투명도 확인
	for (AStaticMeshActor* MeshActor : StaticMeshActors)
	{
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
			continue;
        
		UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
		bool bHasTransparentMaterial = false;
        
		// 메시의 모든 머티리얼 확인
		for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
		{
			UMaterialInterface* Material = MeshComp->GetMaterial(i);
			if (HasTransparency(Material))
			{
				bHasTransparentMaterial = true;
				UE_LOG(LogTemp, Verbose, TEXT("투명 머티리얼 발견: %s (액터: %s)"), 
					  Material ? *Material->GetName() : TEXT("None"),
					  *MeshActor->GetName());
				break;
			}
		}
        
		// 투명 머티리얼이 있는 메시 액터는 제거 목록에 추가
		if (bHasTransparentMaterial)
		{
			ActorsToRemove.Add(MeshActor);
		}
	}
    
	// 투명 메시 액터 제거
	int32 RemovedCount = 0;
	for (AStaticMeshActor* ActorToRemove : ActorsToRemove)
	{
		if (ActorToRemove)
		{
			FString ActorName = ActorToRemove->GetName();
			ActorToRemove->Destroy();
			RemovedCount++;
            
			UE_LOG(LogTemp, Verbose, TEXT("투명 메시 액터 제거: %s"), *ActorName);
		}
	}
    
	UE_LOG(LogTemp, Display, TEXT("투명 메시 액터 제거 완료: %d개 제거됨 (총 %d개 중)"), 
		  RemovedCount, StaticMeshActors.Num());
}

void FDatasmithSceneManager::CleanupNonStaticMeshActors(AActor* RootActor)
{
    if (!RootActor)
        return;
        
    // 모든 StaticMesh 액터 찾기
    TArray<AStaticMeshActor*> StaticMeshActors;
    FindAllStaticMeshActors(RootActor, StaticMeshActors);
    
    UE_LOG(LogTemp, Display, TEXT("StaticMesh 액터 발견: %d개"), StaticMeshActors.Num());
    
    // 모든 StaticMesh 액터를 루트 액터에 직접 연결
    for (AStaticMeshActor* MeshActor : StaticMeshActors)
    {
        // 이미 직접 자식이면 건너뛰기
        AActor* CurrentParent = MeshActor->GetAttachParentActor();
        if (CurrentParent == RootActor)
            continue;
            
        // 월드 변환 저장
        FTransform OriginalTransform = MeshActor->GetActorTransform();
        
        // 현재 부모에서 분리
        MeshActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        
        // 루트 액터에 직접 연결
        MeshActor->AttachToActor(RootActor, FAttachmentTransformRules::KeepWorldTransform);
        
        UE_LOG(LogTemp, Verbose, TEXT("StaticMesh 액터 '%s'를 루트에 직접 연결"), *MeshActor->GetName());
    }
    
    // StaticMesh가 아닌 자식 액터 제거
    int32 RemovedCount = 0;
    RemoveNonStaticMeshChildren(RootActor, RemovedCount);
    
    UE_LOG(LogTemp, Display, TEXT("StaticMesh가 아닌 액터 제거 완료: %d개 제거됨"), RemovedCount);
}

void FDatasmithSceneManager::FindAllStaticMeshActors(AActor* RootActor, TArray<AStaticMeshActor*>& OutStaticMeshActors)
{
    if (!RootActor)
        return;
    
    // 자식 액터 목록 가져오기
    TArray<AActor*> ChildActors;
    RootActor->GetAttachedActors(ChildActors);
    
    // 각 자식 액터에 대해 처리
    for (AActor* ChildActor : ChildActors)
    {
        if (ChildActor)
        {
            // StaticMeshActor인 경우 결과 배열에 추가
            if (ChildActor->IsA(AStaticMeshActor::StaticClass()))
            {
                OutStaticMeshActors.Add(Cast<AStaticMeshActor>(ChildActor));
            }
            
            // 하위 액터들을 재귀적으로 검색
            FindAllStaticMeshActors(ChildActor, OutStaticMeshActors);
        }
    }
}

void FDatasmithSceneManager::RemoveNonStaticMeshChildren(AActor* Actor, int32& OutRemovedCount)
{
    if (!Actor)
        return;
    
    // 자식 액터 목록 가져오기
    TArray<AActor*> ChildActors;
    Actor->GetAttachedActors(ChildActors);
    
    // 각 자식 액터에 대해 처리
    for (AActor* ChildActor : ChildActors)
    {
        if (ChildActor)
        {
            // StaticMeshActor인 경우 스킵하고 보존
            if (ChildActor->IsA(AStaticMeshActor::StaticClass()))
            {
                UE_LOG(LogTemp, Verbose, TEXT("StaticMeshActor 유지: %s"), *ChildActor->GetName());
            }
            else
            {
                // 일반 Actor인 경우, 그 자식들을 먼저 재귀적으로 처리
                RemoveNonStaticMeshChildren(ChildActor, OutRemovedCount);
                
                // 그 다음 현재 Actor 제거
                UE_LOG(LogTemp, Verbose, TEXT("액터 제거: %s"), *ChildActor->GetName());
                ChildActor->Destroy();
                OutRemovedCount++;
            }
        }
    }
}