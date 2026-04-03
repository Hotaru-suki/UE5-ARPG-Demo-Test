// Copyright Epic Games, Inc. All Rights Reserved.

#include "RPGPerformanceMonitorSubsystem.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Misc/CString.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RPGCharacterBase.h"

void URPGPerformanceMonitorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ParseCommandLine();

	if (bCaptureEnabled)
	{
		StartCapture();
	}
}

void URPGPerformanceMonitorSubsystem::Deinitialize()
{
	StopCapture();
	Super::Deinitialize();
}

void URPGPerformanceMonitorSubsystem::Tick(float DeltaTime)
{
	if (!bCaptureEnabled || CsvWriter == nullptr)
	{
		return;
	}

	TimeSinceLastCapture += DeltaTime;
	if (TimeSinceLastCapture < CaptureIntervalSeconds)
	{
		return;
	}

	CaptureSample(DeltaTime);
	TimeSinceLastCapture = 0.0f;
}

TStatId URPGPerformanceMonitorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URPGPerformanceMonitorSubsystem, STATGROUP_Tickables);
}

bool URPGPerformanceMonitorSubsystem::IsTickable() const
{
	return bCaptureEnabled && GetWorld() != nullptr;
}

bool URPGPerformanceMonitorSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FRPGPerformanceSummary URPGPerformanceMonitorSubsystem::GetSummary() const
{
	FRPGPerformanceSummary Summary;
	Summary.bHasSamples = SampleCount > 0;
	Summary.SessionLabel = SessionLabel;
	Summary.SampleCount = SampleCount;
	Summary.AverageFPS = SampleCount > 0 ? (TotalFPS / SampleCount) : 0.0;
	Summary.MinFPS = MinFPS;
	Summary.MaxFPS = MaxFPS;
	Summary.AverageFrameTimeMs = SampleCount > 0 ? (TotalFrameTimeMs / SampleCount) : 0.0;
	Summary.MinFrameTimeMs = MinFrameTimeMs;
	Summary.MaxFrameTimeMs = MaxFrameTimeMs;
	Summary.PeakActors = PeakActors;
	Summary.PeakPawns = PeakPawns;
	Summary.PeakControllers = PeakControllers;
	Summary.PeakAIControllers = PeakAIControllers;
	Summary.PeakAICharacters = PeakAICharacters;
	Summary.PeakPickupLikeActors = PeakPickupLikeActors;
	Summary.AverageCPUTimePct = SampleCount > 0 ? (TotalCPUTimePct / SampleCount) : 0.0;
	Summary.PeakCPUTimePct = PeakCPUTimePct;
	Summary.AverageCPUTimePctRelative = SampleCount > 0 ? (TotalCPUTimePctRelative / SampleCount) : 0.0;
	Summary.PeakCPUTimePctRelative = PeakCPUTimePctRelative;
	Summary.AverageThreadCPUTimePctRelative = SampleCount > 0 ? (TotalThreadCPUTimePctRelative / SampleCount) : 0.0;
	Summary.PeakThreadCPUTimePctRelative = PeakThreadCPUTimePctRelative;
	Summary.PeakUsedPhysicalMB = PeakUsedPhysicalMB;
	Summary.LowestAvailablePhysicalMB = LowestAvailablePhysicalMB;
	Summary.PeakUsedVirtualMB = PeakUsedVirtualMB;
	Summary.LowestAvailableVirtualMB = LowestAvailableVirtualMB;
	Summary.LastMapName = LastMapName;
	Summary.CsvPath = OutputPath;
	Summary.SummaryPath = BuildSummaryPath();
	return Summary;
}

void URPGPerformanceMonitorSubsystem::ParseCommandLine()
{
	const TCHAR* CommandLine = FCommandLine::Get();

	bCaptureEnabled = FParse::Param(CommandLine, TEXT("RPGPerfMonitor")) || FParse::Param(CommandLine, TEXT("RPGAutoTest"));
	bShowOnScreen = FParse::Param(CommandLine, TEXT("RPGPerfScreen"));
	SessionLabel = TEXT("DefaultSession");

	float ParsedInterval = CaptureIntervalSeconds;
	if (FParse::Value(CommandLine, TEXT("RPGPerfInterval="), ParsedInterval) && ParsedInterval > 0.0f)
	{
		CaptureIntervalSeconds = ParsedInterval;
	}

	FParse::Value(CommandLine, TEXT("RPGPerfOutput="), OutputOverride);
	FParse::Value(CommandLine, TEXT("RPGPerfLabel="), SessionLabel);
	if (SessionLabel == TEXT("DefaultSession"))
	{
		FParse::Value(CommandLine, TEXT("RPGAutoTestLabel="), SessionLabel);
	}
}

void URPGPerformanceMonitorSubsystem::StartCapture()
{
	OutputPath = BuildOutputPath();
	const FString SummaryPath = BuildSummaryPath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(SummaryPath), true);

	CsvWriter = IFileManager::Get().CreateFileWriter(*OutputPath, FILEWRITE_Append);
	if (CsvWriter == nullptr)
	{
		UE_LOG(LogActionRPG, Warning, TEXT("Performance monitor failed to open output file: %s"), *OutputPath);
		bCaptureEnabled = false;
		return;
	}

	UE_LOG(LogActionRPG, Log, TEXT("Performance monitor enabled. Label=%s Interval=%.2fs Output=%s"), *SessionLabel, CaptureIntervalSeconds, *OutputPath);
	WriteHeader();
}

void URPGPerformanceMonitorSubsystem::StopCapture()
{
	if (CsvWriter)
	{
		CsvWriter->Flush();
		delete CsvWriter;
		CsvWriter = nullptr;
	}

	if (bCaptureEnabled && SampleCount > 0)
	{
		WriteSummary();
	}
}

void URPGPerformanceMonitorSubsystem::CaptureSample(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	int32 TotalActors = 0;
	int32 PawnCount = 0;
	int32 CharacterCount = 0;
	int32 ControllerCount = 0;
	int32 PlayerControllerCount = 0;
	int32 AIControllerCount = 0;
	int32 PlayerCharacterCount = 0;
	int32 AICharacterCount = 0;

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		++TotalActors;

		if (Cast<APawn>(*ActorIt) != nullptr)
		{
			++PawnCount;
		}

		if (AAIController* AIController = Cast<AAIController>(*ActorIt))
		{
			++ControllerCount;
			++AIControllerCount;
		}
		else if (Cast<APlayerController>(*ActorIt) != nullptr)
		{
			++ControllerCount;
			++PlayerControllerCount;
		}
		else if (Cast<AController>(*ActorIt) != nullptr)
		{
			++ControllerCount;
		}

		if (ARPGCharacterBase* Character = Cast<ARPGCharacterBase>(*ActorIt))
		{
			++CharacterCount;
			if (Cast<APlayerController>(Character->GetController()) != nullptr)
			{
				++PlayerCharacterCount;
			}
			else
			{
				++AICharacterCount;
			}
		}
	}

	const int32 PickupLikeActorCount = CountPickupLikeActors();
	const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
	const FCPUTime CPUTime = FPlatformTime::GetCPUTime();
	const FCPUTime ThreadCPUTime = FPlatformTime::GetThreadCPUTime();
	const double DeltaSeconds = FApp::GetDeltaTime();
	const double FPS = DeltaSeconds > 0.0 ? (1.0 / DeltaSeconds) : 0.0;
	const double FrameTimeMs = DeltaSeconds * 1000.0;
	const double CPUTimePct = CPUTime.CPUTimePct;
	const double CPUTimePctRelative = CPUTime.CPUTimePctRelative;
	const double ThreadCPUTimePctRelative = ThreadCPUTime.CPUTimePctRelative;
	const double UsedPhysicalMB = static_cast<double>(MemoryStats.UsedPhysical) / (1024.0 * 1024.0);
	const double AvailablePhysicalMB = static_cast<double>(MemoryStats.AvailablePhysical) / (1024.0 * 1024.0);
	const double UsedVirtualMB = static_cast<double>(MemoryStats.UsedVirtual) / (1024.0 * 1024.0);
	const double AvailableVirtualMB = static_cast<double>(MemoryStats.AvailableVirtual) / (1024.0 * 1024.0);

	const FString Row = FString::Printf(
		TEXT("%s,%.3f,%.2f,%.3f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%s\n"),
		*SessionLabel,
		World->TimeSeconds,
		FPS,
		FrameTimeMs,
		CPUTimePct,
		CPUTimePctRelative,
		ThreadCPUTimePctRelative,
		TotalActors,
		PawnCount,
		CharacterCount,
		ControllerCount,
		PlayerControllerCount,
		AIControllerCount,
		PlayerCharacterCount,
		AICharacterCount,
		PickupLikeActorCount,
		UsedPhysicalMB,
		AvailablePhysicalMB,
		UsedVirtualMB,
		AvailableVirtualMB,
		*World->GetMapName());

	WriteRow(Row);
	UpdateSummary(
		FPS,
		FrameTimeMs,
		CPUTimePct,
		CPUTimePctRelative,
		ThreadCPUTimePctRelative,
		TotalActors,
		PawnCount,
		ControllerCount,
		AIControllerCount,
		AICharacterCount,
		PickupLikeActorCount,
		UsedPhysicalMB,
		AvailablePhysicalMB,
		UsedVirtualMB,
		AvailableVirtualMB,
		World->GetMapName());

	if (bShowOnScreen)
	{
		const FString DebugLine = FString::Printf(
			TEXT("Perf FPS %.1f | Frame %.2f ms | CPU %.1f%% | AI %d | Ctrl %d | Pickups %d | Actors %d"),
			FPS,
			FrameTimeMs,
			CPUTimePctRelative,
			AICharacterCount,
			AIControllerCount,
			PickupLikeActorCount,
			TotalActors);
		EmitScreenDebug(DebugLine);
	}
}

void URPGPerformanceMonitorSubsystem::WriteHeader()
{
	if (bHeaderWritten || CsvWriter == nullptr)
	{
		return;
	}

	static const FString Header = TEXT("SessionLabel,TimeSeconds,FPS,FrameTimeMs,CPUTimePct,CPUTimePctRelative,ThreadCPUTimePctRelative,TotalActors,Pawns,Characters,Controllers,PlayerControllers,AIControllers,PlayerCharacters,AICharacters,PickupLikeActors,UsedPhysicalMB,AvailablePhysicalMB,UsedVirtualMB,AvailableVirtualMB,MapName\n");
	WriteRow(Header);
	bHeaderWritten = true;
}

void URPGPerformanceMonitorSubsystem::WriteRow(const FString& Row)
{
	if (CsvWriter == nullptr)
	{
		return;
	}

	FTCHARToUTF8 Converter(*Row);
	CsvWriter->Serialize((void*)Converter.Get(), Converter.Length());
	CsvWriter->Flush();
}

void URPGPerformanceMonitorSubsystem::EmitScreenDebug(const FString& Message) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage((uint64)((PTRINT)this), CaptureIntervalSeconds, FColor::Green, Message);
	}
}

FString URPGPerformanceMonitorSubsystem::BuildOutputPath() const
{
	if (!OutputOverride.IsEmpty())
	{
		return FPaths::ConvertRelativePathToFull(OutputOverride);
	}

	const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PerfReports"), FString::Printf(TEXT("RPGPerf_%s.csv"), *Timestamp));
}

FString URPGPerformanceMonitorSubsystem::BuildSummaryPath() const
{
	const FString ResolvedOutputPath = OutputPath.IsEmpty() ? BuildOutputPath() : OutputPath;
	return FPaths::ChangeExtension(ResolvedOutputPath, TEXT("summary.md"));
}

void URPGPerformanceMonitorSubsystem::UpdateSummary(
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
	const FString& MapName)
{
	++SampleCount;
	TotalFPS += FPS;
	TotalFrameTimeMs += FrameTimeMs;
	TotalCPUTimePct += CPUTimePct;
	TotalCPUTimePctRelative += CPUTimePctRelative;
	TotalThreadCPUTimePctRelative += ThreadCPUTimePctRelative;
	LastMapName = MapName;

	if (SampleCount == 1)
	{
		MinFPS = MaxFPS = FPS;
		MinFrameTimeMs = MaxFrameTimeMs = FrameTimeMs;
		LowestAvailablePhysicalMB = AvailablePhysicalMB;
		LowestAvailableVirtualMB = AvailableVirtualMB;
	}
	else
	{
		MinFPS = FMath::Min(MinFPS, FPS);
		MaxFPS = FMath::Max(MaxFPS, FPS);
		MinFrameTimeMs = FMath::Min(MinFrameTimeMs, FrameTimeMs);
		MaxFrameTimeMs = FMath::Max(MaxFrameTimeMs, FrameTimeMs);
		LowestAvailablePhysicalMB = FMath::Min(LowestAvailablePhysicalMB, AvailablePhysicalMB);
		LowestAvailableVirtualMB = FMath::Min(LowestAvailableVirtualMB, AvailableVirtualMB);
	}

	PeakActors = FMath::Max(PeakActors, TotalActors);
	PeakPawns = FMath::Max(PeakPawns, PawnCount);
	PeakControllers = FMath::Max(PeakControllers, ControllerCount);
	PeakAIControllers = FMath::Max(PeakAIControllers, AIControllerCount);
	PeakAICharacters = FMath::Max(PeakAICharacters, AICharacterCount);
	PeakPickupLikeActors = FMath::Max(PeakPickupLikeActors, PickupLikeActorCount);
	PeakCPUTimePct = FMath::Max(PeakCPUTimePct, CPUTimePct);
	PeakCPUTimePctRelative = FMath::Max(PeakCPUTimePctRelative, CPUTimePctRelative);
	PeakThreadCPUTimePctRelative = FMath::Max(PeakThreadCPUTimePctRelative, ThreadCPUTimePctRelative);
	PeakUsedPhysicalMB = FMath::Max(PeakUsedPhysicalMB, UsedPhysicalMB);
	PeakUsedVirtualMB = FMath::Max(PeakUsedVirtualMB, UsedVirtualMB);
}

void URPGPerformanceMonitorSubsystem::WriteSummary()
{
	const double AverageFPS = SampleCount > 0 ? (TotalFPS / SampleCount) : 0.0;
	const double AverageFrameTimeMs = SampleCount > 0 ? (TotalFrameTimeMs / SampleCount) : 0.0;
	const double AverageCPUTimePct = SampleCount > 0 ? (TotalCPUTimePct / SampleCount) : 0.0;
	const double AverageCPUTimePctRelative = SampleCount > 0 ? (TotalCPUTimePctRelative / SampleCount) : 0.0;
	const double AverageThreadCPUTimePctRelative = SampleCount > 0 ? (TotalThreadCPUTimePctRelative / SampleCount) : 0.0;
	const FString SummaryPath = BuildSummaryPath();
	const FString Summary = FString::Printf(
		TEXT("# Performance Session Summary\n\n")
		TEXT("- SessionLabel: `%s`\n")
		TEXT("- Samples: `%d`\n")
		TEXT("- AverageFPS: `%.2f`\n")
		TEXT("- MinFPS: `%.2f`\n")
		TEXT("- MaxFPS: `%.2f`\n")
		TEXT("- AverageFrameTimeMs: `%.3f`\n")
		TEXT("- MinFrameTimeMs: `%.3f`\n")
		TEXT("- MaxFrameTimeMs: `%.3f`\n")
		TEXT("- AverageCPUTimePct: `%.2f`\n")
		TEXT("- PeakCPUTimePct: `%.2f`\n")
		TEXT("- AverageCPUTimePctRelative: `%.2f`\n")
		TEXT("- PeakCPUTimePctRelative: `%.2f`\n")
		TEXT("- AverageThreadCPUTimePctRelative: `%.2f`\n")
		TEXT("- PeakThreadCPUTimePctRelative: `%.2f`\n")
		TEXT("- PeakActors: `%d`\n")
		TEXT("- PeakPawns: `%d`\n")
		TEXT("- PeakControllers: `%d`\n")
		TEXT("- PeakAIControllers: `%d`\n")
		TEXT("- PeakAICharacters: `%d`\n")
		TEXT("- PeakPickupLikeActors: `%d`\n")
		TEXT("- PeakUsedPhysicalMB: `%.2f`\n")
		TEXT("- LowestAvailablePhysicalMB: `%.2f`\n")
		TEXT("- PeakUsedVirtualMB: `%.2f`\n")
		TEXT("- LowestAvailableVirtualMB: `%.2f`\n")
		TEXT("- LastMapName: `%s`\n")
		TEXT("- CsvPath: `%s`\n"),
		*SessionLabel,
		SampleCount,
		AverageFPS,
		MinFPS,
		MaxFPS,
		AverageFrameTimeMs,
		MinFrameTimeMs,
		MaxFrameTimeMs,
		AverageCPUTimePct,
		PeakCPUTimePct,
		AverageCPUTimePctRelative,
		PeakCPUTimePctRelative,
		AverageThreadCPUTimePctRelative,
		PeakThreadCPUTimePctRelative,
		PeakActors,
		PeakPawns,
		PeakControllers,
		PeakAIControllers,
		PeakAICharacters,
		PeakPickupLikeActors,
		PeakUsedPhysicalMB,
		LowestAvailablePhysicalMB,
		PeakUsedVirtualMB,
		LowestAvailableVirtualMB,
		*LastMapName,
		*OutputPath);

	FFileHelper::SaveStringToFile(Summary, *SummaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogActionRPG, Log, TEXT("Performance summary written: %s"), *SummaryPath);
}

int32 URPGPerformanceMonitorSubsystem::CountPickupLikeActors() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		const FString ClassName = GetNameSafe((*ActorIt)->GetClass());
		if (ClassName.Contains(TEXT("Pickup")) || ClassName.Contains(TEXT("SoulItem")) || ClassName.Contains(TEXT("Drop")))
		{
			++Count;
		}
	}

	return Count;
}
