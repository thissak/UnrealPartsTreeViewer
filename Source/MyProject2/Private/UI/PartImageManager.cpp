// PartImageManager.cpp
// FA-50M 파트 이미지 관리 클래스 구현

#include "UI/PartImageManager.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체 정의를 위해 필요
#include "Engine/Texture2D.h"
#include "ServiceLocator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/TreeViewUtils.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

FPartImageManager::FPartImageManager()
    : bIsInitialized(false)
{
}

FPartImageManager::~FPartImageManager()
{
}

void FPartImageManager::Initialize()
{
    if (bIsInitialized)
    {
        return;
    }

    // 초기화 로직은 실제로 CacheImageExistence에서 수행됨
    bIsInitialized = true;
}

void FPartImageManager::CacheImageExistence(const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap)
{
    // Set 및 맵 초기화
    PartsWithImageSet.Empty();
    PartNoToImagePathMap.Empty();
    
    // 이미지 폴더 경로 (Game 폴더 상대 경로)
    const FString ImageDir = TEXT("/Game/Data/00_image");
    
    // 물리적 이미지 폴더 경로 (프로젝트 콘텐츠 폴더 내)
    FString PhysicalImageDir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/00_image"));
    
    // 디렉토리가 존재하는지 확인
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*PhysicalImageDir))
    {
        UE_LOG(LogTemp, Warning, TEXT("이미지 디렉토리가 존재하지 않음: %s"), *PhysicalImageDir);
        return;
    }
    
    // 폴더 내 모든 uasset 파일 찾기
    TArray<FString> FoundFiles;
    PlatformFile.FindFilesRecursively(FoundFiles, *PhysicalImageDir, TEXT(".uasset"));
    
    UE_LOG(LogTemp, Display, TEXT("이미지 캐싱 시작: 총 %d개 uasset 파일 발견"), FoundFiles.Num());
    
    // 모든 uasset 파일 처리
    for (const FString& FilePath : FoundFiles)
    {
        // 파일 이름에서 확장자 제거
        FString FileName = FPaths::GetBaseFilename(FilePath);
        
        // TreeViewUtils를 사용하여 파일 이름에서 파트 번호 추출
        FString PartNo = FTreeViewUtils::ExtractPartNoFromAssetName(FileName);
        
        // 파트 번호가 유효하고 맵에 존재하는지 확인
        if (!PartNo.IsEmpty() && PartNoToItemMap.Contains(PartNo))
        {
            // 상대 에셋 경로 생성 (/Game/...)
            FString RelativePath = FilePath;
            FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectContentDir());
            RelativePath = RelativePath.Replace(TEXT(".uasset"), TEXT(""));
            FString AssetPath = FString::Printf(TEXT("/Game/%s"), *RelativePath);
            AssetPath = AssetPath.Replace(TEXT("\\"), TEXT("/"));
            
            PartsWithImageSet.Add(PartNo);
            PartNoToImagePathMap.Add(PartNo, AssetPath);
            
            UE_LOG(LogTemp, Verbose, TEXT("이미지 매핑: PartNo=%s, AssetPath=%s"), *PartNo, *AssetPath);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("이미지 캐싱 완료: 이미지 있는 노드 %d개 / 전체 노드 %d개"), 
           PartsWithImageSet.Num(), PartNoToItemMap.Num());
}

TSharedPtr<FSlateBrush> FPartImageManager::CreateImageBrush(UTexture2D* Texture)
{
    FSlateBrush* NewBrush = new FSlateBrush();
    NewBrush->DrawAs = ESlateBrushDrawType::Image;
    NewBrush->Tiling = ESlateBrushTileType::NoTile;
    NewBrush->Mirroring = ESlateBrushMirrorType::NoMirror;
    
    if (Texture)
    {
        NewBrush->SetResourceObject(Texture);
        NewBrush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    }
    else
    {
        NewBrush->DrawAs = ESlateBrushDrawType::NoDrawType;
        NewBrush->ImageSize = FVector2D(400, 300);
    }
    
    return MakeShareable(NewBrush);
}

bool FPartImageManager::HasImage(const FString& PartNo) const
{
    return PartsWithImageSet.Contains(PartNo);
}

UTexture2D* FPartImageManager::LoadPartImage(const FString& PartNo)
{
    if (!HasImage(PartNo))
    {
        return nullptr;
    }
    
    FString AssetPath = GetImagePathForPart(PartNo);
    if (AssetPath.IsEmpty())
    {
        return nullptr;
    }
    
    // 에셋 로드 시도
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
    
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("이미지 에셋 로드 실패: %s"), *AssetPath);
    }
    
    return Texture;
}

FString FPartImageManager::GetImagePathForPart(const FString& PartNo) const
{
    if (!HasImage(PartNo))
    {
        return FString();
    }
    
    const FString* AssetPathPtr = PartNoToImagePathMap.Find(PartNo);
    if (AssetPathPtr)
    {
        return *AssetPathPtr;
    }
    
    return FString();
}

// 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
bool FPartImageManager::HasChildWithImage(TSharedPtr<FPartTreeItem> Item)
{
    if (!Item.IsValid())
        return false;
    
    // 직접 이미지가 있는지 확인
    if (HasImage(Item->PartNo))
        return true;
    
    // 자식 항목들도 확인
    for (const auto& Child : Item->Children)
    {
        if (HasChildWithImage(Child))
            return true;
    }
    
    return false;
}