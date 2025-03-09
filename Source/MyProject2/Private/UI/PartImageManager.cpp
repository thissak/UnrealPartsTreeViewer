// PartImageManager.cpp
// FA-50M 파트 이미지 관리 클래스 구현

#include "UI/PartImageManager.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체 정의를 위해 필요
#include "Engine/Texture2D.h"
#include "ServiceLocator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/TreeViewUtils.h"

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
    
    // 이미지 디렉토리 경로 (Game 폴더 상대 경로)
    const FString ImageDir = TEXT("/Game/Data/00_image");
    
    // 에셋 레지스트리 활용
    TArray<FAssetData> AssetList;
    FARFilter Filter;
    
    // ClassNames 대신 ClassPaths 사용
    Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*ImageDir);
    
    // 에셋 레지스트리에서 모든 텍스처 에셋 찾기
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().GetAssets(Filter, AssetList);
    UE_LOG(LogTemp, Display, TEXT("이미지 캐싱 시작: 총 %d개 에셋 발견"), AssetList.Num());
    
    // 에셋 처리
    for (const FAssetData& Asset : AssetList)
    {
        FString AssetName = Asset.AssetName.ToString();
        FString AssetPath = Asset.GetObjectPathString();
        
        // TreeViewUtils를 사용하여 에셋 이름에서 파트 번호 추출
        FString PartNo = FTreeViewUtils::ExtractPartNoFromAssetName(AssetName);
        
        // 파트 번호가 유효하고 맵에 존재하는지 확인
        if (!PartNo.IsEmpty() && PartNoToItemMap.Contains(PartNo))
        {
            PartsWithImageSet.Add(PartNo);
            // 실제 에셋 경로 저장
            PartNoToImagePathMap.Add(PartNo, AssetPath);
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
        UE_LOG(LogTemp, Warning, TEXT("이미지 로드 실패: %s"), *AssetPath);
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
    
    // 경로가 저장되지 않은 경우 기존 방식으로 경로 구성
    return FString::Printf(TEXT("/Game/00_image/aaa_bbb_ccc_%s"), *PartNo);
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