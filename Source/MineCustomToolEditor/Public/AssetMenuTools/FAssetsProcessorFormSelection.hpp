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
		UAssetEditorSubsystem *&&AssetSubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
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
				AssetSubSystem->CloseAllEditorsForAsset (Asset);
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


namespace AssetsProcessorCastHelper
{
	template<typename P>
	static TSharedPtr<FAssetsProcessorFormSelection_Base>
	FORCEINLINE CreateBaseProcessorPtr (const TArray<FAssetData> &SelectedAssets)
	{
		static_assert (
			std::is_base_of_v<FAssetsProcessorFormSelection_Base, P>,
			"Must be derived from FAssetsProcessorFormSelection_Base"
			);

		TSharedPtr<P> Processor = MakeShareable (new P);//On Heap
		Processor->SelectedAssets = SelectedAssets;
		return StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (Processor);
	}

	/**
	 * @brief :                     Check if target type in current selections
	 * @tparam :                    Should derived from UObject
	 * @param   SelectedAssets :    Current Selections
	 * @param   bCanCast :          Test if Asset Can cast to target Type
	 * @return :                    return true if target type in current selections
	 */
	template<typename T>
	FORCEINLINE static bool CheckSelectedTypeTarget (const TArray<FAssetData> &SelectedAssets, bool bCanCast = false)
	{
		bool bCurrentType = false;
		bool bCanCastType = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
			const FAssetData &Asset = *AssetIt;
			if (bCanCast) bCanCastType = Cast<T> (Asset.GetAsset ()) != nullptr;
			bCurrentType = bCurrentType || (Asset.AssetClass == T::StaticClass ()->GetFName ()) || bCanCastType;
		}
		return bCurrentType;
	}
}

