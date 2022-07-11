#pragma once
#include <stdarg.h>
#include "CoreMinimal.h"

namespace MineFormatStringInternal
{
    namespace FormattedFStringHelper
    {
        /**
         * @brief :use as FormattedFString(TEXT("Example String {0} {1}"), 2, FString_0, FString_1)
         * @param BaseString : TCHAR *
         * @param ArgsCount : uint8
         * @param ... : FString
         * @return : FString
         */
        FORCEINLINE static FString FormattedFStringTypeWithCount (const TCHAR *BaseString, uint8 ArgsCount, ...)
        {
            TArray<FStringFormatArg> FormatArray;

            va_list Args;
            va_start (Args, ArgsCount);

            for (int i = 0; i < ArgsCount; ++i) {
                FString const CurString = va_arg (Args, FString);
                FormatArray.Add (FStringFormatArg (CurString));
            }
            va_end (Args);

            return FString::Format (BaseString, FormatArray);
        }

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
