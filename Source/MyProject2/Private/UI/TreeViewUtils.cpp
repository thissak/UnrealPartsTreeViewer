// TreeViewUtils.cpp
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 구현

#include "UI/TreeViewUtils.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체 정의를 위해 필요
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"

// CSV 파일 읽기 함수
bool FTreeViewUtils::ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows)
{
    // 파일을 줄 단위로 읽기
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("파일 로드 실패: %s"), *FilePath);
        return false;
    }
    
    for (const FString& Line : Lines)
    {
        if (Line.IsEmpty())
            continue;
            
        TArray<FString> Cells;
        
        // CSV 파싱 (쉼표로 구분된 셀)
        bool bInQuotes = false;
        FString CurrentCell;
        
        for (int32 i = 0; i < Line.Len(); i++)
        {
            TCHAR CurrentChar = Line[i];
            
            if (CurrentChar == TEXT('"'))
            {
                bInQuotes = !bInQuotes;
            }
            else if (CurrentChar == TEXT(',') && !bInQuotes)
            {
                Cells.Add(CurrentCell);
                CurrentCell.Empty();
            }
            else
            {
                CurrentCell.AppendChar(CurrentChar);
            }
        }
        
        // 마지막 셀 추가
        Cells.Add(CurrentCell);
        
        OutRows.Add(Cells);
    }
    
    return OutRows.Num() > 0;
}

// 문자열 유틸리티 함수
FString FTreeViewUtils::GetSafeString(const FString& InStr)
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
}

// 항목이 검색어와 일치하는지 확인하는 함수
bool FTreeViewUtils::DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText)
{
    if (!Item.IsValid())
        return false;
        
    // 대소문자 구분 없이 검색하기 위해 모든 문자열을 소문자로 변환
    FString LowerPartNo = Item->PartNo.ToLower();
    FString LowerType = Item->Type.ToLower();
    FString LowerNomenclature = Item->Nomenclature.ToLower();
    
    // 파트 번호, 유형, 명칭에서 검색
    return LowerPartNo.Contains(InSearchText) || 
           LowerType.Contains(InSearchText) || 
           LowerNomenclature.Contains(InSearchText);
}

// 한 항목이 다른 항목의 자식인지 확인하는 함수
bool FTreeViewUtils::IsChildOf(const TSharedPtr<FPartTreeItem>& PotentialChild, const TSharedPtr<FPartTreeItem>& PotentialParent)
{
    if (!PotentialChild.IsValid() || !PotentialParent.IsValid())
    {
        return false;
    }
    
    // 직접적인 자식인지 확인
    for (const auto& DirectChild : PotentialParent->Children)
    {
        if (DirectChild == PotentialChild)
        {
            return true;
        }
        
        // 재귀적으로 하위 자식들 확인
        if (IsChildOf(PotentialChild, DirectChild))
        {
            return true;
        }
    }
    
    return false;
}

// 항목의 부모 항목 찾기 함수
TSharedPtr<FPartTreeItem> FTreeViewUtils::FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem, const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap)
{
    if (!ChildItem.IsValid() || ChildItem->NextPart.IsEmpty() || ChildItem->NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
    {
        return nullptr;
    }
    
    // NextPart를 기반으로 부모 항목 찾기
    const TSharedPtr<FPartTreeItem>* ParentPtr = PartNoToItemMap.Find(ChildItem->NextPart);
    return ParentPtr ? *ParentPtr : nullptr;
}

// 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
bool FTreeViewUtils::HasChildWithImage(TSharedPtr<FPartTreeItem> Item, const TSet<FString>& PartsWithImageSet)
{
    if (!Item.IsValid())
        return false;
    
    // 직접 이미지가 있는지 확인
    if (PartsWithImageSet.Contains(Item->PartNo))
        return true;
    
    // 자식 항목들도 확인
    for (const auto& Child : Item->Children)
    {
        if (HasChildWithImage(Child, PartsWithImageSet))
            return true;
    }
    
    return false;
}

// 이미지 필터링 적용 시 항목 필터링 함수
bool FTreeViewUtils::FilterItemsByImage(TSharedPtr<FPartTreeItem> Item, const TSet<FString>& PartsWithImageSet)
{
    // 현재 항목에 이미지가 있는지 확인
    bool bCurrentItemHasImage = PartsWithImageSet.Contains(Item->PartNo);
    
    // 하위 항목도 확인
    bool bAnyChildHasImage = false;
    
    for (auto& ChildItem : Item->Children)
    {
        if (FilterItemsByImage(ChildItem, PartsWithImageSet))
        {
            bAnyChildHasImage = true;
        }
    }
    
    // 현재 항목이나 하위 항목 중 하나라도 이미지가 있으면 true 반환
    return bCurrentItemHasImage || bAnyChildHasImage;
}

// 이미지 존재 여부 확인 함수
bool FTreeViewUtils::HasImage(const FString& PartNo, const TSet<FString>& PartsWithImageSet)
{
    return PartsWithImageSet.Contains(PartNo);
}

// 이미지 브러시 생성 함수
TSharedPtr<FSlateBrush> FTreeViewUtils::CreateImageBrush(UTexture2D* Texture)
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

// 파트 이미지 로드 함수
UTexture2D* FTreeViewUtils::LoadPartImage(const FString& PartNo, 
                                         const TSet<FString>& PartsWithImageSet,
                                         const TMap<FString, FString>& PartNoToImagePathMap)
{
    if (!PartsWithImageSet.Contains(PartNo))
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

// 이미지 존재 여부 캐싱 함수
void FTreeViewUtils::CacheImageExistence(
    const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap,
    TSet<FString>& OutPartsWithImageSet,
    TMap<FString, FString>& OutPartNoToImagePathMap)
{
    // Set 및 맵 초기화
    OutPartsWithImageSet.Empty();
    OutPartNoToImagePathMap.Empty();
    
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
                OutPartsWithImageSet.Add(PartNo);
                // 실제 에셋 경로 저장
                OutPartNoToImagePathMap.Add(PartNo, AssetPath);
            }
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("이미지 캐싱 완료: 이미지 있는 노드 %d개 / 전체 노드 %d개"), 
           OutPartsWithImageSet.Num(), PartNoToItemMap.Num());
}