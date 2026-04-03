// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActionRPG.h"
#include "Subsystems/WorldSubsystem.h"
#include "RPGAutomationTestSubsystem.generated.h"

UCLASS()
class ACTIONRPG_API URPGAutomationTestSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void ParseCommandLine();
	void StartTest();
	void FinishTest();
	void RunStartupCommands();
	void TryTriggerAutoplay();
	void WriteReport(bool bPassed, const FString& Reason) const;
	FString BuildReportPath() const;

private:
	bool bEnabled = false;
	bool bStarted = false;
	bool bFinished = false;
	bool bAutoExit = false;
	float StartupDelaySeconds = 1.0f;
	float TestDurationSeconds = 30.0f;
	float ElapsedSeconds = 0.0f;
	float ActiveTestSeconds = 0.0f;
	float MinAverageFPS = 0.0f;
	float AutoplayInitialDelaySeconds = 0.5f;
	float AutoplayRetryIntervalSeconds = 1.0f;
	int32 MinPeakAICharacters = 0;
	int32 RemainingAutoplayAttempts = 0;
	FString TestLabel;
	FString StartupCommands;
	bool bTriggerAutoplay = false;
	bool bAutoplayTriggered = false;
};
