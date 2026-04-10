@echo off
echo ============================================
echo  Formatting Disk 2 (128GB SD Card) as FAT32
echo ============================================
echo.

(
echo select disk 2
echo clean
echo create partition primary
echo format fs=exfat label=DASHCAM quick
echo assign letter=F
) | diskpart

echo.
echo Done! Check F: drive.
pause
