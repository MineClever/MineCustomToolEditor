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

/**
 * @brief : Help to auto filter assets with specific Template Typename
 * @tparam TAsset : Should be derived form UObject
 */
template<class TAsset>
class TAssetsProcessorFormSelection_Builder : public FAssetsProcessorFormSelection_Base
{
public:
	bool bSpecificAssetType;
	bool bSourceControl;
	TAssetsProcessorFormSelection_Builder () : bSpecificAssetType (false), bSourceControl (true)
	{
	}

	TAssetsProcessorFormSelection_Builder (const bool &bSourceControl) :
        bSpecificAssetType (false), bSourceControl(bSourceControl)
	{
	}

	virtual void ProcessAssets (TArray<TAsset *> &Assets);

	virtual void Execute () override
	{
		// Filter for specific type assets
		TArray<TAsset *> Assets;
		TArray<FString> FilesPath;
		bool bHasSourceControl = false;
		if (bSourceControl) {
			if (FAssetSourceControlHelper::IsSourceControlAvailable ())
				bHasSourceControl = true;
			else
			{
				bHasSourceControl = false;
				UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Source Control Not Valid at Current, Ignore"));
			}
		}
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &AssetData = *AssetIt;
			if (TAsset *Asset = Cast<TAsset> (AssetData.GetAsset ())) {
				bSpecificAssetType = true | bSpecificAssetType;
				Assets.Add (Asset);
				if (bHasSourceControl)
					FilesPath.Add (AssetData.GetPackage ()->GetPathName());
			}
		}
		if (bHasSourceControl) FAssetSourceControlHelper::CheckOutFiles (FilesPath);

		ProcessAssets (Assets);
	}
};

// template specialization
template <class TAsset>
void TAssetsProcessorFormSelection_Builder<TAsset>::ProcessAssets (TArray<TAsset *> &Assets)
{
	UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Not Found Specialization Of Class % "), *(Assets.Last ()->GetClass ()->GetName ()));
}
