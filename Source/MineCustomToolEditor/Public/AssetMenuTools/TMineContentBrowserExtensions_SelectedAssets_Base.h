#pragma once
#include "AssetToolsModule.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMineStringFormatHelper.h"


namespace  TMineContentBrowserExtensions_SelectedAssets_Internal
{
    using namespace MineFormatStringInternal;

    template<typename TAsset>
    class TMineContentBrowserExtensions_SelectedAssets_Base
    {
    public:

        FNsLocTextDescriptions SubMenuDescriptions;
        FNsLocTextDescriptions SubMenuTipDescriptions;
        FName SubMenuInExtensionHookName;
        FSlateIcon SubMenuIcon;

        virtual ~TMineContentBrowserExtensions_SelectedAssets_Base () = default;

        virtual void InitSubMenu () = 0;

        static void ExecuteProcessor (
            TSharedPtr<FAssetsProcessorFormSelection_Base> const Processor
        )
        {
            Processor->Execute ();
        }

        TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu (const TArray<FAssetData> &SelectedAssets)
        {

            static TSharedPtr<FUICommandList> CommandList;
            if (!CommandList.IsValid ())
                CommandList = MakeShareable (new FUICommandList);
            TSharedRef<FExtender> Extender (new FExtender ());

            MappingCommand (CommandList, SelectedAssets);

            if (AssetsProcessorCastHelper::CheckSelectedTypeTarget<TAsset> (SelectedAssets, true)) {
                // Add the Static actions sub-menu extender
                Extender->AddMenuExtension (
                    "GetAssetActions",
                    EExtensionHook::After,
                    CommandList,
                    FMenuExtensionDelegate::CreateRaw (this,&TMineContentBrowserExtensions_SelectedAssets_Base::CreateActionsSubMenu)
                );
            }
            return Extender;
        };

        void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {

            const FSlateIcon BaseMenuIcon = SubMenuIcon;
            const FText BaseMenuName = NsLocText (SubMenuDescriptions);
            const FText BaseMenuTip = NsLocText (SubMenuTipDescriptions);

            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateRaw (this,&TMineContentBrowserExtensions_SelectedAssets_Base::PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                SubMenuInExtensionHookName
            );

        }

        virtual void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        ) = 0;

        virtual void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        ) = 0;
    };

}

