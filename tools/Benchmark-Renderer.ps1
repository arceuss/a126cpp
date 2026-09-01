param(
	[string]$Backend = "vulkan",
	[int]$Frames = 600,
	[int[]]$Signs = @(0, 256),
	[bool]$BlankText = $false,
	[bool]$FinishEachFrame = $false,
	[string]$World = ""
)

$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo "bin"
$executable = Join-Path $bin "Alpha126Cpp.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf))
{
	throw "Alpha126Cpp.exe is not built: $executable"
}

Push-Location -LiteralPath $bin
try
{
	foreach ($signCount in $Signs)
	{
		& $executable --backend $Backend --sign-bench $Frames $signCount ([int]$BlankText) ([int]$FinishEachFrame) $World
		if ($LASTEXITCODE -ne 0)
		{
			exit $LASTEXITCODE
		}
	}
}
finally
{
	Pop-Location
}
