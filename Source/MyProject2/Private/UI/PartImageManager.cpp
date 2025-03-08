// PartImageManager.cpp
// FA-50M 파트 이미지 관리 클래스 구현

#include "UI/PartImageManager.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체 정의를 위해 필요
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"

// FPartImageManager 싱글톤 구현
FPartImageManager* FPartImageManager::Instance = nullptr;

FPartImageManager& FPartImageManager::Get()
{
    if (Instance == nullptr)
    {
        Instance = new FPartImageManager();
    }
    return *Instance;
}

FPartImageManager::FPartImageManager()
    : bIsInitialized(false)
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
    const FString ImageDir = TEXT("/Game/00_image");
    
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
        
        // 언더바로 문자열 분리
        TArray<FString> Parts;
        AssetName.ParseIntoArray(Parts, TEXT("_"));
        
        // 언더바로 구분된 부분이 4개 이상인지 확인
        if (Parts.Num() >= 4)
        {
            // 3번째 인덱스가 파트 번호
            FString PartNo = Parts[3];
            
            // PartsWithImageSet에 파트 번호 추가
            if(PartNoToItemMap.Contains(PartNo))
            {
                PartsWithImageSet.Add(PartNo);
                // 실제 에셋 경로 저장
                PartNoToImagePathMap.Add(PartNo, AssetPath);
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("이미지 캐싱 완료: 이미지 있는 노드 %d개 / 전체 노드 %d개"), 
           PartsWithImageSet.Num(), PartNoToItemMap.Num());
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
    
    const FString* AssetPathPtr = PartNoToImagePathMap.Find(PartNo);
    FString AssetPath;
    
    if (AssetPathPtr)
    {
        AssetPath = *AssetPathPtr;
    }
    else
    {
        // 경로가 저장되지 않은 경우 기존 방식으로 경로 구성
        AssetPath = FString::Printf(TEXT("/Game/00_image/aaa_bbb_ccc_%s"), *PartNo);
    }
    
    // 에셋 로드 시도
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
    
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("이미지 로드 실패: %s"), *AssetPath);
    }
    
    return Texture;
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

// 이미지 필터링 적용 시 항목 필터링 함수
bool FPartImageManager::FilterItemsByImage(TSharedPtr<FPartTreeItem> Item)
{
    if (!Item.IsValid())
        return false;
        
    // 현재 항목에 이미지가 있는지 확인
    bool bCurrentItemHasImage = HasImage(Item->PartNo);
    
    // 하위 항목도 확인
    bool bAnyChildHasImage = false;
    
    for (auto& ChildItem : Item->Children)
    {
        if (FilterItemsByImage(ChildItem))
        {
            bAnyChildHasImage = true;
        }
    }
    
    // 현재 항목이나 하위 항목 중 하나라도 이미지가 있으면 true 반환
    return bCurrentItemHasImage || bAnyChildHasImage;
}