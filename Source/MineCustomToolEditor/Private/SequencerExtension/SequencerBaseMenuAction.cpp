#include <SequencerExtension/SequencerBaseMenuAction.h>
#include "AssetCreateHelper/FMineStringFormatHelper.h"
#include <AssetCreateHelper/FMinePackageToObjectHelper.hpp>

#include "GeometryCache.h"
#include "LevelSequence.h"
#include "ISequencer.h"
#include "ILevelSequenceEditorToolkit.h"
#include "MovieScene.h"
#include "ConfigIO/ConfigIO.h"
#include "Sections/MovieScenePrimitiveMaterialSection.h"
#include "Tracks/MovieScenePrimitiveMaterialTrack.h"

#define LOCTEXT_NAMESPACE "FMineSequencerBaseMenuAction"

// Helper Functions
namespace FMineSequencerBaseMenuAction_Helper_Internal
{
    template<typename TObjType>
    bool HasValidType (TArray<UObject *> InUObject, const bool bCanCast=false)
    {
        bool bCurrentType = false;
        bool bCanCastType = false;
        for (auto ObjIt = InUObject.CreateConstIterator();ObjIt;++ObjIt)
        {
            if (bCurrentType) break;
            auto const CurObj = *ObjIt;
            bCanCastType = bCanCast ? (Cast<TObjType> (CurObj) != nullptr) : false;
            bCurrentType = CurObj->StaticClass ()->GetFName () == TObjType::StaticClass ()->GetFName () || bCanCastType;
        }
        return bCurrentType;
    }

    FORCEINLINE static UObject *FGuidToUObject (const FGuid &Guid)
    {
        return FUniqueObjectGuid (Guid).ResolveObject ();
    }

    static TArray<UObject *> FGuidToUObject (const TArray<FGuid> &GuidArray)
    {
        TArray<UObject *> ObjectArray;
        for (FGuid Guid : GuidArray) {
            ObjectArray.Emplace (FGuidToUObject (Guid));
        }
        return ObjectArray;
    }

    FORCEINLINE static void GetCurrentEditedAssetsObject (TArray<UObject *> &RefAssets)
    {
        RefAssets.Empty ();
        UAssetEditorSubsystem *&&LEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
        RefAssets = LEditorSubsystem->GetAllEditedAssets ();
    }

    struct FSequencerHelperFunctions
    {

        /**
         * @brief : found sequence in editing, and GUID of the selections
         * @param SequencerEditor: Access Sequence in Editor
         * @return : sequence in current sequencer editor
         */
        static ULevelSequence* GetFocusSequence (TSharedPtr<ISequencer> &SequencerEditor)
        {
            // Find all Assets opened in Editor
            UAssetEditorSubsystem * &&LEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem> ();
            TArray<UObject *> Assets = LEditorSubsystem->GetAllEditedAssets ();

            for (UObject *Asset : Assets) {
                // Find Sequencer Editor by Asset
                IAssetEditorInstance * AssetEditor = LEditorSubsystem->FindEditorForAsset (Asset, false);
                ILevelSequenceEditorToolkit * LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit *>(AssetEditor);

                if (LevelSequenceEditor) {
                    // The LevelSequence Object
                    ULevelSequence *LevelSeq = Cast<ULevelSequence> (Asset);
                    
                    // Get Current Level Sequencer FSequencer Tool
                    SequencerEditor = LevelSequenceEditor->GetSequencer();

                    return LevelSeq;
                }
            }
            return nullptr;
        }

    };

    class FTestAction
    {
    public:
        static void RunTest ()
        {
            FString const TempDebugString = TEXT ("Do Test to Sequencer!");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempDebugString);
            GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *TempDebugString);
        }; // End of Function


    }; // End Of Class

}

// Asset Processor Class
namespace FMineSequencerBaseMenuAction_Internal
{
    using namespace FMineSequencerBaseMenuAction_Helper_Internal;
    class FMineSequencerAction_SetHiddenProxyMatKey
    {

    public:
        static const UMineEditorConfigSettings * LoadConfig ()
        {
            return GetDefault<UMineEditorConfigSettings> ();
        }

        static void DoProcess ()
        {

            FString const TempDebugString = TEXT ("Do Test to Sequencer!");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), *TempDebugString);
            GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, *TempDebugString);

            // Find Sequence
            TSharedPtr<ISequencer> SequencerEditor;
            ULevelSequence *const LevelSequence = FSequencerHelperFunctions::GetFocusSequence (SequencerEditor);

            if (LevelSequence != nullptr) {
                // Tool
                TArray<FGuid> BindingsGuid;
                SequencerEditor->GetSelectedObjects (BindingsGuid);
                UMovieScene *MovieScene = LevelSequence->GetMovieScene ();

                // Make New section with Proxy Material!
                UObject *MaterialObject;
                LoadHiddenProxyMat (MaterialObject);

                // Find Binding in Current Sequence
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Sequence is %s;\n"), *LevelSequence->GetName ());
                for (FGuid Guid : BindingsGuid) {
                    FMovieSceneBinding *const Binding = MovieScene->FindBinding (Guid);
                    if (Binding == nullptr) continue;
                    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Current Sequence Object %s Has been selected in Sequencer Editor;\n"), *Binding->GetName ());

                    // Make a track to add material switcher

                    /* Check if already valid material switch track here */

                    TArray<UMovieSceneTrack *> CurBindingTracks = Binding->GetTracks ();
                    for (UMovieSceneTrack const*const Track : CurBindingTracks) {
                        UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Current Track Info :%s, %s;\n"), *Track->GetDisplayName ().ToString (), *Track->GetClass ()->GetName ());
                        if (Track->GetClass ()->GetFName () == UMovieScenePrimitiveMaterialTrack::StaticClass ()->GetFName ()) {
                            UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Track %s is MaterialTrack;\n"), *Track->GetDisplayName ().ToString ());
                        }
                    }

                    /* Add new Material Switcher Track */
                    // Ref to \Engine\Source\Editor\MovieSceneTools\Private\TrackEditors\PrimitiveMaterialTrackEditor.cpp

                    /* One Object Binding may get several Object Components */
                    for (TWeakObjectPtr<> WeakObject : SequencerEditor->FindObjectsInCurrentSequence (Guid)) {
                        const UMeshComponent *MeshComponent = Cast<UMeshComponent> (WeakObject.Get ());
                        if (!IsValid(MeshComponent)) continue;

                        // Find ABC Path
                        TArray<FString> AbcPathArray;

                        // Package path for current component
                        FString PackagePath;

                        if (!GetCurrentMeshComponentPackagePath (WeakObject, PackagePath)) continue;;

                        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Name Finder Get Package path :\n %s ;\n"), *PackagePath);

                        MakeRelativeAbcDirPath (PackagePath, AbcPathArray);

                        bool &&bHasFoundAbcPath = false;
                        bool &&bHasFoundNotFlattenedTrack = false;

                        // try to find flattened abc in current Sequencer Named Directory
                        UObject *MatchedAbcObj;
                        if (HasFoundNotFlattenedClothAbcFile (LevelSequence, AbcPathArray, MatchedAbcObj))
                            bHasFoundNotFlattenedTrack = true;

                        TArray<UMaterialInterface *> &&ComponentMatInterfaces = MeshComponent->GetMaterials ();
                        TArray<FName> &&SlotNames = MeshComponent->GetMaterialSlotNames ();

                        // Hidden Material when slot name same as GeoCache name or material show in not flattened GeoCach
                        for (auto SlotName : SlotNames) {
                            bool &&HasProxyTag = false;

                            if (bHasFoundNotFlattenedTrack) {
                                bHasFoundAbcPath |= true;
                                auto const GeoCache = Cast<UGeometryCache> (MatchedAbcObj);
                                for (auto const GeoCacheMat : GeoCache->Materials) {
                                    if (GeoCacheMat->GetFName () == ComponentMatInterfaces[MeshComponent->GetMaterialIndex (SlotName)]->GetFName ()) {
                                        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Name Finder Get MatchedAbc Mat name : %s;"), *GeoCacheMat->GetName());
                                        HasProxyTag |= true;
                                        break;
                                    }
                                }
                            }
                            else {
                                static FString MatchedPackagePath;
                                for (auto AbcDirPath : AbcPathArray) {
                                    UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Searching @ %s"), *AbcDirPath);
                                    if (AbcDirPath.Find (LevelSequence->GetName ()) < 1) continue;

                                    if (!FPaths::DirectoryExists (AbcDirPath)) continue;

                                    // Make sure SlotNamed Abc Package path is valid, Or Skip
                                    if (HasFoundClothAbcFile (SlotName, AbcDirPath, MatchedPackagePath)) {
                                        HasProxyTag |= true;
                                        bHasFoundAbcPath |= true;
                                        break;
                                    }
                                }
                            }
                            /* If has found Proxy , add to indexArray */
                            if (HasProxyTag) {
                                CreateTrackForMeshElement (MovieScene, SequencerEditor, Guid,
                                    MeshComponent->GetMaterialIndex (SlotName), SlotName, MaterialObject);
                            }
                        }

                        if (!bHasFoundAbcPath) {
                            FText &&Message = FText::Format (
                                FText::FromString ("Can not find any matched Alembic directory path for\n{0}\n"),
                                FText::FromString (PackagePath));

                            FMessageDialog::Open (EAppMsgType::Ok, Message);
                        }

                    } // End Traverse Objects of current guid binding

                }// End Traverse BindingsGuid
            } // End of If valid LevelSequence
        }; // End of Function

        static bool GetCurrentMeshComponentPackagePath (TWeakObjectPtr<> WeakObject, FString &PackagePath)
        {
            bool bCastToMesh = false;
            if (WeakObject->GetClass ()->GetFName () == UStaticMeshComponent::StaticClass ()->GetFName ()) {
                auto const StaticMeshComponent = Cast<UStaticMeshComponent> (WeakObject.Get ());
                UStaticMesh const *CurMesh = StaticMeshComponent->GetStaticMesh ();
                PackagePath = CurMesh->GetPathName ();
                bCastToMesh = true;
            }
            if (WeakObject->GetClass ()->GetFName () == USkeletalMeshComponent::StaticClass ()->GetFName ()) {
                auto const SkeletalMeshComponent = Cast<USkeletalMeshComponent> (WeakObject.Get ());
                auto const *CurMesh = SkeletalMeshComponent->SkeletalMesh;
                PackagePath = CurMesh->GetPathName ();
                bCastToMesh = true;
            }
            return bCastToMesh;
        }

        static void CreateTrackForMeshElement (UMovieScene *MovieScene, const TSharedPtr<ISequencer> &SequencerEditor,
            const FGuid &ObjectBindingID, const int32 &MaterialIndex, const FName &SlotName, UObject *MaterialObject)
        {
            // FScopedTransaction Transaction (LOCTEXT ("CreateTrack", "Create Material Track"));
            MovieScene->Modify ();

            UMovieScenePrimitiveMaterialTrack *NewTrack =
                MovieScene->AddTrack<UMovieScenePrimitiveMaterialTrack> (ObjectBindingID);
            NewTrack->MaterialIndex = MaterialIndex;
            NewTrack->SetDisplayName (FText::Format (LOCTEXT ("MaterialTrackName_Format", "Proxy_{0}"), FText::FromName (SlotName))
            );

            UMovieSceneSection *MovieSceneSection = NewTrack->CreateNewSection ();

            auto const MaterialSection = Cast<UMovieScenePrimitiveMaterialSection> (MovieSceneSection);
            MaterialSection->MaterialChannel.SetDefault (MaterialObject);

            NewTrack->AddSection (*MaterialSection);

            SequencerEditor->NotifyMovieSceneDataChanged (EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
        }

        static void MakeRelativeAbcDirPath (const FString &MatPackagePath, TArray<FString> &AbcPathArray)
        {
            class FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
            {
            protected:
                virtual bool Visit (const TCHAR *FilenameOrDirectory, bool bIsDirectory) override
                {
                    if (bIsDirectory) {
                        FString TempPath;
                        // FPackageName::TryConvertFilenameToLongPackageName (FString (FilenameOrDirectory), TempPath);
                        DirectoryArray.AddUnique (FString (FilenameOrDirectory));
                    }
                    return true;
                }
            public:
                TArray<FString> DirectoryArray;
            };

            AbcPathArray.Empty ();
            // Path @ [CurrentAssetDir]/Animations/Alembic
            FString TempDirPath;
            FPackageName::TryConvertLongPackageNameToFilename (MatPackagePath, TempDirPath);

            //Read from config file
            auto const ConfigSettings = LoadConfig ();
            FString const ConfigAlembicPathRule = 
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicPathRule :
                TEXT ("Animations/Alembic");

            TempDirPath = FPaths::GetPath (TempDirPath) / ConfigAlembicPathRule;
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to found all Alembic Diectory @ %s"), *TempDirPath);
            if (FPaths::DirectoryExists (TempDirPath)) {
                FDirectoryVisitor Visitor;
                FPlatformFileManager::Get ().GetPlatformFile ().IterateDirectory (*TempDirPath, Visitor);
                AbcPathArray = Visitor.DirectoryArray;
            }
        }

        static bool HasFoundNotFlattenedClothAbcFile (const ULevelSequence* const Sequence, const TArray<FString> &AbcDirPathArray, UObject* &MatchedAbcAsset)
        {
            // Read from config file
            auto const ConfigSettings = LoadConfig ();

            // Disable! Skip!
            if (!ConfigSettings->bEnableNotFlattenClothMatch) return false;

            FString const ConfigSubPathRule =
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicClothSubDirMatchKey : TEXT ("Cloth");

            bool const bUseSequencerDirName = ConfigSettings->bUseSequencerDirectoryNameToMatch;

            FName AbcDirMatchToSequencerName;
            if (bUseSequencerDirName) // by Directory name of current sequencer
            {
                AbcDirMatchToSequencerName = FName(FPaths::GetPath(Sequence->GetPathName ()));
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Set Sequencer search name as  %s"), *AbcDirMatchToSequencerName.ToString());
            }else // by current sequencer name
            {
                AbcDirMatchToSequencerName = Sequence->GetFName();
            }

            // Help to find matched Sequencer named dir
            auto HasFoundMatchedSequencerAbcDir = [&](const FString &AbcDirPath) ->bool
        	{
                return AbcDirPath.Find (AbcDirMatchToSequencerName.ToString (),
                    ESearchCase::IgnoreCase,ESearchDir::FromEnd) ? true : false;
            };

            bool &&bFoundAbcAssetStatus = false;

            // Found all .uasset under directory of sequencer name matched
            TArray<FString> MatchedPackagePaths;
            for (auto const AbcDirPath : AbcDirPathArray) {
                MatchedPackagePaths.Empty ();
	            if (!HasFoundMatchedSequencerAbcDir(AbcDirPath)) continue;
                bFoundAbcAssetStatus = true;
                FString &&MatchedDirPath = FPaths::ConvertRelativePathToFull(AbcDirPath / ConfigSubPathRule);
                // Find all package name under current AnimCache Directory
                IFileManager::Get ().FindFiles (MatchedPackagePaths, *MatchedDirPath, TEXT ("uasset"));

                // NOTE: make fully path
                for (int PathId = 0; PathId < MatchedPackagePaths.Num (); ++PathId) {
                    //PackageName = FPaths::ConvertRelativePathToFull (MatchedDirPath, PackageName);
                    FPackageName::TryConvertFilenameToLongPackageName ((MatchedDirPath / MatchedPackagePaths[PathId]), MatchedPackagePaths[PathId]);
                }

                bFoundAbcAssetStatus |= MatchedPackagePaths.Num () > 0 ? true : false;
                break;
            }

            if (!bFoundAbcAssetStatus) return false;

            bFoundAbcAssetStatus = false;
            for (auto const MatchedPackagePath : MatchedPackagePaths) {
                // Load Asset to UObject to modify
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Check Package @ %s"), *MatchedPackagePath);
                MatchedAbcAsset = MinePackageLoadHelper::LoadAsset (MatchedPackagePath);
                if (!IsValid (MatchedAbcAsset)) continue;

                // Check if abc type
                auto const GeoCache = Cast<UGeometryCache> (MatchedAbcAsset);
                if (GeoCache == nullptr || GeoCache->StaticClass ()->GetFName () != UGeometryCache::StaticClass ()->GetFName ()) {
                    UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s is not valid ABC GeometryCache, SKIP\n"), *MatchedAbcAsset->GetFullName ());
                    continue;
                }

                // NOTE: Get Current Geometry Materials && Tracks
                TArray<UGeometryCacheTrack *> GeoCacheTracks = GeoCache->Tracks;
                if (GeoCacheTracks.Num()<1) continue;
                UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Found Matched Unflattened Abc Package @ %s"), *MatchedPackagePath);
                bFoundAbcAssetStatus = true;
            }

            // Return status
            return bFoundAbcAssetStatus;
        }

        static bool HasFoundClothAbcFile (const FName &MatSlotName, const FString &AbcDirPath, FString &MatchedPackagePath)
        {
            //Read from config file
            auto const ConfigSettings = LoadConfig ();
            FString const ConfigSubPathRule = 
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicClothSubDirMatchKey : TEXT ("Cloth");

            MatchedPackagePath = FPaths::ConvertRelativePathToFull (AbcDirPath / ConfigSubPathRule, MatSlotName.ToString ());
            FPackageName::TryConvertFilenameToLongPackageName (MatchedPackagePath, MatchedPackagePath);
            // UE_LOG (LogMineCustomToolEditor, Log, TEXT ("Try to found Alembic @ %s"), *MatchedPackagePath);
            if (FPackageName::DoesPackageExist (MatchedPackagePath)) {
                return true;
            }

            return false;
        }

        static bool LoadHiddenProxyMat (UObject *&ProxyMaterial)
        {
            /* Path to Proxy material */
            // TODO: Read from Config
            auto const ConfigSettings = LoadConfig ();

            static FString const ProxyMatPath = 
                ConfigSettings->bUseCustomProxyConfig ?
                ConfigSettings->ConfigAlembicProxyMatPath :
                TEXT ("/Game/PalTrailer/MaterialLibrary/Base/Charactor/CFX_Material/Mat_Daili_Inst");

            /* Fallback Proxy Material Engine Path */
            static FString const DefaultWorldMatPath = TEXT ("/Engine/EngineMaterials/WorldGridMaterial");

            /* Load Mat to Object */
            ProxyMaterial = MinePackageLoadHelper::LoadAsset (ProxyMatPath);

            if (!IsValid (ProxyMaterial)) {
                /* Load WorldDefault Material to replace */
                UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to Load Default World Material;\n"));
                ProxyMaterial = MinePackageLoadHelper::LoadAsset (DefaultWorldMatPath);
                if (!IsValid (ProxyMaterial)) {
                    UE_LOG (LogMineCustomToolEditor, Error, TEXT ("Cant Load Default World Material !!!;\n"));
                    ProxyMaterial = nullptr;
                    return false;
                }
            }
            return true;
        }

    };

}

// UI Command Class
namespace FMineSequencerBaseMenuAction_Internal
{
    using namespace FMineSequencerBaseMenuAction_Helper_Internal;

    struct FMineSequencerAction_CommandsInfo_Base
    {
        TArray<TSharedPtr<FUICommandInfo>> UICommandInfoArray;
    };

    class FMineSequencerBaseMenuAction_CommandsInfo :
        public TCommands<FMineSequencerBaseMenuAction_CommandsInfo>, public FMineSequencerAction_CommandsInfo_Base
    {
    #define CMD_INFO_CTX_NAME "FMineSequencerBaseMenuAction"
    public:

        /* INIT */
        FMineSequencerBaseMenuAction_CommandsInfo ()
            : TCommands<FMineSequencerBaseMenuAction_CommandsInfo> (
                TEXT (CMD_INFO_CTX_NAME), // Context name for fast lookup
                FText::FromString(TEXT (CMD_INFO_CTX_NAME)), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        virtual void RegisterCommands () override
        {
            /**
             * @brief: Add UI_COMMAND fastly
             * @param ID : Int
             * @param NAME : "TEXT"
             * @param TIP : "TEXT"
             */
            #define ADD_UI_COMMAND_INFO(ID,NAME,TIP) \
                TSharedPtr<FUICommandInfo> UICommandInfo_0;\
                UI_COMMAND (UICommandInfo_##ID,NAME,TIP, \
                    EUserInterfaceActionType::Button, FInputChord ()); \
                UICommandInfoArray.Emplace (UICommandInfo_##ID)

            // 0
            ADD_UI_COMMAND_INFO (0, "Auto Bind Hidden Mat", "Try to set Hidden Material for current selection in sequencer editor");

            #undef ADD_UI_COMMAND_INFO
        }

        // Mapping FUICommandList
        static void MapCommands (const TSharedPtr<FUICommandList> &CommandList)
        {
            auto BaseMenuAction_CommandInfo = Get ();

            #define BIND_UI_COMMAND_TO_SLOT(ID,FUNC) \
                CommandList->MapAction ( BaseMenuAction_CommandInfo.UICommandInfoArray[##ID##], \
                    FExecuteAction::CreateStatic (&##FUNC##),FCanExecuteAction () \
                );

            // 0
            BIND_UI_COMMAND_TO_SLOT (0, FMineSequencerAction_SetHiddenProxyMatKey::DoProcess);

            #undef BIND_UI_COMMAND_TO_SLOT
        }

    #undef CMD_INFO_CTX_NAME
    };
}

// Extension Builder
namespace FMineSequencerBaseMenuAction_Internal
{
    // ToolBar Extension
    template<typename TCmdInfo>
    class FMineSequenceBaseBarActionExtension
    {
    public:
        static TSharedPtr<FExtender> CreateFExtender ()
        {
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Bar Extender"));
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            TCmdInfo::MapCommands (CommandList);
            TSharedPtr<FExtender> MenuExtender = MakeShareable (new FExtender ());

            MenuExtender->AddToolBarExtension (
                TEXT ("Curve Editor"),
                EExtensionHook::Before,
                CommandList,
                FToolBarExtensionDelegate::CreateStatic (&FMineSequenceBaseBarActionExtension::CreateBarActionsMenu)
            );

            return MenuExtender;
        }

        static void CreateBarActionsMenu (
            FToolBarBuilder &BarBuilder
        )
        {
            auto BaseMenuAction_CommandInfo = TCmdInfo::Get ();
            UE_LOG (LogMineCustomToolEditor, Error, TEXT ("%s"), TEXT ("Debug: Create Sequencer Bar Button"));
            BarBuilder.BeginSection ("MineSequencerToolBar");
            //for (int CommandId = 0; CommandId < BaseMenuAction_CommandInfo.UICommandInfoArray.Num (); ++CommandId)
            //{
            //    
            //}
            BarBuilder.AddToolBarButton (BaseMenuAction_CommandInfo.UICommandInfoArray[0],
                NAME_None, TAttribute<FText> (), TAttribute<FText> (),
                FSlateIcon (FEditorStyle::GetStyleSetName (), "Matinee.ToggleCurveEditor")
            );
            BarBuilder.EndSection ();
            BarBuilder.AddSeparator ();
        }
    };

    // Context Extension
    template<typename TCmdInfo>
    class FMineSequenceBaseCtxMenuActionExtension
    {

    public:
        FMineSequenceBaseCtxMenuActionExtension () {};
        virtual ~FMineSequenceBaseCtxMenuActionExtension () {};

        static TSharedPtr<FExtender> CreateFExtender ()
        {
            TSharedPtr<FUICommandList> const CommandList = MakeShareable (new FUICommandList);
            TCmdInfo::MapCommands (CommandList);
            TSharedPtr<FExtender> MenuExtender = MakeShareable (new FExtender ());
            MenuExtender->AddMenuExtension (
                TEXT ("Edit"),
                EExtensionHook::Before,
                CommandList,
                FMenuExtensionDelegate::CreateStatic (&FMineSequenceBaseCtxMenuActionExtension::CreateActionsSubMenu)
            );
            return MenuExtender;
        }

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuToolName", "Mine Sub Actions");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Add Specific Material key to Current Object.");
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsMainMenu"), LOCTEXT ("MineSequencerBaseActionsMainMenu", "Mine Sequencer Actions"));
            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (&PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineSequencerBaseActions"))
            );
            MenuBuilder.EndSection ();
        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            auto BaseMenuAction_CommandInfo = TCmdInfo::Get ();
            MenuBuilder.BeginSection (TEXT ("MineSequencerBaseActionsSubMenu"), LOCTEXT ("MineSequencerBaseActionsSubMenu", "SequencerBaseAction"));

            // Add CommandsInfo to Menus
            for (int CommandId =0; CommandId < BaseMenuAction_CommandInfo.UICommandInfoArray.Num(); ++CommandId)
            {
                MenuBuilder.AddMenuEntry (BaseMenuAction_CommandInfo.UICommandInfoArray[CommandId]);
            }

            MenuBuilder.EndSection ();
        }
    };
}

// Module Loader
namespace FMineSequencerBaseMenuAction_Internal
{
    void FMineSequencerBaseExtensionLoader::OnStartupModule ()
    {

        UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("%s"), TEXT("Install Sequencer Menu Hooks"));

        // Register Command
        FMineSequencerBaseMenuAction_CommandsInfo::Register ();

        // Add Base Ctx Action Extender
        TSharedPtr<FExtender> const BaseCtxActionExt = FMineSequenceBaseCtxMenuActionExtension<FMineSequencerBaseMenuAction_CommandsInfo>::CreateFExtender();
        CtxExtenderPtrArray.Emplace (BaseCtxActionExt);

        // Add Base Ctx Action Extender
        TSharedPtr<FExtender> const BaseBarActionExt = FMineSequenceBaseBarActionExtension<FMineSequencerBaseMenuAction_CommandsInfo>::CreateFExtender();
        BarExtenderPtrArray.Emplace (BaseBarActionExt);
        

        // Add All Extender to manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray) {
            ObjBindCtxExtensibilityManager->AddExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->AddExtender (ExtenderPtr);
        }

    }

    void FMineSequencerBaseExtensionLoader::OnShutdownModule ()
    {
        // Remove All Extender from manager
        for (auto const ExtenderPtr : CtxExtenderPtrArray) {
            ObjBindCtxExtensibilityManager->RemoveExtender (ExtenderPtr);
        }
        for (auto const ExtenderPtr : BarExtenderPtrArray) {
            ToolBarExtensibilityManager->RemoveExtender (ExtenderPtr);
        }

    }
}

#undef LOCTEXT_NAMESPACE