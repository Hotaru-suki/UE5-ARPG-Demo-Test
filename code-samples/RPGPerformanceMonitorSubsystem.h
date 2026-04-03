// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActionRPG.h"
#include "Subsystems/WorldSubsystem.h"
#include "RPGPerformanceMonitorSubsystem.generated.h"

class FArchive;

USTRUCT()
struct ACTIONRPG_API FRPGPerformanceSummary
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasSamples = false;

	UPROPERTY()
	FString SessionLabel;

	UPROPERTY()
	int32 SampleCount = 0;

	UPROPERTY()
	double AverageFPS = 0.0;

	UPROPERTY()
	double MinFPS = 0.0;

	UPROPERTY()
	double MaxFPS = 0.0;

	UPROPERTY()
	double AverageFrameTimeMs = 0.0;

	UPROPERTY()
	double MinFrameTimeMs = 0.0;

	UPROPERTY()
	double MaxFrameTimeMs = 0.0;

	UPROPERTY()
	int32 PeakActors = 0;

	UPROPERTY()
	int32 PeakPawns = 0;

	UPROPERTY()
	int32 PeakControllers = 0;

	UPROPERTY()
	int32 PeakAIControllers = 0;

	UPROPERTY()
	int32 PeakAICharacters = 0;

	UPROPERTY()
	int32 PeakPickupLikeActors = 0;

	UPROPERTY()
	double AverageCPUTimePct = 0.0;

	UPROPERTY()
	double PeakCPUTimePct = 0.0;

	UPROPERTY()
	double AverageCPUTimePctRelative = 0.0;

	UPROPERTY()
	double PeakCPUTimePctRelative = 0.0;

	UPROPERTY()
	double AverageThreadCPUTimePctRelative = 0.0;

	UPROPERTY()
	double PeakThreadCPUTimePctRelative = 0.0;

	UPROPERTY()
	double PeakUsedPhysicalMB = 0.0;

	UPROPERTY()
	double LowestAvailablePhysicalMB = 0.0;

	UPROPERTY()
	double PeakUsedVirtualMB = 0.0;

	UPROPERTY()
	double LowestAvailableVirtualMB = 0.0;

	UPROPERTY()
	FString LastMapName;

	UPROPERTY()
	FString CsvPath;

	UPROPERTY()
	FString SummaryPath;
};

UCLASS()
class ACTIONRPG_API URPGPerformanceMonitorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	bool IsCaptureEnabled() const { return bCaptureEnabled; }
	FRPGPerformanceSummary GetSummary() const;

private:
	void ParseCommandLine();
	void StartCapture();
	void StopCapture();
	void CaptureSample(float DeltaTime);
	void WriteHeader();
	void WriteRow(const FString& Row);
	void EmitScreenDebug(const FString& Message) const;
	FString BuildOutputPath() const;
	FString BuildSummaryPath() const;
	void UpdateSummary(
		double FPS,
		double FrameTimeMs,
		double CPUTimePct,
		double CPUTimePctRelative,
		double ThreadCPUTimePctRelative,
		int32 TotalActors,
		int32 PawnCount,
		int32 ControllerCount,
		int32 AIControllerCount,
		int32 AICharacterCount,
		int32 PickupLikeActorCount,
		double UsedPhysicalMB,
		double AvailablePhysicalMB,
		double UsedVirtualMB,
		double AvailableVirtualMB,
		const FString& MapName);
	void WriteSummary();
	int32 CountPickupLikeActors() const;

private:
	bool bCaptureEnabled = false;
	bool bShowOnScreen = false;
	bool bHeaderWritten = false;
	float CaptureIntervalSeconds = 1.0f;
	float TimeSinceLastCapture = 0.0f;
	FString OutputOverride;
	FString OutputPath;
	FString SessionLabel;
	FString LastMapName;
	FArchive* CsvWriter = nullptr;
	int32 SampleCount = 0;
	double TotalFPS = 0.0;
	double TotalFrameTimeMs = 0.0;
	double TotalCPUTimePct = 0.0;
	double TotalCPUTimePctRelative = 0.0;
	double TotalThreadCPUTimePctRelative = 0.0;
	double MinFPS = 0.0;
	double MaxFPS = 0.0;
	double MinFrameTimeMs = 0.0;
	double MaxFrameTimeMs = 0.0;
	int32 PeakActors = 0;
	int32 PeakPawns = 0;
	int32 PeakControllers = 0;
	int32 PeakAIControllers = 0;
	int32 PeakAICharacters = 0;
	int32 PeakPickupLikeActors = 0;
	double PeakCPUTimePct = 0.0;
	double PeakCPUTimePctRelative = 0.0;
	double PeakThreadCPUTimePctRelative = 0.0;
	double PeakUsedPhysicalMB = 0.0;
	double LowestAvailablePhysicalMB = 0.0;
	double PeakUsedVirtualMB = 0.0;
	double LowestAvailableVirtualMB = 0.0;
};
