class LocaleInfo {
    [int]$Index
    [string]$LCID
    [string]$StartTime
    LocaleInfo() { $this.Init(@{}) }
    LocaleInfo([hashtable]$Properties) { $this.Init($Properties) }
    [void] Init([hashtable]$Properties) {
        foreach ($Property in $Properties.Keys) {
            $this.$Property = $Properties.$Property
        }
    }
}

[LocaleInfo[]] $LocaleList = @(
    [LocaleInfo]::new(@{
            Index     = 0
            LCID      = "en-US"
            StartTime = "07:00"
        })
    [LocaleInfo]::new(@{
            Index     = 1
            LCID      = "zh-CN"
            StartTime = "16:00"
        })
)

$LocaleList | Select-Object "Index", "LCID" | Out-Host

[int]$Index = Read-Host "Please select the index number"
if ($Index -lt 0 -or $Index -ge $LocaleList.Count) {
    Write-Error "You have selected an incorrect index $Index"
    return
}

$TaskName = "BingWallpaper"
$Description = "Update Bing wallpapers daily"
$FilePath = (Get-Location).path + "\BingWallpaper.exe"

$Local = $LocaleList[$Index]
$LCID = $Local.LCID
$Time = $(Get-Date $Local.StartTime).ToLocalTime().AddMinutes(1)

$Action = New-ScheduledTaskAction -Execute "$FilePath" -Argument $LCID -WorkingDirectory %temp%
$Trigger = New-ScheduledTaskTrigger -Daily -At $Time
$Settings = New-ScheduledTaskSettingsSet -StartWhenAvailable -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
$Task = New-ScheduledTask -Description $Description -Action $Action -Trigger $Trigger -Settings $Settings
Register-ScheduledTask -TaskName $TaskName -InputObject $Task
Start-ScheduledTask -TaskName $TaskName
