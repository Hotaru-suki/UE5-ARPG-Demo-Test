// Copyright Epic Games, Inc. All Rights Reserved.

#include "RPGAutomationTestSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RPGPerformanceMonitorSubsystem.h"

void URPGAutomationTestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ParseCommandLine();
}

void URPGAutomationTestSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URPGAutomationTestSubsystem::Tick(float DeltaTime)
{
	if (!bEnabled || bFinished)
	{
		return;
	}

	ElapsedSeconds += DeltaTime;

	if (!bStarted && ElapsedSeconds >= StartupDelaySeconds)
	{
		StartTest();
	}

	if (bStarted && !bAutoplayTriggered && RemainingAutoplayAttempts > 0)
	{
		ActiveTestSeconds += DeltaTime;
		if (ActiveTestSeconds >= AutoplayInitialDelaySeconds)
		{
			TryTriggerAutoplay();
			AutoplayInitialDelaySeconds += AutoplayRetryIntervalSeconds;
		}
	}

	if (bStarted && ElapsedSeconds >= (StartupDelaySeconds + TestDurationSeconds))
	{
		FinishTest();
	}
}

TStatId URPGAutomationTestSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URPGAutomationTestSubsystem, STATGROUP_Tickables);
}

bool URPGAutomationTestSubsystem::IsTickable() const
{
	return bEnabled && GetWorld() != nullptr;
}

bool URPGAutomationTestSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URPGAutomationTestSubsystem::ParseCommandLine()
{
	const TCHAR* CommandLine = FCommandLine::Get();
	bEnabled = FParse::Param(CommandLine, TEXT("RPGAutoTest"));
	if (!bEnabled)
	{
		return;
	}

	TestLabel = TEXT("RPGAutoTest");
	FParse::Value(CommandLine, TEXT("RPGAutoTestLabel="), TestLabel);
	FParse::Value(CommandLine, TEXT("RPGAutoTestCommands="), StartupCommands);
	FParse::Value(CommandLine, TEXT("RPGAutoTestDuration="), TestDurationSeconds);
	FParse::Value(CommandLine, TEXT("RPGAutoTestDelay="), StartupDelaySeconds);
	FParse::Value(CommandLine, TEXT("RPGAutoTestMinAvgFPS="), MinAverageFPS);
	FParse::Value(CommandLine, TEXT("RPGAutoTestMinPeakAI="), MinPeakAICharacters);
	bTriggerAutoplay = FParse::Param(CommandLine, TEXT("RPGAutoTestAutoplay"));
	FParse::Value(CommandLine, TEXT("RPGAutoTestAutoplayDelay="), AutoplayInitialDelaySeconds);
	FParse::Value(CommandLine, TEXT("RPGAutoTestAutoplayRetryInterval="), AutoplayRetryIntervalSeconds);
	RemainingAutoplayAttempts = bTriggerAutoplay ? 3 : 0;
	bAutoExit = FParse::Param(CommandLine, TEXT("RPGAutoTestExit"));

	if (TestDurationSeconds <= 0.0f)
	{
		TestDurationSeconds = 30.0f;
	}

	if (StartupDelaySeconds < 0.0f)
	{
		StartupDelaySeconds = 0.0f;
	}

	if (AutoplayInitialDelaySeconds < 0.0f)
	{
		AutoplayInitialDelaySeconds = 0.0f;
	}

	if (AutoplayRetryIntervalSeconds <= 0.0f)
	{
		AutoplayRetryIntervalSeconds = 1.0f;
	}
}

void URPGAutomationTestSubsystem::StartTest()
{
	if (bStarted)
	{
		return;
	}

	bStarted = true;
	ActiveTestSeconds = 0.0f;
	RunStartupCommands();
	UE_LOG(LogActionRPG, Log, TEXT("Automation test started. Label=%s Duration=%.2fs Autoplay=%s"), *TestLabel, TestDurationSeconds, bTriggerAutoplay ? TEXT("true") : TEXT("false"));
}

void URPGAutomationTestSubsystem::FinishTest()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;

	const URPGPerformanceMonitorSubsystem* PerfSubsystem = GetWorld()->GetSubsystem<URPGPerformanceMonitorSubsystem>();
	const FRPGPerformanceSummary Summary = PerfSubsystem ? PerfSubsystem->GetSummary() : FRPGPerformanceSummary();

	bool bPassed = true;
	FString Reason = TEXT("Completed");

	if (!Summary.bHasSamples)
	{
		bPassed = false;
		Reason = TEXT("No performance samples were captured. Ensure -RPGAutoTest or -RPGPerfMonitor is active.");
	}
	else if (MinAverageFPS > 0.0f && Summary.AverageFPS < MinAverageFPS)
	{
		bPassed = false;
		Reason = FString::Printf(TEXT("AverageFPS %.2f is below threshold %.2f"), Summary.AverageFPS, MinAverageFPS);
	}
	else if (MinPeakAICharacters > 0 && Summary.PeakAICharacters < MinPeakAICharacters)
	{
		bPassed = false;
		Reason = FString::Printf(TEXT("PeakAICharacters %d is below threshold %d"), Summary.PeakAICharacters, MinPeakAICharacters);
	}

	WriteReport(bPassed, Reason);
	UE_LOG(LogActionRPG, Log, TEXT("Automation test finished. Label=%s Result=%s Reason=%s"), *TestLabel, bPassed ? TEXT("PASS") : TEXT("FAIL"), *Reason);

	if (bAutoExit)
	{
		FPlatformMisc::RequestExit(false);
	}
}

void URPGAutomationTestSubsystem::RunStartupCommands()
{
	if (StartupCommands.IsEmpty())
	{
		return;
	}

	TArray<FString> Commands;
	StartupCommands.ParseIntoArray(Commands, TEXT("|"), true);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	for (const FString& Command : Commands)
	{
		const FString Trimmed = Command.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			continue;
		}

		if (PlayerController)
		{
			PlayerController->ConsoleCommand(Trimmed, true);
		}
		else if (GEngine)
		{
			GEngine->Exec(GetWorld(), *Trimmed);
		}
	}
}

void URPGAutomationTestSubsystem::TryTriggerAutoplay()
{
	--RemainingAutoplayAttempts;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController == nullptr)
	{
		UE_LOG(LogActionRPG, Warning, TEXT("Automation autoplay trigger skipped: no PlayerController yet. Remaining attempts=%d"), RemainingAutoplayAttempts);
		return;
	}

	const bool bPressed = PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::BackSpace, IE_Pressed, 1.0f));
	const bool bReleased = PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::BackSpace, IE_Released, 0.0f));
	bAutoplayTriggered = bPressed || bReleased;

	UE_LOG(
		LogActionRPG,
		Log,
		TEXT("Automation autoplay trigger attempted via BackSpace. Pressed=%s Released=%s Remaining attempts=%d"),
		bPressed ? TEXT("true") : TEXT("false"),
		bReleased ? TEXT("true") : TEXT("false"),
		RemainingAutoplayAttempts);
}

void URPGAutomationTestSubsystem::WriteReport(bool bPassed, const FString& Reason) const
{
	const URPGPerformanceMonitorSubsystem* PerfSubsystem = GetWorld()->GetSubsystem<URPGPerformanceMonitorSubsystem>();
	const FRPGPerformanceSummary Summary = PerfSubsystem ? PerfSubsystem->GetSummary() : FRPGPerformanceSummary();
	const FString ReportPath = BuildReportPath();

	const FString Report = FString::Printf(
		TEXT("# Automation Test Report\n\n")
		TEXT("- Label: `%s`\n")
		TEXT("- Result: `%s`\n")
		TEXT("- Reason: `%s`\n")
		TEXT("- DurationSeconds: `%.2f`\n")
		TEXT("- StartupDelaySeconds: `%.2f`\n")
		TEXT("- MinAverageFPSThreshold: `%.2f`\n")
		TEXT("- MinPeakAIThreshold: `%d`\n")
		TEXT("- StartupCommands: `%s`\n")
		TEXT("- TriggerAutoplay: `%s`\n")
		TEXT("- AutoplayTriggered: `%s`\n")
		TEXT("- HasPerfSamples: `%s`\n")
		TEXT("- AverageFPS: `%.2f`\n")
		TEXT("- AverageCPUTimePctRelative: `%.2f`\n")
		TEXT("- PeakAICharacters: `%d`\n")
		TEXT("- PeakActors: `%d`\n")
		TEXT("- PeakPawns: `%d`\n")
		TEXT("- PeakControllers: `%d`\n")
		TEXT("- PeakAIControllers: `%d`\n")
		TEXT("- PeakPickupLikeActors: `%d`\n")
		TEXT("- PeakCPUTimePctRelative: `%.2f`\n")
		TEXT("- PeakUsedPhysicalMB: `%.2f`\n")
		TEXT("- LowestAvailablePhysicalMB: `%.2f`\n")
		TEXT("- CsvPath: `%s`\n")
		TEXT("- PerfSummaryPath: `%s`\n"),
		*TestLabel,
		bPassed ? TEXT("PASS") : TEXT("FAIL"),
		*Reason,
		TestDurationSeconds,
		StartupDelaySeconds,
		MinAverageFPS,
		MinPeakAICharacters,
		*StartupCommands,
		bTriggerAutoplay ? TEXT("true") : TEXT("false"),
		bAutoplayTriggered ? TEXT("true") : TEXT("false"),
		Summary.bHasSamples ? TEXT("true") : TEXT("false"),
		Summary.AverageFPS,
		Summary.AverageCPUTimePctRelative,
		Summary.PeakAICharacters,
		Summary.PeakActors,
		Summary.PeakPawns,
		Summary.PeakControllers,
		Summary.PeakAIControllers,
		Summary.PeakPickupLikeActors,
		Summary.PeakCPUTimePctRelative,
		Summary.PeakUsedPhysicalMB,
		Summary.LowestAvailablePhysicalMB,
		*Summary.CsvPath,
		*Summary.SummaryPath);

	FFileHelper::SaveStringToFile(Report, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString URPGAutomationTestSubsystem::BuildReportPath() const
{
	const URPGPerformanceMonitorSubsystem* PerfSubsystem = GetWorld()->GetSubsystem<URPGPerformanceMonitorSubsystem>();
	const FRPGPerformanceSummary Summary = PerfSubsystem ? PerfSubsystem->GetSummary() : FRPGPerformanceSummary();
	const FString BasePath = !Summary.CsvPath.IsEmpty()
		? Summary.CsvPath
		: FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PerfReports"), TestLabel + TEXT(".csv"));
	return FPaths::ChangeExtension(BasePath, TEXT("autotest.md"));
}
