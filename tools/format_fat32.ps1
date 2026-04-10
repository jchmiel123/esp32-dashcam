# Format the 128GB SD card as FAT32 with 32KB allocation units
# Windows diskpart can't do FAT32 > 32GB, but Format-Volume can

$disk = Get-Disk | Where-Object { $_.BusType -eq 'USB' -and $_.Size -gt 100GB }
if (-not $disk) { Write-Host "No USB disk > 100GB found!"; pause; exit }

Write-Host "Found disk $($disk.Number): $($disk.FriendlyName) ($([math]::Round($disk.Size/1GB))GB)"

# Clean and create partition
$disk | Clear-Disk -RemoveData -Confirm:$false
$part = $disk | New-Partition -UseMaximumSize -AssignDriveLetter

Write-Host "Partition created: $($part.DriveLetter):"

# Format as FAT32 with 32KB clusters (max for FAT32 compatibility)
$vol = $part | Format-Volume -FileSystem FAT32 -AllocationUnitSize 32768 -NewFileSystemLabel "DASHCAM" -Confirm:$false

Write-Host "Formatted: $($vol.FileSystemLabel) $($vol.FileSystem) $([math]::Round($vol.Size/1GB,1))GB"
Write-Host "Done!"
pause
