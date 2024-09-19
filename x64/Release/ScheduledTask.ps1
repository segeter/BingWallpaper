if (0 -eq $($args.Count))
{
	Write-Output "Missing parameter: register | unregister"
	return
}

$Arg = $args[0]
if ("register" -ine $Arg -and "unregister" -ine $Arg)
{
	Write-Output "Parameter error: register | unregister"
	return
}

$TaskName = "BingWallpaper"

if ("register" -ieq $Arg)
{
	$Description = "Update Bing wallpapers daily"
	$FilePath = (Get-Location).path + "\BingWallpaper.exe"
	$Action = New-ScheduledTaskAction -Execute "$FilePath"
	$Trigger = New-ScheduledTaskTrigger -Daily -At 4pm
	$Settings = New-ScheduledTaskSettingsSet -StartWhenAvailable -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
	$Task = New-ScheduledTask -Description $Description -Action $Action -Trigger $Trigger -Settings $Settings
	Register-ScheduledTask -TaskName $TaskName -InputObject $Task
}
else
{
	Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
}