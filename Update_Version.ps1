# Update_Version.ps1

$date = Get-Date

function ParseString ([string]$searchString) {
	[string]$result = (Select-String -Path '.\VersionInfo.h' -Pattern $searchString -SimpleMatch).Line
	if ([string]::IsNullOrEmpty($result)) { return -1 }
	return ($result -split "\s+", 3).Trim('"')[2]
}

$currentYear = $date.Year
$buildDateTime = "Build date: $($date.GetDateTimeFormats('u').Replace('Z', ''))"
$spanDays = [math]::Round((New-TimeSpan -Start $(Get-Date -Month 1 -Day 1 -Year 2000) -End $date).TotalDays)
$spanSecs = [math]::Round((New-TimeSpan -Start $($date.Date) -End $($date.DateTime)).TotalSeconds)

if (-not (Test-Path .\Version.h)) {
	Write-Host "Can't find file 'Version.h'"
	Start-Sleep -Seconds 3
	exit
}
Copy-Item .\Version.h .\VersionInfo.h

$verMajor = ParseString("V_MAJOR")
$verMinor = ParseString("V_MINOR")
$verPatch = ParseString("V_PATCH")
$pn = ParseString("PRODUCT_NAME")
$pa = ParseString("PRODUCT_AUTHORS")
$pys = ParseString("PRODUCT_YEAR_START")
$aboutBuild = ""
$pnf = ""

if ($pys -eq $currentYear) {
	$pcf = "Copyright (C) $pys by $pa"
} else {
	$pcf = "Copyright (C) $pys-$currentYear by $pa"
}	

if (Test-Path .\.git) {
	$gitCommitCount = Invoke-Expression -Command "git rev-list --count HEAD"
	$gitRevBranch = Invoke-Expression -Command "git symbolic-ref --short HEAD"
	$gitRevDate = Invoke-Expression -Command "git log -1 --date=rfc --pretty=format:%ad%n"
	$gitVerStr = Invoke-Expression -Command "git describe --long"

	if ($LastExitCode -eq 0) {
		$gitVerStr = $gitVerStr.Replace('-g', '-')
		$gitRevCount = $gitVerStr.Split('-')[-2]
	} else {
		$gitVerStr = ""
		$gitRevCount = $gitCommitCount
	}

	$vs = [string]::Join(".", $verMajor, $verMinor, $verPatch, $gitRevCount)
	$vn = [string]::Join(",", $verMajor, $verMinor, $verPatch, $gitRevCount)

	if ($gitVerStr -eq "") {
		$pnf = "$pn v$vs"
	} else {
		$pnf = "$pn $gitVerStr".Trim()
	}

	"#define GIT_VERSION_STR `"$gitVerStr`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
	"#define GIT_REV_BRANCH `"$gitRevBranch`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
	"#define GIT_REV_DATE `"Git date: $gitRevDate`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
	"#define GIT_REV_COUNT $gitRevCount" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
	"#define GIT_COMMIT_COUNT $gitCommitCount" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
	$aboutBuild = "Git date: $gitRevDate"
} else {
	$vs = [string]::Join(".", $verMajor, $verMinor, $verPatch)
	$vn = [string]::Join(",", $verMajor, $verMinor, $verPatch)
	$pnf = "$pn v$vs"
	$aboutBuild = $buildDateTime
}	

$intName = "$pn`C++"
$origName = "$pn.exe"

"#define ABOUT_BUILD `"$aboutBuild`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_NAME_FULL `"$pnf`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define INTERNAL_NAME `"$intName`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define ORIG_FILE_NAME `"$origName`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_COPYRIGHT `"$pcf`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define VERSION_STR `"$vs`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define VERSION_NUM $vn" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define SPAN_DAYS $spanDays" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define SPAN_SECS $spanSecs" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
