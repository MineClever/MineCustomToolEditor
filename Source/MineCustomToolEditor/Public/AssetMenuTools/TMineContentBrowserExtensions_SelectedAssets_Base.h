#pragma once
#include "AssetToolsModule.h"
#include "AssetMenuTools/TAssetsProcessorFormSelection.hpp"
#include "AssetCreateHelper/FMineStringFormatHelper.h"


namespace  TMineContentBrowserExtensions_SelectedAssets_Internal
{
    using namespace MineFormatStringInternal;

    /**
     * @brief : Load for MenuCommandInfoActionMap and FORMAT_COMMAND_INFO Macro
     */
    class MineAssetCtxMenuCommands_CommandMap
    {
    public:
        using FBaseProcessorFunction =
            TFunctionRef<TSharedPtr<FAssetsProcessorFormSelection_Base> (const TArray<FAssetData> &)>;

        /* Command Action Objects */
        TMap<TSharedPtr<FUICommandInfo>, FBaseProcessorFunction> MenuCommandInfoActionMap;


        /**
         * @brief : Help to bind Command Class to this->MenuCommandInfoActionMap
         * @param ID : int type; Index of Command
         * @param TXT : Command Name to Show
         * @param TIP : Command Tip ,when point on Command Name
         * @param CMD : Command Class to Register
        */
        #define FORMAT_COMMAND_INFO(ID, TXT, TIP, CMD) TSharedPtr<FUICommandInfo> MenuCommandInfo_##ID; \
                    UI_COMMAND (MenuCommandInfo_##ID, TXT, TIP, EUserInterfaceActionType::Button, FInputGesture ()); \
                    MenuCommandInfoActionMap.Emplace (MenuCommandInfo_##ID, AssetsProcessorCastHelper::CreateBaseProcessorPtr<##CMD##>);
    };

    template<typename TAsset, typename TCommand>
    class TMineContentBrowserExtensions_SelectedAssets_Base
    {
        static_assert(std::is_base_of_v<MineAssetCtxMenuCommands_CommandMap, TCommand>, "Not a Valid TCommand Class. Must derived from MineAssetCtxMenuCommands_CommandMap.");
    public:

        FNsLocTextDescriptions SubMenuDescriptions;
        FNsLocTextDescriptions SubMenuTipDescriptions;
        FName SubMenuInExtensionHookName;
        FSlateIcon SubMenuIcon;
        static FName AssetTypeName;

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

        virtual void PopulateActionsMenu (FMenuBuilder &MenuBuilder)
        {
            // Add to Menu
            static const TCommand &ToolCommandsInfo = TCommand::Get ();
            for (auto CommandInfo : ToolCommandsInfo.MenuCommandInfoActionMap) {
                MenuBuilder.AddMenuEntry (CommandInfo.Key);
            }
        }

        virtual void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static const TCommand &ToolCommandsInfo = TCommand::Get ();
            for (auto CommandInfo : ToolCommandsInfo.MenuCommandInfoActionMap) {
                CommandList->MapAction (CommandInfo.Key,
                    FExecuteAction::CreateStatic (&ExecuteProcessor, CommandInfo.Value (SelectedAssets)),
                    FCanExecuteAction ()
                );
            }
        };
    };
    
    template<typename TAsset, typename TCommand>
    FName TMineContentBrowserExtensions_SelectedAssets_Base<TAsset, TCommand>::AssetTypeName = TAsset::StaticClass ()->GetFName ();
}

