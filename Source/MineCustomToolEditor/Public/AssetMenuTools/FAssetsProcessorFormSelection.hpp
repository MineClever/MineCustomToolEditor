#pragma once
#include "CoreMinimal.h"
#include "FAssetSourceControlCheckOut.hpp"
#include "MineMouduleDefine.h"

//////////////////////////////////////////////////////////////////////////
// FAssetsProcessorFormSelection_Base

class FAssetsProcessorFormSelection_Base
{
public:
	TArray<struct FAssetData> SelectedAssets;

public:
	virtual void Execute () {}
	virtual ~FAssetsProcessorFormSelection_Base () {}
};


//////////////////////////////////////////////////////////////////////////
// TAssetsProcessorFormSelection_Builder

template<class TAsset>
class TAssetsProcessorFormSelection_Builder : public FAssetsProcessorFormSelection_Base
{
public:
	bool bSpecificAssetType;

	TAssetsProcessorFormSelection_Builder () : bSpecificAssetType (false)
	{
	}

	virtual void ProcessAssets (TArray<TAsset *> &Assets);

	virtual void Execute () override
	{
		// Filter for specific type assets
		TArray<TAsset *> Assets;
		TArray<FString> FilesPath;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &AssetData = *AssetIt;
			if (TAsset *Asset = Cast<TAsset> (AssetData.GetAsset ())) {
				Assets.Add (Asset);
				FilesPath.Add (AssetData.GetFullName ());
			}
		}
		FAssetSourceControlHelper::CheckOutFiles (FilesPath);
		ProcessAssets (Assets);
	}
};

// template specialization
template <class TAsset>
void TAssetsProcessorFormSelection_Builder<TAsset>::ProcessAssets (TArray<TAsset *> &Assets)
{
	UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Not Found Specialization Of Class % "), *(Assets.Last ()->GetClass ()->GetName ()));
}
