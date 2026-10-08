#pragma once
#include "CoreMinimal.h"

// Keyboard-only sequence, with a per-letter timeout. Ordinary movement keys
// remain untouched until the distinctive "si" prefix has been entered.
struct FPOCDeveloperCode
{
    int32 Matched = 0;
    double LastLetter = 0;
    bool Push(TCHAR Letter, double Now, bool& Consume)
    {
        static const FString Code(TEXT("siddarthisgod"));
        if (Now - LastLetter > 3.0) Matched = 0;
        Letter = FChar::ToLower(Letter);
        Matched = Letter == Code[Matched] ? Matched + 1 : Letter == Code[0] ? 1 : 0;
        LastLetter = Now;
        Consume = Matched >= 2;
        if (Matched == Code.Len()) { Matched = 0; return true; }
        return false;
    }
    void Reset() { Matched = 0; }
};
