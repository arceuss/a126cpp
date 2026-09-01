param(
	[string[]]$Backends = @("nativegl", "gl21", "gl46", "vulkan", "d3d12"),
	[string[]]$WorkloadNames = @("world", "sign-control", "sign-boards", "sign-text"),
	[int]$MeasuredFrames = 600,
	[int]$Repetitions = 5,
	[int]$WarmupFrames = 240,
	[int]$SignCount = 256,
	[string]$World = "World4",
	[string]$BuildDirectory = "",
	[string]$OutputDirectory = "",
	[string]$Executable = "",
	[switch]$SkipBuild,
	[switch]$SkipForcedFinish,
	[switch]$OnlyForcedFinish,
	[switch]$DisableDiagnostics
)
$ErrorActionPreference = "Stop"


$repo = Split-Path -Parent $PSScriptRoot
$workspace = Split-Path -Parent $repo
if ([string]::IsNullOrWhiteSpace($BuildDirectory))
{
	$BuildDirectory = Join-Path $repo "build-profile-renderers"
}
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
if ([string]::IsNullOrWhiteSpace($OutputDirectory))
{
	$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
	$OutputDirectory = Join-Path $repo "profile-results\$stamp"
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

if ($MeasuredFrames -le 0)
{
	throw "MeasuredFrames must be positive"
}
if ($Repetitions -lt 1)
{
	throw "Repetitions must be at least one"
}

$oldBuildDirectory = $env:A126_BUILD_DIR
$oldDiagnostics = $env:A126_RENDER_DIAGNOSTICS
$oldValidation = $env:A126_LEGACYGL_VALIDATE
$oldTrace = $env:A126_LEGACYGL_TRACE
$oldPipelineCache = $env:A126_VULKAN_PIPELINE_CACHE

function Restore-EnvironmentValue([string]$Name, [string]$Value)
{
	if ($null -eq $Value)
	{
		Remove-Item "Env:$Name" -ErrorAction SilentlyContinue
	}
	else
	{
		Set-Item "Env:$Name" $Value
	}
}

function Get-Median([double[]]$Values)
{
	if ($Values.Count -eq 0)
	{
		return 0.0
	}
	$sorted = $Values | Sort-Object
	$middle = [int][Math]::Floor($sorted.Count / 2)
	if (($sorted.Count % 2) -eq 1)
	{
		return [double]$sorted[$middle]
	}
	return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function Get-FileSHA256([string]$Path)
{
	$stream = [System.IO.File]::OpenRead($Path)
	$hash = [System.Security.Cryptography.SHA256]::Create()
	try
	{
		$bytes = $hash.ComputeHash($stream)
		return ([System.BitConverter]::ToString($bytes)).Replace("-", "").ToLowerInvariant()
	}
	finally
	{
		$hash.Dispose()
		$stream.Dispose()
	}
}

function Invoke-CapturedProcess([string]$FileName, [object[]]$Arguments)
{
	$quotedArguments = @($Arguments | ForEach-Object {
		'"' + ([string]$_).Replace('"', '\"') + '"'
	}) -join " "
	$startInfo = New-Object System.Diagnostics.ProcessStartInfo
	$startInfo.FileName = $FileName
	$startInfo.Arguments = $quotedArguments
	$startInfo.UseShellExecute = $false
	$startInfo.CreateNoWindow = $true
	$startInfo.RedirectStandardOutput = $true
	$startInfo.RedirectStandardError = $true
	$process = New-Object System.Diagnostics.Process
	$process.StartInfo = $startInfo
	if (-not $process.Start())
	{
		throw "failed to start profile client"
	}
	$stdout = $process.StandardOutput.ReadToEndAsync()
	$stderr = $process.StandardError.ReadToEndAsync()
	$process.WaitForExit()
	$lines = @(($stdout.Result + $stderr.Result) -split "`r?`n" |
		Where-Object { $_ -ne "" })
	return [pscustomobject]@{
		ExitCode = $process.ExitCode
		Lines = $lines
	}
}

function Add-UTF8Line([string]$Path, [string]$Line)
{
	$encoding = New-Object System.Text.UTF8Encoding($false)
	$writer = New-Object System.IO.StreamWriter($Path, $true, $encoding)
	try
	{
		$writer.WriteLine($Line)
	}
	finally
	{
		$writer.Dispose()
	}
}

try
{
	$diagnosticsCMake = if ($DisableDiagnostics) {
		"-DA126_ENABLE_RENDER_DIAGNOSTICS=OFF"
	} else {
		"-DA126_ENABLE_RENDER_DIAGNOSTICS=ON"
	}
	if (-not $SkipBuild)
	{
		$env:A126_BUILD_DIR = $BuildDirectory
		& (Join-Path $workspace "b.bat") configure `
			-DCMAKE_BUILD_TYPE=Release `
			-DA126_DEFAULT_RENDER_BACKEND=gl21 `
			-DA126_EXECUTABLE_NAME=a126cpp_profile `
			-DA126_ENABLE_GL_GPU_TESTS=ON `
			-DA126_ENABLE_VULKAN_BACKEND=ON `
			-DA126_ENABLE_D3D12_BACKEND=ON `
			$diagnosticsCMake
		if ($LASTEXITCODE -ne 0)
		{
			throw "profile build configuration failed with exit code $LASTEXITCODE"
		}
		& (Join-Path $workspace "b.bat") build Alpha126Cpp
		if ($LASTEXITCODE -ne 0)
		{
			throw "profile client build failed with exit code $LASTEXITCODE"
		}
	}

	$profileExecutable = if ([string]::IsNullOrWhiteSpace($Executable)) {
		Join-Path $repo "bin\a126cpp_profile.exe"
	} else {
		$Executable
	}
	if (-not (Test-Path -LiteralPath $profileExecutable -PathType Leaf))
	{
		throw "profile client is not built: $profileExecutable"
	}
	$profileExecutable = (Resolve-Path -LiteralPath $profileExecutable).Path
	$executableHash = Get-FileSHA256 $profileExecutable
	$pipelineCache = Join-Path $OutputDirectory "vulkan-pipeline-cache.bin"
	Remove-Item -LiteralPath $pipelineCache -Force -ErrorAction SilentlyContinue

	$workloads = @(
		[pscustomobject]@{ Name = "world"; Signs = 0; BlankText = $false; World = $World },
		[pscustomobject]@{ Name = "sign-control"; Signs = 0; BlankText = $false; World = "" },
		[pscustomobject]@{ Name = "sign-boards"; Signs = $SignCount; BlankText = $true; World = "" },
		[pscustomobject]@{ Name = "sign-text"; Signs = $SignCount; BlankText = $false; World = "" }
	)
	$workloads = @($workloads | Where-Object { $WorkloadNames -contains $_.Name })
	if ($workloads.Count -eq 0)
	{
		throw "WorkloadNames selected no known workloads"
	}
	if ($SkipForcedFinish -and $OnlyForcedFinish)
	{
		throw "SkipForcedFinish and OnlyForcedFinish cannot be combined"
	}
	if ($OnlyForcedFinish)
	{
		$finishModes = @($true)
	}
	elseif ($SkipForcedFinish)
	{
		$finishModes = @($false)
	}
	else
	{
		$finishModes = @($false, $true)
	}

	if ($DisableDiagnostics)
	{
		Remove-Item Env:A126_RENDER_DIAGNOSTICS -ErrorAction SilentlyContinue
	}
	else
	{
		$env:A126_RENDER_DIAGNOSTICS = "1"
	}
	Remove-Item Env:A126_LEGACYGL_VALIDATE -ErrorAction SilentlyContinue
	Remove-Item Env:A126_LEGACYGL_TRACE -ErrorAction SilentlyContinue
	$env:A126_VULKAN_PIPELINE_CACHE = $pipelineCache

	$resultsPath = Join-Path $OutputDirectory "profile-results.jsonl"
	$results = @()
	$runId = 0
	Push-Location -LiteralPath (Join-Path $repo "bin")
	try
	{
		foreach ($backend in $Backends)
		{
			foreach ($workload in $workloads)
			{
				foreach ($finishEachFrame in $finishModes)
				{
					# One discarded warmup per cell. Without it the first
					# measured run pays one-off costs (Vulkan pipeline-cache
					# miss, shader compile, cold file cache) and lands in its
					# own summary group, so a cell reports N-1 comparable runs
					# plus a single unrepresentative outlier.
					$warmupArguments = @(
						"--backend", $backend,
						"--sign-bench", $WarmupFrames, $workload.Signs,
						([int]$workload.BlankText), ([int]$finishEachFrame), $workload.World
					)
					$warmupResult = Invoke-CapturedProcess $profileExecutable $warmupArguments
					if ($warmupResult.ExitCode -ne 0)
					{
						throw "renderer profile warmup failed ($backend/$($workload.Name))"
					}

					for ($repetition = 1; $repetition -le $Repetitions; $repetition++)
					{
						$runId++
						$cacheWasPresent = Test-Path -LiteralPath $pipelineCache -PathType Leaf
						$arguments = @(
							"--backend", $backend,
							"--sign-bench", $MeasuredFrames, $workload.Signs,
							([int]$workload.BlankText), ([int]$finishEachFrame), $workload.World
						)
						$processResult = Invoke-CapturedProcess $profileExecutable $arguments
						$lines = $processResult.Lines
						$exitCode = $processResult.ExitCode
						$logName = "{0:D3}-{1}-{2}-finish{3}-run{4}.log" -f `
							$runId, $backend, $workload.Name, ([int]$finishEachFrame), $repetition
						$logPath = Join-Path $OutputDirectory $logName
						$lines | Set-Content -LiteralPath $logPath -Encoding UTF8
						if ($exitCode -ne 0)
						{
							throw "renderer profile run failed ($backend/$($workload.Name)): $logPath"
						}

						$metrics = [ordered]@{}
						$inMetrics = $false
						foreach ($line in $lines)
						{
							if ($line -eq "sign-bench")
							{
								$inMetrics = $true
								continue
							}
							if ($inMetrics -and $line -cmatch '^([a-z][a-z0-9_]*) (.*)$')
							{
								$metrics[$Matches[1]] = $Matches[2]
							}
						}
						$capability = ($lines | Where-Object { $_ -match 'capability_report' }) -join " | "
						$device = ($lines | Where-Object {
							$_ -match '^legacygl: GL vendor=' -or $_ -match '^vulkan: device ' -or
							$_ -match '^d3d12: adapter='
						}) -join " | "
						$record = [ordered]@{
							schema = 1
							timestamp_utc = (Get-Date).ToUniversalTime().ToString("o")
							run_id = $runId
							backend = $backend
							workload = $workload.Name
							repetition = $repetition
							finish_each_frame = [int]$finishEachFrame
							cache_trial = if ($backend -eq "vulkan") {
								if ($cacheWasPresent) { "warm" } else { "cold" }
							} else { "not-applicable" }
							build_type = "Release"
							render_diagnostics = [int](-not $DisableDiagnostics)
							legacy_validation = 0
							executable_sha256 = $executableHash
							capability_report = $capability
							device = $device
							raw_log = $logName
						}
						foreach ($entry in $metrics.GetEnumerator())
						{
							if (-not $record.Contains($entry.Key))
							{
								$record[$entry.Key] = $entry.Value
							}
						}
						$object = [pscustomobject]$record
						$results += $object
						Add-UTF8Line $resultsPath ($object | ConvertTo-Json -Compress)
						Write-Host ("[{0}/{1}] {2} {3} finish={4} run={5}: mean={6}ms p95={7}ms" -f `
							$runId, ($Backends.Count * $workloads.Count * $finishModes.Count * $Repetitions),
							$backend, $workload.Name, ([int]$finishEachFrame), $repetition,
							$metrics.mean_ms, $metrics.p95_ms)
					}
				}
			}
		}
	}
	finally
	{
		Pop-Location
	}

	$summary = foreach ($group in ($results | Group-Object {
		"$($_.backend)|$($_.workload)|$($_.finish_each_frame)|$($_.cache_trial)"
	}))
	{
		$first = $group.Group[0]
		$means = @($group.Group | ForEach-Object { [double]$_.mean_ms })
		$p95s = @($group.Group | ForEach-Object { [double]$_.p95_ms })
		[pscustomobject]@{
			backend = $first.backend
			workload = $first.workload
			finish_each_frame = $first.finish_each_frame
			cache_trial = $first.cache_trial
			runs = $group.Count
			median_mean_ms = Get-Median $means
			median_p95_ms = Get-Median $p95s
			median_fps = if ((Get-Median $means) -gt 0.0) { 1000.0 / (Get-Median $means) } else { 0.0 }
		}
	}
	$summary | Sort-Object backend, workload, finish_each_frame, cache_trial |
		Export-Csv -LiteralPath (Join-Path $OutputDirectory "profile-summary.csv") -NoTypeInformation
	Write-Host "Profile artifacts: $OutputDirectory"
}
finally
{
	Restore-EnvironmentValue "A126_BUILD_DIR" $oldBuildDirectory
	Restore-EnvironmentValue "A126_RENDER_DIAGNOSTICS" $oldDiagnostics
	Restore-EnvironmentValue "A126_LEGACYGL_VALIDATE" $oldValidation
	Restore-EnvironmentValue "A126_LEGACYGL_TRACE" $oldTrace
	Restore-EnvironmentValue "A126_VULKAN_PIPELINE_CACHE" $oldPipelineCache
}
