// Copyright CrystalVapor 2026, All rights reserved.

#include "Components/CRRecoilSpreadComponent.h"
#include "Engine/World.h"

namespace
{
    bool HasRuntimeCurveData(const FRuntimeFloatCurve& RuntimeCurve)
    {
        const FRichCurve* RichCurve = RuntimeCurve.GetRichCurveConst();
        return RichCurve && RichCurve->HasAnyData();
    }
}

void UCRRecoilSpreadComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const UWorld* World = GetWorld();
    const bool bCanProcessHeat = ReadyToCalculateRecoil();
    if (World && bCanProcessHeat && LastFireTime + RecoilHeatCooldownDelay < World->GetTimeSeconds())
    {
        DoHeatCooldown(DeltaTime);
    }

    // Keeps ticking if heat still needs cooldown or base recoil is still active
    const bool bHasPendingHeatWork = bCanProcessHeat && !FMath::IsNearlyZero(CurrentRecoilHeat);
    const bool bHasPendingRecoilWork = bHasPendingHeatWork || !RecoilToApply.IsNearlyZero() || !RecoilToRecover.IsNearlyZero(0.001);
    SetComponentTickEnabled(bHasPendingRecoilWork);
}

void UCRRecoilSpreadComponent::ApplyShot()
{
    Super::ApplyShot();

    if (ReadyToCalculateRecoil())
    {
        AddRecoilHeat(ShotToHeatCurve.GetRichCurveConst()->Eval(CurrentRecoilHeat));
    }
}

void UCRRecoilSpreadComponent::AddRecoilHeat(const float InHeat)
{
    // It's not redundant for external Blueprint calls - if someone calls AddRecoilHeat outside of ApplyShot
    SetComponentTickEnabled(true);
    SetRecoilHeat(GetRecoilHeat() + InHeat);
}

void UCRRecoilSpreadComponent::SetRecoilHeat(const float InHeat)
{
    const float CachedCurrentRecoilHeat = CurrentRecoilHeat;
    CurrentRecoilHeat = FMath::Clamp(InHeat, 0.f, MaxRecoilHeat);
    OnHeatChanged.Broadcast(CurrentRecoilHeat, CachedCurrentRecoilHeat);
}

float UCRRecoilSpreadComponent::GetRecoilHeat() const
{
    return CurrentRecoilHeat;
}

void UCRRecoilSpreadComponent::SetMaxRecoilHeat(const float InMaxHeat)
{
    MaxRecoilHeat = FMath::Max(0.f, InMaxHeat);
    if (CurrentRecoilHeat > MaxRecoilHeat)
    {
        SetRecoilHeat(MaxRecoilHeat);
    }
}

void UCRRecoilSpreadComponent::SetRecoilHeatCoolDownDelay(const float InDelay)
{
    RecoilHeatCooldownDelay = FMath::Max(0.f, InDelay);
}

float UCRRecoilSpreadComponent::GetCurrentSpreadAngle() const
{
    if (!ensureMsgf(ReadyToCalculateRecoil(), TEXT("All recoil spread curves must contain data")))
    {
        return 0.f;
    }
    return HeatToSpreadAngleCurve.GetRichCurveConst()->Eval(CurrentRecoilHeat);
}

void UCRRecoilSpreadComponent::DoHeatCooldown(const float DeltaTime)
{
    if (!ensureMsgf(HasRuntimeCurveData(HeatToCooldownPerSecondCurve), TEXT("HeatToCooldownPerSecondCurve does not contain data")))
    {
        return;
    }
    const float DeltaCooldown = HeatToCooldownPerSecondCurve.GetRichCurveConst()->Eval(CurrentRecoilHeat) * DeltaTime;
    SetRecoilHeat(CurrentRecoilHeat - DeltaCooldown);
}

bool UCRRecoilSpreadComponent::ReadyToCalculateRecoil() const
{
    return HasRuntimeCurveData(ShotToHeatCurve) && HasRuntimeCurveData(HeatToSpreadAngleCurve) && HasRuntimeCurveData(HeatToCooldownPerSecondCurve);
}
