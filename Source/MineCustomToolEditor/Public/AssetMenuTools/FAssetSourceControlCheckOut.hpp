#pragma once
#include "CoreMinimal.h"
#include "MineMouduleDefine.h"
// Source Control
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "FAssetSourceControlCheckOut"



namespace SourceControlHelpersInternal
{

	/*
	 * Status info set by LogError() and USourceControlHelpers methods if an error occurs
	 * regardless whether their bSilent is set or not.
	 * Should be empty if there is was no error.
	 * @see	USourceControlHelpers::LastErrorMsg(), LogError()
	 */
	FText LastErrorText;

	/* Store error and write to Log if bSilent is false. */
	inline void LogError (const FText &ErrorText, bool bSilent)
	{
		LastErrorText = ErrorText;

		if (!bSilent) {
			FMessageLog ("SourceControl").Error (LastErrorText);
		}
	}

	/* Return provider if ready to go, else return nullptr. */
	ISourceControlProvider *VerifySourceControl (bool bSilent)
	{
		ISourceControlModule const&SCModule = ISourceControlModule::Get ();

		if (!SCModule.IsEnabled ()) {
			LogError (LOCTEXT ("SourceControlDisabled", "Source control is not enabled."), bSilent);

			return nullptr;
		}

		ISourceControlProvider *Provider = &SCModule.GetProvider ();

		if (!Provider->IsAvailable ()) {
			LogError (LOCTEXT ("SourceControlServerUnavailable", "Source control server is currently not available."), bSilent);

			return nullptr;
		}

		// Clear the last error text if there hasn't been an error (yet).
		LastErrorText = FText::GetEmpty ();

		return Provider;
	}


	/*
	 * Converts specified file to fully qualified file path that is compatible with source control.
	 *
	 * @param	InFile		File string - can be either fully qualified path, relative path, long package name, asset path or export text path (often stored on clipboard)
	 * @param	bSilent		if false then write out any error info to the Log. Any error text can be retrieved by LastErrorMsg() regardless.
	 * @return	Fully qualified file path to use with source control or "" if conversion unsuccessful.
	 */
	FString ConvertFileToQualifiedPath (const FString &InFile, bool bSilent, bool bAllowDirectories = false, const TCHAR *AssociatedExtension = nullptr)
	{
		// Converted to qualified file path
		FString SCFile;

		if (InFile.IsEmpty ()) {
			LogError (LOCTEXT ("UnspecifiedFile", "File not specified"), bSilent);

			return SCFile;
		}

		// Try to determine if file is one of:
		// - fully qualified path
		// - relative path
		// - long package name
		// - asset path
		// - export text path (often stored on clipboard)
		//
		// For example:
		// - D:\Epic\Dev-Ent\Projects\Python3rdBP\Content\Mannequin\Animations\ThirdPersonIdle.uasset
		// - Content\Mannequin\Animations\ThirdPersonIdle.uasset
		// - /Game/Mannequin/Animations/ThirdPersonIdle
		// - /Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle
		// - AnimSequence'/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle'

		SCFile = InFile;

		// Is ExportTextPath (often stored in Clipboard) form?
		//  - i.e. AnimSequence'/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle'
		if (SCFile[SCFile.Len () - 1] == '\'') {
			SCFile = FPackageName::ExportTextPathToObjectPath (SCFile);
		}

		// Package paths
		if (SCFile[0] == TEXT ('/') && FPackageName::IsValidLongPackageName (SCFile, /*bIncludeReadOnlyRoots*/false))//	if (SCFile[0] == TEXT('/') && FPackageName::IsValidPath(SCFile))
		{
			// Assume it is a package
			FPackageName::TryConvertFilenameToLongPackageName (SCFile, SCFile);
			bool bPackage = true;
			
			// Try to get filename by finding it on disk
			if (!FPackageName::DoesPackageExist (SCFile, nullptr, &SCFile)) {
				// First do the conversion without any extension set, as this will allow us to test whether the path represents an existing directory rather than an asset
				if (FPackageName::TryConvertLongPackageNameToFilename (SCFile, SCFile)) {
					if (bAllowDirectories && FPaths::DirectoryExists (SCFile)) {
						// This path mapped to a known directory, so ensure it ends in a slash
						SCFile /= FString ();
					}
					else if (AssociatedExtension) {
						// Just use the requested extension
						SCFile += AssociatedExtension;
					}
					else {
						// The package does not exist on disk, see if we can find it in memory and predict the file extension
						UPackage *Package = FindPackage (nullptr, *SCFile);
						SCFile += (Package && Package->ContainsMap () ? FPackageName::GetMapPackageExtension () : FPackageName::GetAssetPackageExtension ());
					}
				}
				else {
					bPackage = false;
				}
			}

			if (bPackage) {
				SCFile = FPaths::ConvertRelativePathToFull (SCFile);
				return SCFile;
			}
		}

		// Assume it is a qualified or relative file path

		// Could normalize it
		FPaths::NormalizeFilename(SCFile);

		if (!FPaths::IsRelative (SCFile)) {
			return SCFile;
		}

		// Qualify based on process base directory.
		// Something akin to "C:/Epic/UE4/Engine/Binaries/Win64/" as a current path.
		SCFile = FPaths::ConvertRelativePathToFull (SCFile);

		//if (SCFile.StartsWith(TEXT ("/Game/"), ESearchCase::IgnoreCase))
		//{
		//	SCFile.ReplaceInline (TEXT ("/Game/"), TEXT (""), ESearchCase::IgnoreCase);
		//}

		if (FPaths::FileExists (SCFile) || (bAllowDirectories && FPaths::DirectoryExists (SCFile))) {
			return SCFile;
		}

		// Qualify based on project directory.
		SCFile = FPaths::ConvertRelativePathToFull (FPaths::ConvertRelativePathToFull (FPaths::ProjectDir ()), SCFile);

		if (FPaths::FileExists (SCFile) || (bAllowDirectories && FPaths::DirectoryExists (SCFile))) {
			return SCFile;
		}

		// Qualify based on Engine directory
		SCFile = FPaths::ConvertRelativePathToFull (FPaths::ConvertRelativePathToFull (FPaths::EngineDir ()), SCFile);

		return SCFile;
	}


	/**
	 * Converts specified files to fully qualified file paths that are compatible with source control.
	 *
	 * @param	InFiles			File strings - can be either fully qualified path, relative path, long package name, asset name or export text path (often stored on clipboard)
	 * @param	OutFilePaths	Fully qualified file paths to use with source control or "" if conversion unsuccessful.
	 * @param	bSilent			if false then write out any error info to the Log. Any error text can be retrieved by LastErrorMsg() regardless.
	 * @return	true if all files successfully converted, false if any had errors
	 */
	bool ConvertFilesToQualifiedPaths (const TArray<FString> &InFiles, TArray<FString> &OutFilePaths, bool bSilent, bool bAllowDirectories = false)
	{
		uint32 SkipNum = 0u;

		for (const FString &File : InFiles) {

			FString SCFile = ConvertFileToQualifiedPath (File, bSilent, bAllowDirectories);
			// UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("File path was converted to : %s"), *SCFile);
			if (SCFile.IsEmpty ()) {
				SkipNum++;
			}
			else {
				OutFilePaths.Add (MoveTemp (SCFile));
			}
		}

		if (SkipNum) {
			FFormatNamedArguments Arguments;
			Arguments.Add (TEXT ("SkipNum"), FText::AsNumber (SkipNum));
			LogError (FText::Format (LOCTEXT ("FilesSkipped", "During conversion to qualified file paths, {SkipNum} files were skipped!"), Arguments), bSilent);

			return false;
		}

		return true;
	}

}  // namespace SourceControlHelpersInternal



class FAssetSourceControlHelper
{
public:
    static ISourceControlModule& SourceControlModule ;

public:

    static FString GetCurrentProviderFName ()
    {
        if (IsSourceControlAvailable())
        {
            return SourceControlModule.GetProvider().GetName ().ToString();
        } else
        {
            return FString(TEXT("None"));
        }
    }

    static bool IsSourceControlAvailable ()
    {
        return SourceControlModule.IsEnabled () && SourceControlModule.GetProvider ().IsAvailable ();
    }

    /**
     * @brief Use currently set source control provider to check out specified files.Wrap for silent mode.
     * @note USE PACKAGE NAME !
     * @param Files Files to check out
     * @return true if succeeded
     */
    static bool CheckOutFiles (const TArray<FString> & Files)
    {
        const bool bCanCheckOut = IsSourceControlAvailable ();
        if (bCanCheckOut) {
            // UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to CheckOut %d files"), Files.Num ());
            const bool &&IsChecked = CheckOutOrAddFiles (Files, false,false);
            return IsChecked;
        }
        else return bCanCheckOut;
    }

	static bool CheckOutFile (const FString &File)
    {
		TArray<FString> const TempStringArray = {File};
		return CheckOutFiles (TempStringArray);
    }

	static bool CheckOutOrAddFiles (const TArray<FString> &InFiles, bool bSilent = false, bool bAdd = true)
	{
		// Determine file type and ensure it is in form source control wants
		// Even if some files were skipped, still apply to the others

		TArray<FString> SCFiles;
		bool const bFilesSkipped = !SourceControlHelpersInternal::ConvertFilesToQualifiedPaths (InFiles, SCFiles, bSilent);
		const int32 NumFiles = SCFiles.Num ();

		// Ensure source control system is up and running
		ISourceControlProvider *Provider = SourceControlHelpersInternal::VerifySourceControl (bSilent);

		// UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("current Provider : %s\n"), *(Provider->GetName().ToString()));

		if (!Provider) {
			// Error or can't communicate with source control
			return false;
		}

		TArray<FSourceControlStateRef> SCStates;
		Provider->GetState (SCFiles, SCStates, EStateCacheUsage::ForceUpdate);

		TArray<FString> SCFilesToAdd;
		TArray<FString> SCFilesToCheckout;
		bool bCannotAddAtLeastOneFile = false;
		bool bCannotCheckoutAtLeastOneFile = false;
		for (int32 Index = 0; Index < NumFiles; ++Index) {
			FString SCFile = SCFiles[Index];
			FSourceControlStateRef SCState = SCStates[Index];

			// Less error checking and info is made for multiple files than the single file version.
			// This multi-file version could be made similarly more sophisticated.

			if (!SCState->IsCheckedOut()) { // UncheckedOut
				if (!SCState->IsAdded ()) { // if not mark added
					if (SCState->CanAdd () && bAdd) {
						//UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Mark for add file : %s"), *SCFile);
						SCFilesToAdd.Add (SCFile);
					}
					else {
						bCannotAddAtLeastOneFile = bAdd?true:false;
					}

					if (SCState->CanCheckout ())
					{
						SCFilesToCheckout.Add (SCFile);
					} else // check out unsafely !
					{
						if (!SCState->IsConflicted () && SCState->IsSourceControlled ()) {
							if (!SCState->IsCheckedOutOther ()) {
								SCFilesToCheckout.Add (SCFile);
							}
							else {
								bCannotCheckoutAtLeastOneFile = true;
							}
						}
					}
				}
			}else
			{
				TArray<FStringFormatArg> FormatArray;
				FormatArray.Add (FStringFormatArg (SCFile));
				FText ErrorLog = FText::FromString(FString::Format(TEXT ("file has been already checked out : {0}"), FormatArray));
				SourceControlHelpersInternal::LogError (ErrorLog,true);
			}
		}

		bool bSuccess = !bFilesSkipped && !bCannotCheckoutAtLeastOneFile && !bCannotAddAtLeastOneFile;

		if (SCFilesToAdd.Num ()) {
			bSuccess &= Provider->Execute (ISourceControlOperation::Create<FMarkForAdd> (), SCFilesToAdd) == ECommandResult::Succeeded;
		}

		if (SCFilesToCheckout.Num ()) {
			bSuccess &= Provider->Execute (ISourceControlOperation::Create<FCheckOut> (), SCFilesToCheckout) == ECommandResult::Succeeded;
		}

		return bSuccess;
	}

	static bool MarkFileForDelete (const FString &InFile, bool bSilent = false)
	{
		// Determine file type and ensure it is in form source control wants
		FString SCFile = SourceControlHelpersInternal::ConvertFileToQualifiedPath (InFile, bSilent);

		if (SCFile.IsEmpty ()) {
			return false;
		}

		// Ensure source control system is up and running
		ISourceControlProvider *Provider = SourceControlHelpersInternal::VerifySourceControl (bSilent);

		if (!Provider) {
			// Error or can't communicate with source control
			// Could erase it anyway, though keeping it for now.
			return false;
		}

		FSourceControlStatePtr SCState = Provider->GetState (SCFile, EStateCacheUsage::ForceUpdate);

		if (!SCState.IsValid ()) {
			// Improper or invalid SCC state
			FFormatNamedArguments Arguments;
			Arguments.Add (TEXT ("InFile"), FText::FromString (InFile));
			Arguments.Add (TEXT ("SCFile"), FText::FromString (SCFile));
			SourceControlHelpersInternal::LogError (FText::Format (LOCTEXT ("CouldNotDetermineState", "Could not determine source control state of file '{InFile}' ({SCFile})."), Arguments), bSilent);

			return false;
		}

		bool bDelete = false;

		if (SCState->IsSourceControlled ()) {
			bool bAdded = SCState->IsAdded ();

			if (bAdded || SCState->IsCheckedOut ()) {
				if (Provider->Execute (ISourceControlOperation::Create<FRevert> (), SCFile) != ECommandResult::Succeeded) {
					FFormatNamedArguments Arguments;
					Arguments.Add (TEXT ("InFile"), FText::FromString (InFile));
					Arguments.Add (TEXT ("SCFile"), FText::FromString (SCFile));
					SourceControlHelpersInternal::LogError (FText::Format (LOCTEXT ("CouldNotRevert", "Could not revert source control state of file '{InFile}' ({SCFile})."), Arguments), bSilent);

					return false;
				}
			}

			if (!bAdded) {
				// Was previously added to source control so mark it for delete
				if (Provider->Execute (ISourceControlOperation::Create<FDelete> (), SCFile) != ECommandResult::Succeeded) {
					FFormatNamedArguments Arguments;
					Arguments.Add (TEXT ("InFile"), FText::FromString (InFile));
					Arguments.Add (TEXT ("SCFile"), FText::FromString (SCFile));
					SourceControlHelpersInternal::LogError (FText::Format (LOCTEXT ("CouldNotDelete", "Could not delete file '{InFile}' from source control ({SCFile})."), Arguments), bSilent);

					return false;
				}
			}
		}

		// Delete file if it still exists
		IFileManager &FileManager = IFileManager::Get ();

		if (FileManager.FileExists (*SCFile)) {
			// Just a regular file not tracked by source control so erase it.
			// Don't bother checking if it exists since Delete doesn't care.
			return FileManager.Delete (*SCFile, false, true);
		}

		return false;
	}

	static bool MarkFilesForDelete (const TArray<FString> &InFiles, bool bSilent = false)
	{
		// Determine file type and ensure it is in form source control wants
		// Even if some files were skipped, still apply to the others
		TArray<FString> SCFiles;
		bool bFilesSkipped = !SourceControlHelpersInternal::ConvertFilesToQualifiedPaths (InFiles, SCFiles, bSilent);
		const int32 NumFiles = SCFiles.Num ();

		// Ensure source control system is up and running
		ISourceControlProvider *Provider = SourceControlHelpersInternal::VerifySourceControl (bSilent);
		if (!Provider) {
			// Error or can't communicate with source control
			// Could erase the files anyway, though keeping them for now.
			return false;
		}

		TArray<FSourceControlStateRef> SCStates;
		Provider->GetState (SCFiles, SCStates, EStateCacheUsage::ForceUpdate);

		TArray<FString> SCFilesToRevert;
		TArray<FString> SCFilesToMarkForDelete;
		bool bCannotDeleteAtLeastOneFile = false;
		for (int32 Index = 0; Index < NumFiles; ++Index) {
			FString SCFile = SCFiles[Index];
			FSourceControlStateRef SCState = SCStates[Index];

			// Less error checking and info is made for multiple files than the single file version.
			// This multi-file version could be made similarly more sophisticated.
			if (SCState->IsSourceControlled ()) {
				//Force to read-only to fix p4 elder version bug
				GetLowLevel ().SetReadOnly (*SCFile, true);

				bool bAdded = SCState->IsAdded ();
				if (bAdded || SCState->IsCheckedOut ()) {
					SCFilesToRevert.Add (SCFile);
				}

				if (!bAdded) {
					if (SCState->CanDelete ()) {
						SCFilesToMarkForDelete.Add (SCFile);
					}
					else {
						bCannotDeleteAtLeastOneFile = true;
					}
				}
			}
		}

		bool bSuccess = !bFilesSkipped && !bCannotDeleteAtLeastOneFile;
		if (SCFilesToRevert.Num ()) {
			bSuccess &= Provider->Execute (ISourceControlOperation::Create<FRevert> (), SCFilesToRevert) == ECommandResult::Succeeded;
		}

		if (SCFilesToMarkForDelete.Num ()) {
			// Must check out to delete
			Provider->Execute (ISourceControlOperation::Create<FCheckOut> (), SCFilesToMarkForDelete);
			bSuccess &= Provider->Execute (ISourceControlOperation::Create<FDelete> (), SCFilesToMarkForDelete) == ECommandResult::Succeeded;
		}

		// Delete remaining files if they still exist : 
		IFileManager &FileManager = IFileManager::Get ();
		TArray<FString> FileFallback;
		for (FString SCFile : SCFiles) {
			if (FileManager.FileExists (*SCFile)) {
				// Just a regular file not tracked by source control so erase it.
				// Don't bother checking if it exists since Delete doesn't care.
				bool const bDeleted = FileManager.Delete (*SCFile, false, true);
				if (!bDeleted) {
					GetLowLevel ().SetReadOnly (*SCFile, true);
				}// recovery file , when falla
				bSuccess &= bDeleted;
			}
		}


		return bSuccess;
	}

	// instead of caching the LowLevel, we call the singleton each time to never be incorrect
	FORCEINLINE static IPlatformFile &GetLowLevel ()
	{
		return FPlatformFileManager::Get ().GetPlatformFile ();
	}


};



ISourceControlModule& FAssetSourceControlHelper::SourceControlModule = ISourceControlModule::Get ();


#undef LOCTEXT_NAMESPACE