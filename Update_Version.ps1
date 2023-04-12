# Update Version v2.1
# Required module: PSIni
# Installation: Install-Module -Name PsIni

$date = Get-Date

function addLeadingZero([int]$nvar) {
	[string]$svar = $null
	if ($nvar -lt 10) { $svar = '0' + $nvar.ToString() }
	else { $svar = $nvar.ToString() }
	return $svar
}

function getValue([string]$var) {
	switch -CaseSensitive ($var) {
		"%y" { $var = $currentYear; break }
		"%m" { $var = $currentMonth; break }
		"%d" { $var = $currentDay; break }
		"%t" { $var = -join ((addLeadingZero($currentHour)), (addLeadingZero($currentMinute)), (addLeadingZero($currentSecond))); break }
		"%D" { $var = $spanDays; break }
		"%S" { $var = $spanSecs; break }
		"%C" { $var = $gitCommitCount; break }
		"%c" { $var = $gitRevCount; break }
	}
	return $var
}

function makeVersionString([string]$vmaj, [string]$vmin, [string]$vbld, [string]$vrev) {
	[string]$vstr = $null
	if ($vmin -eq "") { $vmin = $vbld = $vrev = 0; $vstr = ($vmaj) -join '.' }
	else {
		if ($vbld -eq "") { $vbld = $vrev = 0; $vstr = ($vmaj, $vmin) -join '.' }
		else {
			if ($vrev -eq "") { $vrev = 0; $vstr = ($vmaj, $vmin, $vbld) -join '.' }
			else { $vstr = ($vmaj, $vmin, $vbld, $vrev) -join '.' }
		}
	}
	[string]$nstr = ($vmaj, $vmin, $vbld, $vrev) -join ','
	return @($vstr, $nstr)
}

#region Initializing variables
[string]$verMajor = [string]$verMinor = [string]$verBuild = [string]$verRevision = $null
[string]$pn = [string]$pa = [string]$aboutBuild = [string]$pnf = [string]$pcf = $null
[string]$buildDateTime = [string]$vs = [string]$vn = [string]$intName = [string]$origName = $null
[int]$pys = [int]$spanDays = [int]$spanSecs = $null
[int]$currentYear = [int]$currentMonth = [int]$currentDay = [int]$currentHour = [int]$currentMinute = [int]$currentSecond = $null
[string]$iniFile = ".\Version.ini"
#endregion

if (-not (Test-Path $iniFile)) {
	Write-Error "Can't find file '$iniFile'"
	# Start-Sleep -Seconds 3
	exit 1
}

[bool]$isGit = $false
if (Test-Path ".\.git") {
	$isGit = $true
	[int]$gitCommitCount = [int]$gitRevCount = $null
	[string]$gitRevDate = [string]$gitVerStr = [string]$gitAHash = $null
	$gitCommitCount = $(git rev-list --count HEAD)
	$gitRevDate = $(git log -1 HEAD --date=rfc --pretty=format:%ad%n)
	$gitVerStr = $(git describe HEAD --long)
	if ($LastExitCode -eq 0) {
		$gitVerStr = $gitVerStr.Replace('-g', '-')
		$gitRevCount = $gitVerStr.Split('-')[-2]
		$gitAHash = $gitVerStr.Split('-')[-1]
	}
	else {
		$gitVerStr = ""
		$gitRevCount = 0
		$gitAHash = $(git describe HEAD --always)
	}
}

#region Reading values from INI file
$config = $null
$config = (Get-IniContent -FilePath $iniFile)["Config"]
$verMajor = $config.Major
$verMinor = $config.Minor
$verBuild = $config.Build
$verRevision = $config.Revision
$pys = $config.ProductYearStart
$pn = $config.ProductName
$pa = $config.ProductAutors
$pd = $config.ProductDescription
#endregion

if ($verMajor -eq "") {
	Write-Error "Major version cannot be empty!"
	# Start-Sleep -Seconds 3
	exit 2
}

#region Variables preparation
$currentYear = $date.Year
$currentMonth = $date.Month
$currentDay = $date.Day
$currentHour = $date.Hour
$currentMinute = $date.Minute
$currentSecond = $date.Second
$buildDateTime = "Build date: $($date.GetDateTimeFormats('u').Replace('Z', ''))"
$spanDays = [math]::Round((New-TimeSpan -Start $(Get-Date -Month 1 -Day 1 -Year 2000) -End $date).TotalDays)
$spanSecs = [math]::Round((New-TimeSpan -Start $($date.Date) -End $($date.DateTime)).TotalSeconds)
if ($pys -eq $currentYear) { $pcf = "Copyright (C) $pys by $pa" } else { $pcf = "Copyright (C) $pys-$currentYear by $pa" }
$intName = "$pn-C++"
$origName = "$pn.exe"
$verMajor = getValue $verMajor
$verMinor = getValue $verMinor
$verBuild = getValue $verBuild
$verRevision = getValue $verRevision
#endregion

$vs = (makeVersionString $verMajor $verMinor $verBuild $verRevision)[0]
$vn = (makeVersionString $verMajor $verMinor $verBuild $verRevision)[1]

# if ([string]::IsNullOrEmpty($gitVerStr)) { $pnf = "$pn v$vs" } else { $pnf = "$pn $gitVerStr" }
if ($isGit) {
	$aboutBuild = "Git date: $gitRevDate"
	$pnf = "$pn v$vs ($gitAHash)"
}
else {
	$aboutBuild = $buildDateTime
	$pnf = "$pn v$vs"
}

#region Save all variables to file
"// $pn" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode
"// VersionInfo.h" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#pragma once" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_NAME `"$pn`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_NAME_FULL `"$pnf`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_AUTHORS `"$pa`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_DESCRIPTION `"$pd`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define ABOUT_BUILD `"$aboutBuild`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define INTERNAL_NAME `"$intName`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define ORIG_FILE_NAME `"$origName`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define PRODUCT_COPYRIGHT `"$pcf`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define VERSION_STR `"$vs`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
"#define VERSION_NUM $vn" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# "#define SPAN_DAYS $spanDays" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# "#define SPAN_SECS $spanSecs" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append

# if ($isGit) {
# 	# "#define GIT_VERSION_STR `"$gitVerStr`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# 	# "#define GIT_REV_DATE `"Git date: $gitRevDate`"" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# 	# "#define GIT_REV_COUNT $gitRevCount" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# 	# "#define GIT_COMMIT_COUNT $gitCommitCount" | Out-File -FilePath ".\VersionInfo.h" -Encoding unicode -Append
# }
#endregion

#region Print out all variables
# echo "            verMajor: [$verMajor]"
# echo "            verMinor: [$verMinor]"
# echo "            verBuild: [$verBuild]"
# echo "         verRevision: [$verRevision]"
# echo ""
# echo "          aboutBuild: [$aboutBuild]"
# echo "     productNameFull: [$pnf]"
# echo "productCopyrightFull: [$pcf]"
# echo "        internalName: [$intName]"
# echo "        originalName: [$origName]"
# echo "          versionStr: [$vs]"
# echo "          versionNum: [$vn]"
# echo "            spanDays: [$spanDays]"
# echo "            spanSecs: [$spanSecs]"
# echo ""
# echo "      gitCommitCount: [$gitCommitCount]"
# echo "         gitRevCount: [$gitRevCount]"
# echo "          gitRevDate: [$gitRevDate]"
# echo "           gitVerStr: [$gitVerStr]"
#endregion
