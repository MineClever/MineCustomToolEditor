#pragma once
#include "CoreMinimal.h"
#include "MineMouduleDefine.h"
// Source Control
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "ISourceControlProvider.h"
//#include "SourceControlHelpers.h"
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
		//FPaths::NormalizeFilename(SCFile);

		if (!FPaths::IsRelative (SCFile)) {
			return SCFile;
		}

		// Qualify based on process base directory.
		// Something akin to "C:/Epic/UE4/Engine/Binaries/Win64/" as a current path.
		SCFile = FPaths::ConvertRelativePathToFull (InFile);

		if (FPaths::FileExists (SCFile) || (bAllowDirectories && FPaths::DirectoryExists (SCFile))) {
			return SCFile;
		}

		// Qualify based on project directory.
		SCFile = FPaths::ConvertRelativePathToFull (FPaths::ConvertRelativePathToFull (FPaths::ProjectDir ()), InFile);

		if (FPaths::FileExists (SCFile) || (bAllowDirectories && FPaths::DirectoryExists (SCFile))) {
			return SCFile;
		}

		// Qualify based on Engine directory
		SCFile = FPaths::ConvertRelativePathToFull (FPaths::ConvertRelativePathToFull (FPaths::EngineDir ()), InFile);

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
			UE_LOG (LogMineCustomToolEditor, Log, TEXT ("current input file path is :%"), *File);
			FString SCFile = ConvertFileToQualifiedPath (File, bSilent, bAllowDirectories);

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
     * @brief Use currently set source control provider to check out specified files.Wrap for silent mode
     * @param Files Files to check out
     * @return true if succeeded
     */
    static bool CheckOutFiles (const TArray<FString> & Files)
    {
        const bool bCheckOut = IsSourceControlAvailable ();
        if (bCheckOut) {
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Try to CheckOut %d files"), Files.Num ());
            const bool &&IsChecked = CheckOutOrAddFiles (Files, false);
            FString const TempBoolString (IsChecked?"True":"False");
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Is CheckOut all files? %s"), *TempBoolString);
            return IsChecked;
        }
        else return bCheckOut;

    }

	static bool CheckOutOrAddFiles (const TArray<FString> &InFiles, bool bSilent)
	{
		// Determine file type and ensure it is in form source control wants
		// Even if some files were skipped, still apply to the others

		TArray<FString> SCFiles;
		bool const bFilesSkipped = !SourceControlHelpersInternal::ConvertFilesToQualifiedPaths (InFiles, SCFiles, bSilent);
		const int32 NumFiles = SCFiles.Num ();

		// Ensure source control system is up and running
		ISourceControlProvider *Provider = SourceControlHelpersInternal::VerifySourceControl (bSilent);

		UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("current Provider : %s\n"), *(Provider->GetName().ToString()));

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
			UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("current file : %s\n"), *SCFile);
			// Less error checking and info is made for multiple files than the single file version.
			// This multi-file version could be made similarly more sophisticated.

			if (!SCState->IsCheckedOut()) { // UncheckedOut
				if (!SCState->IsAdded ()) { // if not mark added
					if (SCState->CanAdd ()) {
						UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Mark for add file : %s"), *SCFile);
						SCFilesToAdd.Add (SCFile);
					}
					else {
						bCannotAddAtLeastOneFile = true;
					}

//#pragma region TEST_CHECKER_STATE
//					FString TempStringToTest = TEXT ("False");
//					if (SCState->CanCheckout ())
//						TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("CanCheckout %s"), *TempStringToTest);
//					TempStringToTest = TEXT ("False");
//					if (!SCState->IsConflicted ())
//						TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("!IsConflicted %s"), *TempStringToTest);
//					TempStringToTest = TEXT ("False");
//					if (SCState->IsSourceControlled ())
//						TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("IsSourceControlled %s"), *TempStringToTest);
//					TempStringToTest = TEXT ("False");
//					if (!SCState->IsCurrent ())
//						TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("IsCurrent %s"), *TempStringToTest);
//					TempStringToTest = TEXT ("False");
//					if (SCState->IsUnknown ())
//						TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("IsUnknown %s"), *TempStringToTest);
//					TempStringToTest = TEXT ("False");
//					if (
//						(!SCState->IsConflicted ()) &&
//						!SCState->IsCheckedOutOther ()
//						) TempStringToTest = TEXT ("True");
//					UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("unSafe Checked Out %s"), *TempStringToTest);
//#pragma endregion TEST_CHECKER_STATE


					if (!SCState->IsConflicted ())
					{
						UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("UnknowFile, Unsafe Check Out : %s"), *SCFile);
						if (!SCState->IsCheckedOutOther ())
						{
							SCFilesToCheckout.Add (SCFile);
						}
						else {
							bCannotCheckoutAtLeastOneFile = true;
						}
					}
				}
				else { // if mark added
					if (SCState->CanCheckout ()) {
						SCFilesToCheckout.Add (SCFile);
					}
					else {
						bCannotCheckoutAtLeastOneFile = true;
					}
				}
			}else
			{
				UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("file has been already checked out : %s"), *SCFile);
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
};



ISourceControlModule& FAssetSourceControlHelper::SourceControlModule = ISourceControlModule::Get ();


#undef LOCTEXT_NAMESPACE