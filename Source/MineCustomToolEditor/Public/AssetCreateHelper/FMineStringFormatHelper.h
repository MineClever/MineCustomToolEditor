#pragma once
#include "CoreMinimal.h"

namespace MineFormatStringInternal
{
    namespace FormattedFStringHelper
    {
        template <typename T>
        FORCEINLINE static void AddToFormatArgArray (TArray<FStringFormatArg> &FormatArray, const T &CurArg)
        {
            FormatArray.Add (FStringFormatArg (CurArg));
        }

        template <typename T, typename ...ArgsT>
        FORCEINLINE static void AddToFormatArgArray (TArray<FStringFormatArg> &FormatArray, const T &CurArg, const ArgsT& ... ArgsString)
        {
            FormatArray.Add (FStringFormatArg (CurArg));
            AddToFormatArgArray (FormatArray, ArgsString ...);
        }

        /**
         * @brief : FormattedFString(TEXT("String {0}, {1}"), TEXT("First"), FString_Second )
         * @tparam ArgsT : args package ...
         * @param BaseString : TCHAR*
         * @param ArgsString : Could be Stringing, Numeric
         * @return : FString with formatted
         */
        template <typename ...ArgsT>
        FORCEINLINE static FString FormattedFString (const TCHAR *BaseString, const ArgsT& ... ArgsString)
        {
            TArray<FStringFormatArg> FormatArray;
            AddToFormatArgArray (FormatArray, ArgsString...);
            return FString::Format (BaseString, FormatArray);
        }

    };

    struct FNsLocTextDescriptions
    {
        TCHAR *Key;
        TCHAR *KeyDescription;
        TCHAR *LocTextNameSpace;
    };

    /**
     * @brief : Replace NSLOCTEXT Macro
     * @param Descriptions : Struct to Format
     * @return :FText
     */
    FORCEINLINE static FText NsLocText (const FNsLocTextDescriptions &Descriptions)
    {
        return
            FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText (
                Descriptions.KeyDescription,
                Descriptions.LocTextNameSpace,
                Descriptions.Key
            );
    }
}
